
/******************************** Description *********************************/

/*
 *  This module provides an API over HTTP between the web server and XBMC
 *
 *            heavily based on XBMCweb.cpp
 */

/********************************* Includes ***********************************/

#include "WebServer.h"
#include "XBMChttp.h"
#include "boost/make_shared.hpp"
#include "guiinfo/GUIInfoLabels.h"
#include "GUIInfoManager.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationVolumeHandling.h"
#include "includes.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"

#include "playlists/PlayListFactory.h"
#include "application/Application.h"
#include "messaging/ApplicationMessenger.h"
#include "Util.h"
#include "URL.h"
#include "PlayListPlayer.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/HDDirectory.h"
#include "filesystem/CDDADirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "video/VideoDatabase.h"
#include "GUIButtonControl.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "music/infoscanner/MusicInfoScraper.h"
#include "music/MusicDatabase.h"
#include "pictures/GUIWindowSlideShow.h"
#include "windows/GUIMediaWindow.h"
#include "windows/GUIWindowFileManager.h"
#include "filesystem/Directory.h"
#include "filesystem/VirtualDirectory.h"
#include "network/NetworkServices.h"
#include "network/UdpClient.h"
#include "filesystem/Directory.h"
#include "playlists/PlayList.h"
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SkinSettings.h"
#include "filesystem/File.h"
#include "filesystem/CurlFile.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "TextureCache.h"
#include "utils/SystemInfo.h"
#include "music/MusicThumbLoader.h"
#include "video/VideoThumbLoader.h"
#include "utils/MathUtils.h"

#include "platform/xbox/XKHDD.h"

#ifdef _WIN32PC
extern "C" FILE *fopen_utf8(const char *_Filename, const char *_Mode);
#else
#define fopen_utf8 fopen
#endif

using namespace std;
using namespace MUSIC_GRABBER;
using namespace XFILE;
using namespace PLAYLIST;
using namespace MUSIC_INFO;
using namespace ADDON;
using namespace KODI::MESSAGING;

#define XML_MAX_INNERTEXT_SIZE 256
#define MAX_PARAS 20
#define NO_EID -1

CXbmcHttp* m_pXbmcHttp;
CXbmcHttpShim* pXbmcHttpShim;


CUdpBroadcast::CUdpBroadcast() : CUdpClient()
{
  Create();
}

CUdpBroadcast::~CUdpBroadcast()
{
  Destroy();
}

bool CUdpBroadcast::broadcast(std::string message, int port)
{
  if (port>0)
    return Broadcast(port, message);
  else
    return false;
}


CXbmcHttp::CXbmcHttp()
{
  resetTags();
  CKey temp;
  key = temp;
  lastKey = temp;
  lastThumbFn="";
  lastPlayingInfo="";
  repeatKeyRate=0;
  MarkTime=0;
  pUdpBroadcast=NULL;
  shuttingDown=false;
  autoGetPictureThumbs=true;
  tempSkipWebFooterHeader=false;
}

CXbmcHttp::~CXbmcHttp()
{
  if (pUdpBroadcast)
  {
    delete pUdpBroadcast;
    pUdpBroadcast=NULL;
  }
  CLog::Log(LOGDEBUG, "xbmcHttp ends");
}

/*
** encode
**
** base64 encode a stream adding padding and line breaks as per spec.
*/
std::string CXbmcHttp::encodeFileToBase64(const std::string &inFilename, int linesize )
{
  unsigned char in[3];//, out[4];
  int len, blocksout = 0;
  std::string strBase64="";

//  Translation Table as described in RFC1113
  static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  CFile file;
  bool bOutput=false;
  if (file.Open(inFilename.c_str()))
  {
    while( file.GetPosition() != file.GetLength() )
    {
      memset(in, 0, sizeof(in));
      len = file.Read(in, 3);
      if( len )
      {
        strBase64 += cb64[ in[0] >> 2 ];
        strBase64 += cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
        strBase64 += (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
        strBase64 += (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
        blocksout++;
      }
      if(linesize == 0 && file.GetPosition() == file.GetLength())
        bOutput=true;
      else if ((linesize > 0) && (blocksout >= (linesize/4) || (file.GetPosition() == file.GetLength())))
        bOutput=true;
      if (bOutput)
      {
        if( blocksout && linesize > 0 )
          strBase64 += "\r";
        if( blocksout )
          strBase64 += closeTag ;
        blocksout = 0;
        bOutput=false;
      }
    }
    file.Close();
  }
  return strBase64;
}

/*
** decode
**
** decode a base64 encoded stream discarding padding, line breaks and noise
*/
bool CXbmcHttp::decodeBase64ToFile( const std::string &inString, const std::string &outfilename, bool append)
{
  unsigned char in[4], v; //out[3];
  bool ret=true;
  int i, len ;
  unsigned int ptr=0;
  FILE *outfile;

// Translation Table to decode
  static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

  try
  {
    if (append)
      outfile = fopen_utf8(CSpecialProtocol::TranslatePath(outfilename).c_str(), "ab" );
    else
      outfile = fopen_utf8(CSpecialProtocol::TranslatePath(outfilename).c_str(), "wb" );
    while( ptr < inString.length() )
    {
      for( len = 0, i = 0; i < 4 && ptr < inString.length(); i++ )
      {
        v = 0;
        while( ptr < inString.length() && v == 0 )
        {
          v = (unsigned char) inString[ptr];
          ptr++;
          v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
          if( v )
            v = (unsigned char) ((v == '$') ? 0 : v - 61);
        }
        if( ptr < inString.length() ) {
          len++;
          if( v )
            in[ i ] = (unsigned char) (v - 1);
        }
        else
          in[i] = 0;
      }
      if( len )
      {
        putc((unsigned char ) ((in[0] << 2 | in[1] >> 4) & 255), outfile );
        putc((unsigned char ) ((in[1] << 4 | in[2] >> 2) & 255), outfile );
        putc((unsigned char ) ((in[2] << 6) & 0xc0) | in[3], outfile );
      }
    }
    fclose(outfile);
  }
  catch (...)
  {
    ret=false;
  }
  return ret;
}

__int64 CXbmcHttp::fileSize(const std::string &filename)
{
  if (CFile::Exists(filename))
  {
    struct __stat64 s64;
    if (CFile::Stat(filename, &s64) == 0)
      return s64.st_size;
    else
      return -1;
  }
  else
    return -1;
}

void CXbmcHttp::resetTags()
{
  openTag="<li>";
  closeTag="\n";
  userHeader="";
  userFooter="";
  openRecordSet="";
  closeRecordSet="";
  openRecord="";
  closeRecord="";
  openField="<field>";
  closeField="</field>";
  openBroadcast="<b>";
  closeBroadcast="</b>";
  incWebHeader=true;
  incWebFooter=true;
  closeFinalTag=false;
}

std::string CXbmcHttp::procMask(std::string mask)
{
  StringUtils::ToLower(mask);
  if(mask=="[music]")
    return CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicExtensions;
  if(mask=="[video]")
    return CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoExtensions;
  if(mask=="[pictures]")
    return CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pictureExtensions;
  if(mask=="[files]")
    return "";
  return mask;
}

int CXbmcHttp::splitParameter(const std::string &parameter, std::string& command, std::string paras[], const std::string &sep)
//returns -1 if no command, -2 if too many parameters else the number of parameters
//assumption: sep.length()==1
{
  unsigned int num=0, p;
  std::string empty="";

  paras[0]="";
  for (p=0; p<parameter.length(); p++)
  {
    if (parameter.substr(p,1)==sep)
    {
      if (p<parameter.length()-1)
      {
        if (parameter.substr(p+1,1)==sep)
        {
          paras[num]+=sep;
          p+=1;
        }
        else
        {
          if (command!="")
          {
            StringUtils::Trim(paras[num]);
            num++;
            if (num==MAX_PARAS)
              return -2;
          }
          else
          {
            command=paras[0];
            paras[0]=empty;
            p++; //the ";" after the command is always followed by a space which we can jump over
          }
        }
      }
      else
      {
        if (command!="")
        {
          StringUtils::Trim(paras[num]);
          num++;
          if (num==MAX_PARAS)
            return -2;
        }
        else
        {
          command=paras[0];
          paras[0]=empty;
        }
      }
    }
    else
    {
      paras[num]+=parameter.substr(p,1);
    }
  }
  if (command=="")
    if (paras[0]!="")
    {
      command=paras[0];
      return 0;
    }
    else
      return -1;
  else
  {
    StringUtils::Trim(paras[num]);
    return num+1;
  }
}


bool CXbmcHttp::playableFile(const std::string &filename)
{
  CFileItem item(filename, false);
  return item.IsInternetStream() || CFile::Exists(filename);
}

int CXbmcHttp::SetResponse(const std::string &response)
{
  if (response.length()>=closeTag.length())
  {
    if ((response.substr(response.length() - closeTag.length())!=closeTag) && closeFinalTag)
      return SetResponseInternal(response+closeTag);
  }
  else
    if (closeFinalTag)
      return SetResponseInternal(response+closeTag);
  return SetResponseInternal(response);
}

int CXbmcHttp::displayDir(int numParas, std::string paras[])
{
  //mask = ".mp3|.wma" or one of "[music]", "[video]", "[pictures]", "[files]"-> matching files
  //mask = "*" or "/" -> just folders
  //mask = "" -> all files and folder
  //option = "1" (or "showdate") -> append date&time to file name
  //option = "size" -> just return the number of entries

  CFileItemList dirItems;
  std::string output="";

  std::string  folder, mask="", option="";
  int lineStart=0, numLines=-1;

  if (numParas==0)
  {
    return SetResponse(openTag+"Error:Missing folder");
  }
  folder = paras[0];
  if (folder.empty())
  {
    return SetResponse(openTag+"Error:Missing folder");
  }
  if (numParas>1)
    mask = procMask(paras[1]);
  if (numParas>2)
  {
    option = paras[2];
    StringUtils::ToLower(option);
  }
  if (numParas>3)
    lineStart = atoi(paras[3].c_str());
  if (numParas>4)
    numLines = atoi(paras[4].c_str());
  if (!CDirectory::GetDirectory(folder, dirItems, mask, DIR_FLAG_DEFAULTS))
  {
    return SetResponse(openTag+"Error:Not folder");
  }
  if (option=="size")
  {
    std::string tmp;
    tmp = StringUtils::Format("%i", dirItems.Size());
    return SetResponse(openTag+tmp);
  }
  dirItems.Sort(SortByLabel, SortOrderAscending);
  if (lineStart > dirItems.Size() || lineStart < 0)
    return SetResponse(openTag+"Error:Line start value out of range");
  if (numLines == -1)
    numLines = dirItems.Size();
  if (numLines + lineStart > dirItems.Size())
    numLines=dirItems.Size()-lineStart;
  for (int i = lineStart; i < lineStart + numLines; ++i)
  {
    CFileItemPtr itm = dirItems[i];
    std::string aLine;
    if (mask=="*" || mask=="/" || (mask =="" && itm->m_bIsFolder))
    {
      if (!URIUtils::HasSlashAtEnd(itm->GetPath()))
        aLine = closeTag + openTag + itm->GetPath().c_str() + "\\" ;
      else
        aLine = closeTag + openTag + itm->GetPath().c_str();
    }
    else if (!itm->m_bIsFolder)
      aLine = closeTag + openTag + itm->GetPath().c_str();

    if (!aLine.empty())
    {
      if (option=="1" || option=="showdate")
        output += aLine + "  ;" + itm->m_dateTime.GetAsLocalizedDateTime().c_str();
      else
        output += aLine;
    }
  }
  return SetResponse(output);
}

void CXbmcHttp::SetCurrentMediaItem(CFileItem& newItem)
{
  //  No audio file, we are finished here
  if (!newItem.IsAudio() )
    return;

  //  we have a audio file.
  //  Look if we have this file in database...
  bool bFound=false;
  CMusicDatabase musicdatabase;
  if (musicdatabase.Open())
  {
    CSong song;
    bFound=musicdatabase.GetSongByFileName(newItem.GetPath(), song);
    newItem.GetMusicInfoTag()->SetSong(song);
    musicdatabase.Close();
  }
  if (!bFound && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("musicfiles.usetags"))
  {
    //  ...no, try to load the tag of the file.
    auto_ptr<IMusicInfoTagLoader> pLoader(CMusicInfoTagLoaderFactory::CreateLoader(newItem));
    //  Do we have a tag loader for this file type?
    if (pLoader.get() != NULL)
      pLoader->Load(newItem.GetPath(),*newItem.GetMusicInfoTag());
  }

  //  If we have tag information, ...
  if (newItem.HasMusicInfoTag() && newItem.GetMusicInfoTag()->Loaded())
  {
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_UPDATE_CURRENT_ITEM, -1, -1, static_cast<void*>(new CFileItem(newItem)));
  }
}

int CXbmcHttp::FindPathInPlayList(int playList, std::string path)
{
  CPlayList& thePlayList = CServiceBroker::GetPlaylistPlayer().GetPlaylist(playList);
  for (int i = 0; i < thePlayList.size(); i++)
  {
    CFileItemPtr item = thePlayList[i];
    if (path==item->GetPath())
      return i;
  }
  return -1;
}

void CXbmcHttp::AddItemToPlayList(const CFileItemPtr &pItem, int playList, int sortMethod, std::string mask, bool recursive)
//if playlist==-1 then use slideshow
{
  if (pItem->m_bIsFolder)
  {
    // recursive
    if (pItem->IsParentFolder()) return;
    std::string strDirectory=pItem->GetPath();
    CFileItemList items;
    CDirectory::GetDirectory(pItem->GetPath(), items, mask, DIR_FLAG_DEFAULTS);
    items.Sort(SortByLabel, SortOrderAscending);
    for (int i=0; i < items.Size(); ++i)
      if (!(CFileItem*)items[i]->m_bIsFolder || recursive)
        AddItemToPlayList(items[i], playList, sortMethod, mask, recursive);
  }
  else
  {
    //selected item is a file, add it to playlist
    if (playList==-1)
    {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
      if (!pSlideShow)
        return ;
      pSlideShow->Add(pItem.get());
    }
    else
      CServiceBroker::GetPlaylistPlayer().Add(playList, pItem);
  }
}

bool CXbmcHttp::LoadPlayList(std::string strPath, int iPlaylist, bool clearList, bool autoStart)
{
  CFileItem *item = new CFileItem(URIUtils::GetFileName(strPath));
  item->SetPath(strPath);

  auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(*item));
  if ( NULL == pPlayList.get())
    return false;
  if (!pPlayList->Load(item->GetPath()))
    return false;

  CPlayList& playlist = (*pPlayList);

  if (playlist.size() == 0)
    return false;

  // first item of the list, used to determine the intent
  CFileItemPtr playlistItem = playlist[0];

  if ((playlist.size() == 1) && (autoStart))
  {
    // just 1 song? then play it (no need to have a playlist of 1 song)
    CFileItemList *l = new CFileItemList; //don't delete,
    l->Add(boost::make_shared<CFileItem>(playlistItem->GetPath(), false));
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, -1, -1, static_cast<void*>(l));
    return true;
  }

  if (clearList)
    CServiceBroker::GetPlaylistPlayer().ClearPlaylist(iPlaylist);

  CServiceBroker::GetPlaylistPlayer().Add(iPlaylist, *pPlayList);

  if (autoStart)
    if (CServiceBroker::GetPlaylistPlayer().GetPlaylist( iPlaylist ).size() )
    {
      CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(iPlaylist);
      CServiceBroker::GetPlaylistPlayer().Reset();
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PLAYLISTPLAYER_PLAY);
      return true;
    }
    else
      return false;
  else
    return true;
  return false;
}

void CXbmcHttp::copyThumb(std::string srcFn, std::string destFn)
//Copies src file to dest, unless src=="" or src doesn't exist in which case dest is deleted
{

  if (destFn=="")
    return;
  if (srcFn=="")
  {
    try
    {
      if (CFile::Exists(destFn))
        CFile::Delete(destFn);
      lastThumbFn=srcFn;
    }
    catch (...)
    {
    }
  }
  else
    if (srcFn!=lastThumbFn)
      try
      {
        lastThumbFn=srcFn;
        if (CFile::Exists(srcFn))
          CFile::Copy(srcFn, destFn);
      }
      catch (...)
      {
        return;
      }
}

int CXbmcHttp::xbmcGetMediaLocation(int numParas, std::string paras[])
{
  // getmediadirectory&parameter=type;location;options
  // options = showdate, pathsonly
  // returns a listing of
  // label;path;0|1=folder;date

  int iType = -1;
  std::string strType;
  std::string strMask;
  std::string strLocation;
  std::string strOutput;

  if (numParas < 1)
    return SetResponse(openTag+"Error: must supply media type at minimum");
  else
  {
    if (paras[0] == "music")
      iType = 0;
    else if (paras[0] == "video")
      iType = 1;
    else if (paras[0] == "pictures")
      iType = 2;
    else if (paras[0] == "files")
      iType = 3;
    if (iType < 0)
      return SetResponse(openTag+"Error: invalid media type; valid options are music, video, pictures");

    strType = paras[0];
    StringUtils::ToLower(strType);
    if (numParas > 1)
      strLocation = paras[1];
  }

  // handle options
  bool bShowDate = false;
  bool bPathsOnly = false;
  bool bSize = false;
  int lineStart=0, numLines=-1;
  if (numParas > 2)
  {
    for (int i = 2; i < numParas; ++i)
    {
      if (paras[i] == "showdate")
        bShowDate = true;
      else if (paras[i] == "pathsonly")
        bPathsOnly = true;
      else if (paras[i] == "size")
        bSize = true;
      else if (StringUtils::IsNaturalNumber(paras[i]))
      {
        lineStart=atoi(paras[i].c_str());
        i++;
        if (i<numParas)
          if (StringUtils::IsNaturalNumber(paras[i]))
          {
            numLines=atoi(paras[i].c_str());
            i++;
          }
      }
    }
    // pathsonly and showdate are mutually exclusive, pathsonly wins
    if (bPathsOnly)
      bShowDate = false;
  }

  VECSOURCES *pShares = NULL;
  enum SHARETYPES { MUSIC, VIDEO, PICTURES, FILES };
  switch(iType)
  {
  case MUSIC:
    {
      pShares = CMediaSourceSettings::GetInstance().GetSources("music");
      strMask = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicExtensions;
    }
    break;
  case VIDEO:
    {
      pShares = CMediaSourceSettings::GetInstance().GetSources("video");;
      strMask = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoExtensions;
    }
    break;
  case PICTURES:
    {
      pShares = CMediaSourceSettings::GetInstance().GetSources("pictures");
      strMask = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pictureExtensions;
    }
    break;
  case FILES:
    {
      pShares = CMediaSourceSettings::GetInstance().GetSources("files");
      strMask = "";
    }
    break;
  }

  if (!pShares)
    return SetResponse(openTag+"Error");

  // TODO: Why are we insisting the passed path has anything to do with
  //       the shares in question??
  //       Surely we should just grab the directory regardless??
  //
  // kraqh3d's response:
  // When I added this function, it was meant to behave more like Xbmc internally.
  // This code emulates the CVirtualDirectory class which does not allow arbitrary
  // fetching of directories. (nor does ActivateWindow for that matter.)
  // You can still use the older "getDirectory" command which is unbounded and will
  // fetch any old folder.

  // special locations
  bool bSpecial = false;
  CURL url(strLocation);
  if (url.GetProtocol() == "rar" || url.GetProtocol() == "zip")
    bSpecial = true;
  if (strType == "music")
  {
    if (url.GetProtocol() == "musicdb")
      bSpecial = true;
    else if (strLocation == "$playlists")
    {
      strLocation = "special://musicplaylists/";
      bSpecial = true;
    }
  }
  else if (strType == "video")
  {
    if (strLocation == "$playlists")
    {
      strLocation = "special://videoplaylists/";
      bSpecial = true;
    }
  }

  if (!strLocation.empty() && !bSpecial)
  {
    VECSOURCES VECSOURCES = *pShares;
    bool bIsShareName = false;
    int iIndex = CUtil::GetMatchingSource(strLocation, VECSOURCES, bIsShareName);
    if (iIndex < 0 || iIndex >= (int)VECSOURCES.size())
    {
      std::string strError = "Error: invalid location, " + strLocation;
      return SetResponse(openTag+strError);
    }
    if (bIsShareName)
      strLocation = VECSOURCES[iIndex].strPath;
  }

  CFileItemList items;
  if (strLocation.empty())
  {
    std::string params[2];
    params[0] = strType;
    params[1] = "appendone";
    if (bPathsOnly)
      params[1] = "pathsonly";
    return xbmcGetSources(2, params);
  }
  else if (!CDirectory::GetDirectory(strLocation, items, strMask, DIR_FLAG_DEFAULTS))
  {
    std::string strError = "Error: could not get location, " + strLocation;
    return SetResponse(openTag+strError);
  }
  if (bSize)
  {
    std::string tmp;
    tmp = StringUtils::Format("%i",items.Size());
    return SetResponse(openTag+tmp);
  }
  items.Sort(SortByLabel, SortOrderAscending);
  std::string strLine;
  if (lineStart>items.Size() || lineStart<0)
    return SetResponse(openTag+"Error:Line start value out of range");
  if (numLines==-1)
    numLines=items.Size();
  if ((numLines+lineStart)>items.Size())
    numLines=items.Size()-lineStart;
  for (int i=lineStart; i<lineStart+numLines; ++i)
  {
    CFileItemPtr item = items[i];
    std::string strLabel = item->GetLabel();
    StringUtils::Replace(strLabel, ";",";;");
    std::string strPath = item->GetPath();
    StringUtils::Replace(strPath, ";",";;");
    std::string strFolder = "0";
    if (item->m_bIsFolder)
    {
      if (!item->IsFileFolder() && !URIUtils::HasSlashAtEnd(strPath))
        URIUtils::AddSlashAtEnd(strPath);
      strFolder = "1";
    }
    strLine = openTag;
    if (!bPathsOnly)
      strLine += strLabel + ";";
    strLine += strPath;
    if (!bPathsOnly)
      strLine += ";" + strFolder;
    if (bShowDate)
    {
      strLine += ";" + item->m_dateTime.GetAsLocalizedDateTime();
    }
    strLine += closeTag;
    strOutput += strLine;
  }
  return SetResponse(strOutput);
}

int CXbmcHttp::xbmcGetXBEID(int numParas, std::string paras[])
{
  if (numParas==0) {
    return SetResponse(openTag+"Error:Missing Parameter");
  }
  std::string tmp;
  if (CFile::Exists(paras[0].c_str()))
  {
    tmp = StringUtils::Format("%09x",CUtil::GetXbeID(paras[0]));
    return SetResponse(openTag + tmp);
  }
  else
  {
     return SetResponse(openTag+"Error:xbe doesn't exist");
  }

}

int CXbmcHttp::xbmcGetXBETitle(int numParas, std::string paras[])
{
  std::string xbeinfo;
  if (numParas==0) {
    return SetResponse(openTag+"Error:Missing Parameter");
  }
  std::string tmp;
  if (CUtil::GetXBEDescription(paras[0],xbeinfo))
  {
    tmp = StringUtils::Format("%s",xbeinfo.c_str());
    return SetResponse(openTag + tmp);
  }
  else
  {
     return SetResponse(openTag+"Error:Failed to getxbetitle");
  }
}

int CXbmcHttp::xbmcGetSources(int numParas, std::string paras[])
{
  // returns the share listing in this format:
  // type;name;path
  // literal semicolons are translated into ;;
  // options include the type, and pathsonly boolean

  int iStart = 0;
  int iEnd   = 5;
  bool bShowType = true;
  bool bShowName = true;

  if (numParas > 0)
  {
    if (paras[0] == "music")
    {
      iStart = 0;
      iEnd   = 1;
      bShowType = false;
    }
    else if (paras[0] == "video")
    {
      iStart = 1;
      iEnd   = 2;
      bShowType = false;
    }
    else if (paras[0] == "pictures")
    {
      iStart = 2;
      iEnd   = 3;
      bShowType = false;
    }
    else if (paras[0] == "files")
    {
      iStart = 3;
      iEnd   = 4;
      bShowType = false;
    }
    else if (paras[0] == "programs")
    {
      iStart = 4;
      iEnd   = 5;
      bShowType = false;
    }
    else
      numParas = 0;
  }

  bool bAppendOne = false;
  if (numParas > 1)
  {
    // special case where getmedialocation calls getshares
    if (paras[1] == "appendone")
      bAppendOne = true;
    else if (paras[1] == "pathsonly")
      bShowName = false;
  }

  std::string strOutput;
  enum SHARETYPES { MUSIC, VIDEO, PICTURES, FILES, PROGRAMS };
  for (int i = iStart; i < iEnd; ++i)
  {
    std::string strType;
    VECSOURCES *pShares = NULL;
    switch(i)
    {
    case MUSIC:
      {
        strType = "music";
        pShares = CMediaSourceSettings::GetInstance().GetSources("music");
      }
      break;
    case VIDEO:
      {
        strType = "video";
        pShares = CMediaSourceSettings::GetInstance().GetSources("video");
      }
      break;
    case PICTURES:
      {
        strType = "pictures";
        pShares = CMediaSourceSettings::GetInstance().GetSources("pictures");
      }
      break;
    case FILES:
      {
        strType = "files";
        pShares = CMediaSourceSettings::GetInstance().GetSources("files");
      }
      break;
    case PROGRAMS:
      {
        strType = "programs";
        pShares = CMediaSourceSettings::GetInstance().GetSources("programs");
      }
      break;
    }

    if (!pShares)
      return SetResponse(openTag+"Error");

    VECSOURCES VECSOURCES = *pShares;
    for (int j = 0; j < (int)VECSOURCES.size(); ++j)
    {
      CMediaSource share = VECSOURCES.at(j);
      std::string strName = share.strName;
      StringUtils::Replace(strName, ";", ";;");
      std::string strPath = share.strPath;
      StringUtils::Replace(strPath, ";", ";;");
      URIUtils::AddSlashAtEnd(strPath);
      std::string strLine = openTag;
      if (bShowType)
        strLine += strType + ";";
      if (bShowName)
        strLine += strName + ";";
      strLine += strPath;
      if (bAppendOne)
        strLine += ";1";
      strLine += closeTag;
      strOutput += strLine;
    }
  }
  return SetResponse(strOutput);
}

int CXbmcHttp::xbmcQueryMusicDataBase(int numParas, std::string paras[])
{
  return SetResponse(openTag+"Error:Deprecated function");
}

int CXbmcHttp::xbmcQueryVideoDataBase(int numParas, std::string paras[])
{
  return SetResponse(openTag+"Error:Deprecated function");
}

int CXbmcHttp::xbmcQueryProgramDataBase(int numParas, std::string paras[])
{
  return SetResponse("Error: Deprecated!");
}

int CXbmcHttp::xbmcExecVideoDataBase(int numParas, std::string paras[])
{
  return SetResponse(openTag+"Error:Deprecated function");
}

int CXbmcHttp::xbmcExecMusicDataBase(int numParas, std::string paras[])
{
  return SetResponse(openTag+"Error:Deprecated function");
}

int CXbmcHttp::xbmcAddToPlayListFromDB(int numParas, std::string paras[])
{
  if (numParas == 0)
    return SetResponse(openTag+"Error: Missing Parameter");

  std::string type  = paras[0];

  // Perform open query if empty where clause
  if (paras[1] == "")
    paras[1] = "1 = 1";

  CDatabase::Filter filter;
  filter.where = paras[1];

  int playList;
  CFileItemList filelist;
  if (type == "songs")
  {
    playList = PLAYLIST::TYPE_MUSIC;

    CMusicDatabase musicdatabase;
    if (!musicdatabase.Open())
      return SetResponse(openTag+ "Error: Could not open music database");
    musicdatabase.GetSongsByWhere("musicdb://songs/", filter, filelist);
    musicdatabase.Close();
  }
  else if (type == "movies" ||
           type == "episodes" ||
           type == "musicvideos")
  {
    playList = PLAYLIST::TYPE_VIDEO;

    CVideoDatabase videodatabase;
    if (!videodatabase.Open())
      return SetResponse(openTag+"Error: Could not open video database");

    if (type == "movies")
      videodatabase.GetMoviesByWhere("videodb://movies/titles/", filter, filelist);
    else if (type == "episodes")
      videodatabase.GetEpisodesByWhere("videodb://tvshows/titles/", filter, filelist);
    else if (type == "musicvideos")
      videodatabase.GetMusicVideosByWhere("videodb://musicvideos/titles/", filter, filelist);
    videodatabase.Close();
  }
  else
    return SetResponse(openTag+"Invalid type. Must be songs,music,episodes or musicvideo");

  if (filelist.Size() == 0)
    return SetResponse(openTag+"Nothing added");

  CServiceBroker::GetPlaylistPlayer().Add(playList, filelist);
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcAddToPlayList(int numParas, std::string paras[])
{
  //parameters=playList;mask;recursive
  std::string strFileName, mask="";
  bool changed=false, recursive=true;
  int playList ;

  if (numParas==0)
    return SetResponse(openTag+"Error:Missing Parameter");
  else
  {
    if (numParas==1) //no playlist and no mask
      playList=CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    else
    {
      playList=atoi(paras[1].c_str());
      if (playList==-1)
        playList=CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
      if(numParas>2) //includes mask
        mask=procMask(paras[2]);
      if (numParas>3) //recursive
        recursive=(paras[3]=="1");
    }
    strFileName=paras[0] ;
    CFileItemPtr pItem(new CFileItem(strFileName));
    pItem->SetPath(strFileName.c_str());
    if (pItem->IsPlayList())
      changed=LoadPlayList(pItem->GetPath(), playList, false, false);
    else
    {
      bool bResult = CDirectory::Exists(pItem->GetPath());
      pItem->m_bIsFolder=bResult;
      pItem->m_bIsShareOrDrive=false;
      if (bResult || CFile::Exists(pItem->GetPath()))
      {
        AddItemToPlayList(pItem, playList, 0, mask, recursive);
        changed=true;
      }
    }
    if (changed)
    {
      return SetResponse(openTag+"OK");
    }
    else
      return SetResponse(openTag+"Error");
  }
}

int CXbmcHttp::xbmcGetTagFromFilename(int numParas, std::string paras[])
{
  std::string strFileName;
  if (numParas==0) {
    return SetResponse(openTag+"Error:Missing Parameter");
  }
  strFileName=URIUtils::GetFileName(paras[0]);
  CFileItem *pItem = new CFileItem(strFileName);
  pItem->SetPath(paras[0].c_str());
  if (!pItem->IsAudio())
  {
    delete pItem;
    return SetResponse(openTag+"Error:Not Audio");
  }

  CMusicInfoTag* tag=pItem->GetMusicInfoTag();
  bool bFound=false;
  CSong song;
  CMusicDatabase musicdatabase;
  if (musicdatabase.Open())
  {
    bFound=musicdatabase.GetSongByFileName(pItem->GetPath(), song);
    musicdatabase.Close();
  }
  if (bFound)
  {
    tag->SetReleaseDate(song.strReleaseDate);
    tag->SetTrackNumber(song.iTrack);
    tag->SetAlbum(song.strAlbum);
    tag->SetArtist(song.GetArtist());
    tag->SetGenre(song.genre);
    tag->SetTitle(song.strTitle);
    tag->SetDuration(song.iDuration);
    tag->SetLoaded(true);
  }
  else
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("musicfiles.usetags"))
    {
      // get correct tag parser
      auto_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(*pItem));
      if (NULL != pLoader.get())
      {
        // get id3tag
        if ( !pLoader->Load(pItem->GetPath(),*tag))
          tag->SetLoaded(false);
      }
      else
      {
        return SetResponse(openTag+"Error:Could not load TagLoader");
      }
    }
    else
    {
      return SetResponse(openTag+"Error:System not set to use tags");
    }
  if (tag->Loaded())
  {
    std::string output, tmp;

    output = openTag+"Artist:" + StringUtils::Join(tag->GetArtist(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator).c_str();
    output += closeTag+openTag+"Album:" + tag->GetAlbum().c_str();
    output += closeTag+openTag+"Title:" + tag->GetTitle().c_str();
    tmp = StringUtils::Format("%i", tag->GetTrackNumber());
    output += closeTag+openTag+"Track number:" + tmp;
    tmp = StringUtils::Format("%i", tag->GetDuration());
    output += closeTag+openTag+"Duration:" + tmp;
    output += closeTag+openTag+"Genre:" + StringUtils::Join(tag->GetGenre(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator).c_str();
    tmp = StringUtils::Format("%i", tag->GetYear());
    output += closeTag+openTag+"Release year:" + tmp;
    CMusicThumbLoader loader;
    if (loader.LoadItem(pItem) && pItem->HasArt("thumb"))
      output += closeTag+openTag+"Thumb:" + (std::string)pItem->GetArt("thumb");
    else {
      output += closeTag+openTag+"Thumb:[None]";
    }
    delete pItem;
    return SetResponse(output);
  }
  else
  {
    delete pItem;
    return SetResponse(openTag+"Error:No tag info");
  }
}

int CXbmcHttp::xbmcClearPlayList(int numParas, std::string paras[])
{
  int playList ;
  if (numParas==0)
    playList = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() ;
  else
    playList=atoi(paras[0].c_str()) ;
  CServiceBroker::GetPlaylistPlayer().ClearPlaylist( playList );
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcSwapPlayListItems(int numParas, std::string paras[])
{
  int iPlayList ;
  if (numParas < 3)
    return SetResponse(openTag+"Error: Not enough parameters");
  iPlayList=CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  if (numParas > 2)
    iPlayList = atoi(paras[2].c_str());
  CPlayList& playlist = CServiceBroker::GetPlaylistPlayer().GetPlaylist(iPlayList);

  int item1;
  if (StringUtils::IsNaturalNumber(paras[0]))
    item1 = atoi(paras[0].c_str());
  else
    item1=FindPathInPlayList(iPlayList,paras[0]);
  int item2;
  if (StringUtils::IsNaturalNumber(paras[1]))
    item2 = atoi(paras[1].c_str());
  else
    item2=FindPathInPlayList(iPlayList,paras[1]);

  if (playlist.Swap(item1,item2))
    return SetResponse(openTag+"OK");

  return SetResponse(openTag+"Error swapping items");
}

int CXbmcHttp::xbmcGetDirectory(int numParas, std::string paras[])
{
  if (numParas>0)
    return displayDir(numParas, paras);
  else
    return SetResponse(openTag+"Error:No path") ;
}

int CXbmcHttp::xbmcGetMovieDetails(int numParas, std::string paras[])
{
  if (numParas>0)
  {
    CFileItem *item = new CFileItem(paras[0]);
    item->SetPath(paras[0].c_str());
    if (item->IsVideo()) {
      CVideoDatabase m_database;
      CVideoInfoTag aMovieRec;
      m_database.Open();
      if (m_database.HasMovieInfo(paras[0].c_str()))
      {
        std::string thumb, output;
        m_database.GetMovieInfo(paras[0].c_str(),aMovieRec);
        std::string tmp = StringUtils::Format("%i", aMovieRec.GetYear());
        output = closeTag+openTag+"Year:" + tmp;
        output += closeTag+openTag+"Director:" + StringUtils::Join(aMovieRec.m_director, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator).c_str();
        output += closeTag+openTag+"Title:" + aMovieRec.m_strTitle.c_str();
        output += closeTag+openTag+"Plot:" + aMovieRec.m_strPlot.c_str();
        output += closeTag+openTag+"Genre:" + StringUtils::Join(aMovieRec.m_genre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator).c_str();
        std::string strRating = StringUtils::Format("%3.3f", aMovieRec.GetRating().rating);
        if (strRating=="") strRating="0.0";
        output += closeTag+openTag+"Rating:" + strRating;
        std::string cast = aMovieRec.GetCast(true);
        /*for (CVideoInfoTag::iCast it = aMovieRec.m_cast.begin(); it != aMovieRec.m_cast.end(); ++it)
        {
          std::string character;
          character = StringUtils::Format("%s %s %s\n", it->first.c_str(), g_localizeStrings.Get(20347).c_str(), it->second.c_str());
          cast += character;
        }*/
        output += closeTag+openTag+"Cast:" + cast;
        CVideoThumbLoader loader;
        if (loader.LoadItem(item) && item->HasArt("thumb"))
          thumb = CTextureUtils::GetWrappedImageURL(item->GetArt("thumb"));
        else
          thumb = "[None]";
        output += closeTag+openTag+"Thumb:" + thumb;
        m_database.Close();
        delete item;
        return SetResponse(output);
      }
      else
      {
        m_database.Close();
        delete item;
        return SetResponse(openTag+"Error:Not found");
      }
    }
    else
    {
      delete item;
      return SetResponse(openTag+"Error:Not a video") ;
    }
  }
  else
    return SetResponse(openTag+"Error:No file name") ;
}

int CXbmcHttp::xbmcGetCurrentlyPlaying(int numParas, std::string paras[])
//paras: filename_to_save_thumb, filename_if_nothing_playing, only_return_info_if_changed
{
  std::string output="", tmp="", tag="", thumbFn="", thumbNothingPlaying="", thumb="";
  bool justChange=false, changed=false;
  if (numParas>0)
    thumbFn=paras[0];
  if (numParas>1)
    thumbNothingPlaying=paras[1];
  if (numParas>2)
  {
    StringUtils::ToLower(paras[2]);
    justChange=paras[2]=="true";
  }
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
  if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW && pSlideShow)
  {
    const boost::shared_ptr<const CFileItem>& slide = pSlideShow->GetCurrentSlide();
    output=openTag+"Filename:"+slide->GetPath().c_str();
    if (lastPlayingInfo!=output)
    {
      changed=true;
      lastPlayingInfo=output;
    }
    if (justChange && !changed)
      return SetResponse(openTag+"Changed:False");
    output+=closeTag+openTag+"Type:Picture" ;
    std::string resolution = "0x0";
    if (slide && slide->HasPictureInfoTag() && slide->GetPictureInfoTag()->Loaded())
      resolution = slide->GetPictureInfoTag()->GetInfo(SLIDE_RESOLUTION);
    output+=closeTag+openTag+"Resolution:" + resolution;
    CFileItem item(*slide);
    std::string thumbURL = CTextureUtils::GetWrappedThumbURL(item.GetPath());
    if (autoGetPictureThumbs || CServiceBroker::GetTextureCache()->HasCachedImage(thumbURL))
      thumb = thumbURL;
    if (thumb.empty())
    {
      thumb = "[None]";
      copyThumb("DefaultPicture.png",thumbFn);
    }
    else
      copyThumb(thumb,thumbFn);
    output+=closeTag+openTag+"Thumb:"+thumb;
    if (changed)
      output+=closeTag+openTag+"Changed:True";
    else
      output+=closeTag+openTag+"Changed:False";
    return SetResponse(output);
  }

  CFileItem &fileItem = g_application.CurrentFileItem();
  if (fileItem.GetPath().empty())
  {
    output=openTag+"Filename:[Nothing Playing]";
    if (lastPlayingInfo!=output)
    {
      changed=true;
      lastPlayingInfo=output;
    }
    if (justChange && !changed)
      return SetResponse(openTag+"Changed:False");
    copyThumb(thumbNothingPlaying,thumbFn);
    return SetResponse(output);
  }
  else
  {
    CURL url(fileItem.GetPath());
    std::string strPath(url.GetWithoutUserDetails());
    CURL::Decode(strPath);
    output = openTag + "Filename:" + strPath;  // currently playing item filename
    const CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
    if (appPlayer->IsPlaying())
      if (!appPlayer->IsPaused())
        output+=closeTag+openTag+"PlayStatus:Playing";
      else
        output+=closeTag+openTag+"PlayStatus:Paused";
    else
      output+=closeTag+openTag+"PlayStatus:Stopped";
    if (appPlayer->IsPlayingVideo())
    { // Video information
      tmp = StringUtils::Format("%i",CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx());
      output+=closeTag+openTag+"VideoNo:"+tmp;  // current item # in playlist
      output+=closeTag+openTag+"Type"+tag+":Video" ;
      const CVideoInfoTag* tagVal=CServiceBroker::GetGUI()->GetInfoManager().GetCurrentMovieTag();
      if (tagVal)
      {
        if (!tagVal->m_strShowTitle.empty())
          output+=closeTag+openTag+"Show Title"+tag+":"+tagVal->m_strShowTitle.c_str() ;
        if (!tagVal->m_strTitle.empty())
          output+=closeTag+openTag+"Title"+tag+":"+tagVal->m_strTitle.c_str() ;
        //now have enough info to check for a change
        if (lastPlayingInfo!=output)
        {
          changed=true;
          lastPlayingInfo=output;
        }
        if (justChange && !changed)
          return SetResponse(openTag+"Changed:False");
        //if still here, continue collecting info
        if (!tagVal->m_genre.empty())
          output+=closeTag+openTag+"Genre"+tag+":"+StringUtils::Join(tagVal->m_genre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator).c_str();
        if (!tagVal->m_studio.empty())
          output+=closeTag+openTag+"Studio"+tag+":"+StringUtils::Join(tagVal->m_studio, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator).c_str();
        if (tagVal && tagVal->m_director.size() > 0)
          output+=closeTag+openTag+"Director"+tag+":"+StringUtils::Join(tagVal->m_director, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator).c_str();
        if (tagVal->m_writingCredits.size() > 0)
          output+=closeTag+openTag+"Writer"+tag+":"+StringUtils::Join(tagVal->m_writingCredits, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator).c_str();
        if (!tagVal->m_strTagLine.empty())
          output+=closeTag+openTag+"Tagline"+tag+":"+tagVal->m_strTagLine.c_str();
        if (!tagVal->m_strPlotOutline.empty())
          output+=closeTag+openTag+"Plotoutline"+tag+":"+tagVal->m_strPlotOutline.c_str();
        if (!tagVal->m_strPlot.empty())
          output+=closeTag+openTag+"Plot"+tag+":"+tagVal->m_strPlot.c_str();
        if (tagVal->GetRating().rating != 0.0f)  // only non-zero ratings are of interest
          output = StringUtils::Format("%s%03.1f (%i %s)",output+closeTag+openTag+"Rating"+tag+":",tagVal->GetRating().rating, tagVal->GetRating().votes, g_localizeStrings.Get(20350).c_str());
        if (!tagVal->m_strOriginalTitle.empty())
          output+=closeTag+openTag+"Original Title"+tag+":"+tagVal->m_strOriginalTitle.c_str();
        if (tagVal->m_premiered.IsValid())
          output+=closeTag+openTag+"Premiered"+tag+":"+tagVal->m_premiered.GetAsLocalizedDate().c_str();
        if (!tagVal->m_strStatus.empty())
          output+=closeTag+openTag+"Status"+tag+":"+tagVal->m_strStatus.c_str();
        if (!tagVal->m_strProductionCode.empty())
          output+=closeTag+openTag+"Production Code"+tag+":"+tagVal->m_strProductionCode.c_str();
        if (tagVal->m_firstAired.IsValid())
          output+=closeTag+openTag+"First Aired"+tag+":"+tagVal->m_firstAired.GetAsLocalizedDate().c_str();
        if (tagVal->HasYear())
          output = StringUtils::Format("%s%i",output+closeTag+openTag+"Year"+tag+":",tagVal->GetYear());
        if (tagVal->m_iSeason != -1)
          output = StringUtils::Format("%s%i",output+closeTag+openTag+"Season"+tag+":",tagVal->m_iSeason);
        if (tagVal->m_iEpisode != -1)
          output = StringUtils::Format("%s%i",output+closeTag+openTag+"Episode"+tag+":",tagVal->m_iEpisode);
      }
      else
      {
        //now have enough info to estimate a change
        if (lastPlayingInfo!=output)
        {
          changed=true;
          lastPlayingInfo=output;
        }
        if (justChange && !changed)
         return SetResponse(openTag+"Changed:False");
        //if still here, continue collecting info
      }
      thumb=CServiceBroker::GetGUI()->GetInfoManager().GetImage(VIDEOPLAYER_COVER, (DWORD)-1);

      copyThumb(thumb,thumbFn);
      output+=closeTag+openTag+"Thumb"+tag+":"+thumb;
    }
    else if (appPlayer->IsPlayingAudio())
    { // Audio information
      tmp = StringUtils::Format("%i",CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx());
      output+=closeTag+openTag+"SongNo:"+tmp;  // current item # in playlist
      output+=closeTag+openTag+"Type"+tag+":Audio";
      const CMusicInfoTag* tagVal=CServiceBroker::GetGUI()->GetInfoManager().GetCurrentSongTag();
      if (tagVal && !tagVal->GetTitle().empty())
        output+=closeTag+openTag+"Title"+tag+":"+tagVal->GetTitle().c_str();
      if (tagVal && tagVal->GetTrackNumber())
      {
        std::string tmp;
        tmp = StringUtils::Format("%i",(int)tagVal->GetTrackNumber());
        output+=closeTag+openTag+"Track"+tag+":"+tmp;
      }
      if (tagVal && !tagVal->GetArtist().empty())
        output+=closeTag+openTag+"Artist"+tag+":"+StringUtils::Join(tagVal->GetArtist(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator).c_str();
      if (tagVal && !tagVal->GetAlbum().empty())
        output+=closeTag+openTag+"Album"+tag+":"+tagVal->GetAlbum().c_str();
      //now have enough info to check for a change
      if (lastPlayingInfo!=output)
      {
        changed=true;
        lastPlayingInfo=output;
      }
      if (justChange && !changed)
      return SetResponse(openTag+"Changed:False");
      //if still here, continue collecting info
      if (tagVal && !tagVal->GetGenre().empty())
        output+=closeTag+openTag+"Genre"+tag+":"+StringUtils::Join(tagVal->GetGenre(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator).c_str();
      if (tagVal && tagVal->GetYear())
        output+=closeTag+openTag+"Year"+tag+":"+tagVal->GetYearString().c_str();
      if (tagVal && !tagVal->GetURL().empty())
        output+=closeTag+openTag+"URL"+tag+":"+tagVal->GetURL().c_str();
      if (tagVal && !CServiceBroker::GetGUI()->GetInfoManager().GetItemLabel(&fileItem, INFO::DEFAULT_CONTEXT, MUSICPLAYER_LYRICS).empty())
        output+=closeTag+openTag+"Lyrics"+tag+":"+CServiceBroker::GetGUI()->GetInfoManager().GetItemLabel(&fileItem, INFO::DEFAULT_CONTEXT, MUSICPLAYER_LYRICS).c_str();

      // TODO: Should this be a tagitem member?? (wouldn't have vbr updates though)
      std::string bitRate(CServiceBroker::GetGUI()->GetInfoManager().GetItemLabel(&fileItem, INFO::DEFAULT_CONTEXT, MUSICPLAYER_BITRATE));
      // TODO: This should be a static tag item
      std::string sampleRate(CServiceBroker::GetGUI()->GetInfoManager().GetItemLabel(&fileItem, INFO::DEFAULT_CONTEXT, MUSICPLAYER_SAMPLERATE));
      if (!bitRate.empty())
        output+=closeTag+openTag+"Bitrate"+tag+":"+bitRate;
      if (!sampleRate.empty())
        output+=closeTag+openTag+"Samplerate"+tag+":"+sampleRate;
      thumb=CServiceBroker::GetGUI()->GetInfoManager().GetImage(MUSICPLAYER_COVER, (DWORD)-1);
      copyThumb(thumb,thumbFn);
      output+=closeTag+openTag+"Thumb"+tag+":"+thumb;
    }
    output+=closeTag+openTag+"Time:"+StringUtils::SecondsToTimeString(MathUtils::round_int(g_application.GetTime()), TIME_FORMAT_HH_MM_SS).c_str();
    output+=closeTag+openTag+"Duration:";
    if (appPlayer->IsPlayingVideo())
      output += StringUtils::SecondsToTimeString(MathUtils::round_int(g_application.GetTotalTime()), TIME_FORMAT_HH_MM_SS);
    else
      output += StringUtils::SecondsToTimeString(MathUtils::round_int(g_application.GetTotalTime()), TIME_FORMAT_HH_MM_SS);
    tmp = StringUtils::Format("%i",(int)g_application.GetPercentage());
    output+=closeTag+openTag+"Percentage:"+tmp;
    // file size
    if (!fileItem.m_dwSize)
      fileItem.m_dwSize = fileSize(fileItem.GetPath());
    if (fileItem.m_dwSize)
    {
      tmp = StringUtils::Format("%"PRId64,fileItem.m_dwSize);
      output+=closeTag+openTag+"File size:"+tmp;
    }
    if (changed)
      output+=closeTag+openTag+"Changed:True";
    else
      output+=closeTag+openTag+"Changed:False";
  }
  return SetResponse(output);
}

int CXbmcHttp::xbmcGetMusicLabel(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing Parameter");
  else
  {
    int item=(int)atoi(paras[0].c_str());
    return SetResponse(openTag+CServiceBroker::GetGUI()->GetInfoManager().GetItemLabel(&g_application.CurrentFileItem(), INFO::DEFAULT_CONTEXT, item).c_str());
  }
}

int CXbmcHttp::xbmcGetVideoLabel(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing Parameter");
  else
  {
    int item=(int)atoi(paras[0].c_str());
    return SetResponse(openTag+CServiceBroker::GetGUI()->GetInfoManager().GetItemLabel(&g_application.CurrentFileItem(), INFO::DEFAULT_CONTEXT, item).c_str());
  }
}

int CXbmcHttp::xbmcGetPercentage()
{
  const CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->HasPlayer())
  {
    std::string tmp;
    tmp = StringUtils::Format("%i",(int)g_application.GetPercentage());
    return SetResponse(openTag + tmp ) ;
  }
  else
    return SetResponse(openTag+"Error");
}

int CXbmcHttp::xbmcSeekPercentage(int numParas, std::string paras[], bool relative)
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing Parameter");
  else
  {
    const CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
    if (appPlayer->HasPlayer())
    {
      float percent=(float)atof(paras[0].c_str());
      if (relative)
      {
        double newPos = g_application.GetTime() + percent * 0.01 * g_application.GetTotalTime();
        if ((newPos>=0) && (newPos/1000<=g_application.GetTime()))
        {
          g_application.SeekTime(newPos);
          return SetResponse(openTag+"OK");
        }
        else
          return SetResponse(openTag+"Error:Out of range");
      }
      else
      {
        g_application.SeekPercentage(percent);
        return SetResponse(openTag+"OK");
      }
    }
    else
      return SetResponse(openTag+"Error:Loading mPlayer");
  }
}

int CXbmcHttp::xbmcMute()
{
  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationVolumeHandling> appVolume = components.GetComponent<CApplicationVolumeHandling>();
  appVolume->ToggleMute();
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcSetVolume(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing Parameter");
  else
  {
    int iPercent = atoi(paras[0].c_str());
    CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<CApplicationVolumeHandling> appVolume = components.GetComponent<CApplicationVolumeHandling>();
    appVolume->SetVolume(iPercent);
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcGetVolume()
{
  const CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<const CApplicationVolumeHandling> appVolume = components.GetComponent<CApplicationVolumeHandling>();
  std::string tmp;
  tmp = StringUtils::Format("%i",appVolume->GetVolumeRatio());
  return SetResponse(openTag + tmp);
}

int CXbmcHttp::xbmcClearSlideshow()
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return SetResponse(openTag+"Error:Could not create slideshow");
  else
  {
    pSlideShow->Reset();
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcPlaySlideshow(int numParas, std::string paras[])
{ // (filename(;1)) -> 1 indicates recursive
  // TODO: add suoport for new random and notrandom options
  unsigned int recursive = 0;
  if (numParas>1 && paras[1] == "1")
    recursive=1;
  CGUIMessage msg(GUI_MSG_START_SLIDESHOW, 0, 0, recursive);
  if (numParas==0)
    msg.SetStringParam("");
  else
    msg.SetStringParam(paras[0]);
  CGUIWindow *pWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
  if (pWindow) pWindow->OnMessage(msg);
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcSlideshowSelect(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing filename");
  else
  {
    CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
    if (!pSlideShow)
      return SetResponse(openTag+"Error:Could not create slideshow");
    else
    {
      pSlideShow->Select(paras[0]);
      return SetResponse(openTag+"OK");
    }
  }
}

int CXbmcHttp::xbmcAddToSlideshow(int numParas, std::string paras[])
//filename;mask;recursive=1
{
  std::string mask="";
  bool recursive=true;
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  if (numParas>1)
    mask=procMask(paras[1]);
  if (numParas>2)
    recursive=paras[2]=="1";
  CFileItemPtr pItem(new CFileItem(paras[0]));
  pItem->m_bIsShareOrDrive=false;
  pItem->SetPath(paras[0].c_str());
  // if its not a picture type, test to see if its a folder
  if (!pItem->IsPicture())
  {
    IDirectory *pDirectory = CFactoryDirectory::Create(pItem->GetURL());
    if (!pDirectory)
      return SetResponse(openTag+"Error");
    bool bResult=pDirectory->Exists(CURL(pItem->GetPath()));
    pItem->m_bIsFolder=bResult;
  }
  AddItemToPlayList(pItem, -1, 0, mask, recursive); //add to slideshow
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcSetPlaySpeed(int numParas, std::string paras[])
{
  if (numParas>0) {
    CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
    appPlayer->SetPlaySpeed(atoi(paras[0].c_str()));
    return SetResponse(openTag+"OK");
  }
  else
    return SetResponse(openTag+"Error:Missing parameter");
}

int CXbmcHttp::xbmcGetPlaySpeed()
{
  const CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
  std::string strSpeed;
  strSpeed = StringUtils::Format("%i", appPlayer->GetPlaySpeed());
  return SetResponse(openTag + strSpeed );
}

int CXbmcHttp::xbmcGetGUIDescription()
{
  std::string strWidth, strHeight;
  strWidth = StringUtils::Format("%i", CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth());
  strHeight = StringUtils::Format("%i", CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());
  return SetResponse(openTag+"Width:" + strWidth + closeTag+openTag+"Height:" + strHeight  );
}

int CXbmcHttp::xbmcGetGUIStatus()
{
  std::string output, tmp, strTmp;
  CGUIMediaWindow *mediaWindow = (CGUIMediaWindow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_MUSIC_NAV);
  if (mediaWindow)
    output = closeTag+openTag+"MusicPath:" + mediaWindow->CurrentDirectory().GetPath().c_str();
  mediaWindow = (CGUIMediaWindow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_VIDEO_NAV);
  if (mediaWindow)
    output += closeTag+openTag+"VideoPath:" + mediaWindow->CurrentDirectory().GetPath().c_str();
  mediaWindow = (CGUIMediaWindow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_PICTURES);
  if (mediaWindow)
    output += closeTag+openTag+"PicturePath:" + mediaWindow->CurrentDirectory().GetPath().c_str();
  mediaWindow = (CGUIMediaWindow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_PROGRAMS);
  if (mediaWindow)
    output += closeTag+openTag+"ProgramsPath:" + mediaWindow->CurrentDirectory().GetPath().c_str();
  CGUIWindowFileManager *fileManager = (CGUIWindowFileManager *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_FILES);
  if (fileManager)
  {
    output += closeTag+openTag+"FilesPath1:" + fileManager->CurrentDirectory(0).GetPath().c_str();
    output += closeTag+openTag+"FilesPath2:" + fileManager->CurrentDirectory(1).GetPath().c_str();
  }
  int iWin=CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  CGUIWindow* pWindow=CServiceBroker::GetGUI()->GetWindowManager().GetWindow(iWin);
  tmp = StringUtils::Format("%i", iWin);
  output += openTag+"ActiveWindow:" + tmp;
  if (pWindow)
  {
    output += StringUtils::Format("%s%sActiveWindowName:%s", closeTag, openTag, g_localizeStrings.Get(iWin).c_str());
    CGUIControl* pControl=pWindow->GetFocusedControl();
    if (pControl)
    {
      std::string id;
      id = StringUtils::Format("%d",(int)pControl->GetID());
      output += closeTag+openTag+"ControlId:" + id;
      strTmp = pControl->GetDescription();
      if (pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      {
        output += closeTag+openTag+"Type:Button";
        if (strTmp!="")
          output += closeTag+openTag+"Description:" + strTmp;
        if (((CGUIButtonControl *)pControl)->HasClickActions())
          output += closeTag+openTag+"Execution:" + ((CGUIButtonControl *)pControl)->GetClickActions().GetFirstAction().c_str();
      }
      else if (pControl->GetControlType() == CGUIControl::GUICONTROL_SPIN)
      {
        output += closeTag+openTag+"Type:Spin"+closeTag+openTag+"Description:" + strTmp;
      }
    }
  }
  return SetResponse(output);
}

int CXbmcHttp::xbmcGetThumb(int numParas, std::string paras[], bool bGetThumb)
{
  std::string thumb="";
  int linesize=80;
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  bool bImgTag=false;
  // only allow the old GetThumb command to accept "imgtag"
  if (bGetThumb && numParas==2 && paras[1] == "imgtag")
  {
    bImgTag=true;
    thumb="<img src=\"data:image/jpg;base64,";
    linesize=0;
  }
  if (numParas>1)
  {
    StringUtils::ToLower(paras[1]);
    tempSkipWebFooterHeader=paras[1] == "bare";
  }
  if (numParas>2)
  {
    StringUtils::ToLower(paras[2]);
    tempSkipWebFooterHeader=paras[2] == "bare";
  }
  if (URIUtils::IsRemote(paras[0]))
  {
    std::string strDest="special://temp/xbmcDownloadFile.tmp";
    CFile::Copy(paras[0], strDest, NULL, NULL) ;
    if (CFile::Exists(strDest))
    {
      thumb+=encodeFileToBase64(strDest,linesize);
      CFile::Delete(strDest);
    }
    else
    {
      return SetResponse(openTag+"Error");
    }
  }
  else
    thumb+=encodeFileToBase64(paras[0],linesize);

  if (bImgTag)
  {
    thumb+="\" alt=\"Your browser doesnt support this\" title=\"";
    thumb+=paras[0];
    thumb+="\">";
  }
  if (tempSkipWebFooterHeader)
  {
    std::string strHttpResponseHeaders = StringUtils::Format(
    "HTTP/1.0 200 OK\r\n"
    "Pragma: no-cache\r\n"
    "Cache-control: no-cache\r\n"
    "Content-Length: %i\r\n"
    "Content-Type: text/plain\r\n"
    "\r\n"
    ,thumb.length()
    );
    return SetResponse( strHttpResponseHeaders + thumb);
  }
  return SetResponse(thumb) ;
}

int CXbmcHttp::xbmcGetThumbFilename(int numParas, std::string paras[])
{
  return SetResponse(openTag+"Error:Deprecated function") ;
}

int CXbmcHttp::xbmcPlayerPlayFile(int numParas, std::string paras[])
{
  int iPlaylist = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing file parameter");
  if (numParas>1)
    iPlaylist = atoi(paras[1].c_str());
  CFileItem item(paras[0], FALSE);
  if (iPlaylist == PLAYLIST::TYPE_NONE)
    iPlaylist = PLAYLIST::TYPE_MUSIC;
  if (item.IsPlayList())
  {
    LoadPlayList(paras[0], iPlaylist, true, true);
    std::string strPlaylist;
    strPlaylist = StringUtils::Format("%i", iPlaylist);
    return SetResponse(openTag+"OK:Playlist="+strPlaylist);
  }
  else
  {
    CFileItemList *l = new CFileItemList; //don't delete,
    l->Add(boost::make_shared<CFileItem>(paras[0], false));
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, -1, -1, static_cast<void*>(l));
    const CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
    if(appPlayer->IsPlaying())
      return SetResponse(openTag+"OK");
  }
  return SetResponse(openTag+"Error:Could not play file");
}

int CXbmcHttp::xbmcGetCurrentPlayList()
{
  std::string tmp;
  tmp = StringUtils::Format("%i", CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());
  return SetResponse(openTag + tmp  );
}

int CXbmcHttp::xbmcSetCurrentPlayList(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing playlist") ;
  else {
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(atoi(paras[0].c_str()));
    return SetResponse(openTag+"OK") ;
  }
}

int CXbmcHttp::xbmcGetPlayListContents(int numParas, std::string paras[])
{
  // option = showindex -> index;path
  // option = showtitle -> path;tracktitle
  // option = showduration -> path;duration

  std::string list="";
  int playList = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  bool bShowIndex = false;
  bool bShowTitle = false;
  bool bShowDuration = false;
  for (int i = 0; i < numParas; ++i)
  {
    if (paras[i] == "showindex")
      bShowIndex = true;
    else if (paras[i] == "showtitle")
      bShowTitle = true;
    else if (paras[i] == "showduration")
      bShowDuration = true;
    else if (StringUtils::IsNaturalNumber(paras[i]))
      playList = atoi(paras[i].c_str());
  }
  CPlayList& thePlayList = CServiceBroker::GetPlaylistPlayer().GetPlaylist(playList);
  if (thePlayList.size()==0)
    list=openTag+"[Empty]" ;
  bool bIsMusic = (playList == PLAYLIST::TYPE_MUSIC);
  for (int i = 0; i < thePlayList.size(); i++)
  {
    CFileItemPtr item = thePlayList[i];
    const CMusicInfoTag* tagVal = NULL;
    if (bIsMusic)
      tagVal = item->GetMusicInfoTag();
    std::string strInfo;
    if (bShowIndex)
      strInfo = StringUtils::Format("%i;", i);
    if (tagVal && tagVal->GetURL()!="")
      strInfo += tagVal->GetURL();
    else
      strInfo += item->GetPath();
    if (bShowTitle && tagVal)
      strInfo += ';' + tagVal->GetTitle();
    if (bShowDuration && tagVal)
    {
      strInfo += ';' + StringUtils::SecondsToTimeString(tagVal->GetDuration(), TIME_FORMAT_GUESS);
    }
    list += closeTag + openTag + strInfo;
  }
  return SetResponse(list) ;
}

int CXbmcHttp::xbmcGetPlayListLength(int numParas, std::string paras[])
{
  int playList;

  if (numParas<1)
    playList=CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  else
    playList=atoi(paras[0].c_str());
  CPlayList& thePlayList = CServiceBroker::GetPlaylistPlayer().GetPlaylist(playList);

  std::string tmp;
  tmp = StringUtils::Format("%i", thePlayList.size());
  return SetResponse(openTag + tmp );
}

int CXbmcHttp::xbmcGetSlideshowContents()
{
  std::string list="";
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return SetResponse(openTag+"Error");
  else
  {
    CFileItemList slideshowContents;
    pSlideShow->GetSlideShowContents(slideshowContents);
    if (slideshowContents.Size()==0)
      list=openTag+"[Empty]" ;
    else
    for (int i = 0; i < slideshowContents.Size(); ++i)
      list += closeTag+openTag + slideshowContents[i]->GetPath().c_str();
    return SetResponse(list) ;
  }
}

int CXbmcHttp::xbmcGetPlayListSong(int numParas, std::string paras[])
{
  std::string Filename;
  int iSong;

  if (numParas<1)
  {
    std::string tmp;
    tmp = StringUtils::Format("%i", CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx());
    return SetResponse(openTag + tmp );
  }
  else {
    CPlayList thePlayList;
    iSong=atoi(paras[0].c_str());
    if (iSong!=-1){
      thePlayList=CServiceBroker::GetPlaylistPlayer().GetPlaylist( CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() );
      if (thePlayList.size()>iSong) {
        Filename=thePlayList[iSong]->GetPath();
        return SetResponse(openTag + Filename );
      }
    }
  }
  return SetResponse(openTag+"Error");
}

int CXbmcHttp::xbmcSetPlayListSong(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing song number");
  else
  {
    CServiceBroker::GetPlaylistPlayer().Play(atoi(paras[0].c_str()), "");
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcPlayListNext()
{
  CServiceBroker::GetPlaylistPlayer().PlayNext();
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcPlayListPrev()
{
  CServiceBroker::GetPlaylistPlayer().PlayPrevious();
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcRemoveFromPlayList(int numParas, std::string paras[])
{
  if (numParas > 0)
  {
    int iPlaylist = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    std::string strItem = paras[0];
    int itemToRemove;
    if (numParas > 1)
      iPlaylist = atoi(paras[1].c_str());
    if (StringUtils::IsNaturalNumber(strItem))
      itemToRemove=atoi(strItem.c_str());
    else
      itemToRemove=FindPathInPlayList(iPlaylist, strItem);
    // The current playing song can't be removed
    const CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
    if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC && appPlayer->IsPlayingAudio()
      && CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx() == itemToRemove)
      return SetResponse(openTag+"Error:Can't remove current playing song");
    if (itemToRemove<0 || itemToRemove>=CServiceBroker::GetPlaylistPlayer().GetPlaylist(iPlaylist).size())
      return SetResponse(openTag+"Error:Item not found or parameter out of range");
    CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST::TYPE_MUSIC).Remove(itemToRemove);

    // Correct the current playing song in playlistplayer
    if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_MUSIC && appPlayer->IsPlayingAudio())
    {
      int iCurrentSong = CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx();
      if (itemToRemove <= iCurrentSong)
      {
        iCurrentSong--;
        CServiceBroker::GetPlaylistPlayer().SetCurrentItemIdx(iCurrentSong);
      }
    }
    return SetResponse(openTag+"OK");
  }
  else
    return SetResponse(openTag+"Error:Missing parameter");
}

std::string CXbmcHttp::GetOpenTag()
{
  return openTag;
}

std::string CXbmcHttp::GetCloseTag()
{
  return closeTag;
}

CKey CXbmcHttp::GetKey()
{
  if (repeatKeyRate!=0)
    if ((XbmcThreads::SystemClockMillis() - MarkTime) >=  (unsigned int)repeatKeyRate)
    {
      MarkTime=XbmcThreads::SystemClockMillis();
      key=lastKey;
    }
  return key;
}

void CXbmcHttp::ResetKey()
{
  CKey newKey;
  key = newKey;
}

int CXbmcHttp::xbmcSetKey(int numParas, std::string paras[])
{
  int buttonCode=0;
  uint8_t leftTrigger=0, rightTrigger=0;
  float fLeftThumbX=0.0f, fLeftThumbY=0.0f, fRightThumbX=0.0f, fRightThumbY=0.0f ;
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameters");

  else
  {
    buttonCode=(int) strtol(paras[0].c_str(), NULL, 0);
    if (numParas>1) {
      leftTrigger=(uint8_t) atoi(paras[1].c_str()) ;
      if (numParas>2) {
        rightTrigger=(uint8_t) atoi(paras[2].c_str()) ;
        if (numParas>3) {
          fLeftThumbX=(float) atof(paras[3].c_str()) ;
          if (numParas>4) {
            fLeftThumbY=(float) atof(paras[4].c_str()) ;
            if (numParas>5) {
              fRightThumbX=(float) atof(paras[5].c_str()) ;
              if (numParas>6)
                fRightThumbY=(float) atof(paras[6].c_str()) ;
            }
          }
        }
      }
    }
    CKey tempKey(buttonCode, leftTrigger, rightTrigger, fLeftThumbX, fLeftThumbY, fRightThumbX, fRightThumbY) ;
    tempKey.SetFromHttpApi(true);
    key = tempKey;
    lastKey = key;
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcSetKeyRepeat(int numParas, std::string paras[])
{
  if (numParas!=1)
    return SetResponse(openTag+"Error:Should be only one parameter");
  else
  {
    repeatKeyRate = atoi(paras[0].c_str());
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcAction(int numParas, std::string paras[], int theAction)
{
  bool showingSlideshow=(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW);

  switch(theAction)
  {
  case 1:
    if (showingSlideshow) {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow)
        pSlideShow->OnAction(CAction(ACTION_PAUSE));
    }
    else
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_PAUSE);
    return SetResponse(openTag+"OK");
    break;
  case 2:
    if (showingSlideshow) {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow)
        pSlideShow->OnAction(CAction(ACTION_STOP));
    }
    else
      //g_application.StopPlaying();
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_STOP);
    return SetResponse(openTag+"OK");
    break;
  case 3:
    if (showingSlideshow) {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow)
        pSlideShow->OnAction(CAction(ACTION_NEXT_PICTURE));
    }
    else
      CServiceBroker::GetPlaylistPlayer().PlayNext();
    return SetResponse(openTag+"OK");
    break;
  case 4:
    if (showingSlideshow) {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow)
        pSlideShow->OnAction(CAction(ACTION_PREV_PICTURE));
    }
    else
      CServiceBroker::GetPlaylistPlayer().PlayPrevious();
    return SetResponse(openTag+"OK");
    break;
  case 5:
    if (showingSlideshow)
    {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow) {
        pSlideShow->OnAction(CAction(ACTION_ROTATE_PICTURE_CW));
        return SetResponse(openTag+"OK");
      }
      else
        return SetResponse(openTag+"Error");
    }
    else
      return SetResponse(openTag+"Error");
    break;
  case 6:
    if (showingSlideshow)
    {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow) {
        if (numParas>1) {
          CAction action(ACTION_ANALOG_MOVE, (float)atof(paras[0].c_str()), (float)atof(paras[1].c_str()));
          pSlideShow->OnAction(action);
          return SetResponse(openTag+"OK");
        }
        else
          return SetResponse(openTag+"Error:Missing parameters");
      }
      else
        return SetResponse(openTag+"Error");
    }
    else
      return SetResponse(openTag+"Error");
    break;
  case 7:
    if (showingSlideshow)
    {
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow) {
        if (numParas>0)
        {
          pSlideShow->OnAction(CAction(ACTION_ZOOM_LEVEL_NORMAL+atoi(paras[0].c_str())));
          return SetResponse(openTag+"OK");
        }
        else
          return SetResponse(openTag+"Error:Missing parameters");
      }
      else
        return SetResponse(openTag+"Error");
    }
    else
      return SetResponse(openTag+"Error");
    break;
  default:
    return SetResponse(openTag+"Error");
  }
}

int CXbmcHttp::xbmcExit(int theAction)
{
  if (theAction>0 && theAction<6)
  {
    SetResponse(openTag+"OK");
    shuttingDown=true;
    return theAction;
  }
  else
    return SetResponse(openTag+"Error");
}

int CXbmcHttp::xbmcLookupAlbum(int numParas, std::string paras[])
// paras: album
//        album, artist
//        album, artist, 1
{
  std::string albums="", album, artist="", tmp;
  double relevance;
  bool rel = false;
  AddonPtr addon;
  if (!ADDON::CAddonSystemSettings::GetInstance().GetActive(ADDON::AddonType::SCRAPER_ALBUMS, addon))
    return -1;
  ScraperPtr info = boost::dynamic_pointer_cast<CScraper>(addon);
  if (!info)
    return -1;

  CMusicInfoScraper scraper(info);

  if (numParas<1)
    return SetResponse(openTag+"Error:Missing album name");
  else
  {
    try
    {
      int cnt=0;
      album=paras[0];
      if (numParas>1)
      {
        artist = paras[1];
        scraper.FindAlbumInfo(album, artist);
        if (numParas>2)
          rel = (paras[2]=="1");
      }
      else
        scraper.FindAlbumInfo(album);
      //wait a max of 20s
      while (!scraper.Completed() && cnt++<200)
        Sleep(100);
      if (scraper.Succeeded())
      {
        // did we find at least 1 album?
        int iAlbumCount=scraper.GetAlbumCount();
        if (iAlbumCount >=1)
        {
          for (int i=0; i < iAlbumCount; ++i)
          {
            CMusicAlbumInfo& info = scraper.GetAlbum(i);
            albums += closeTag+openTag + info.GetTitle2().c_str() + "<@@>" + info.GetAlbumURL().GetFirstThumbUrl().c_str();
            if (rel)
            {
              relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum, album, StringUtils::Join(info.GetAlbum().GetAlbumArtist(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator), artist);
              tmp = StringUtils::Format("%f",relevance);
              albums += "<@@@>"+tmp;
            }
          }
          return SetResponse(albums) ;
        }
        else
          return SetResponse(openTag+"Error:No albums found") ;
      }
      else
        return SetResponse(openTag+"Error:Scraping") ;
    }
    catch (...)
    {
      return SetResponse(openTag+"Error");
    }
  }
}

int CXbmcHttp::xbmcChooseAlbum(int numParas, std::string paras[])
{
  std::string output="";

  if (numParas<1)
    return SetResponse(openTag+"Error:Missing album name");
  else
    try
    {
      CMusicAlbumInfo musicInfo;//("", "") ;
      XFILE::CCurlFile http;
      ScraperPtr info; // TODO - WTF is this code supposed to do?
      if (musicInfo.Load(http,info))
      {
        if (musicInfo.GetAlbum().thumbURL.HasUrls())
          output=openTag+"image:" + musicInfo.GetAlbum().thumbURL.GetFirstThumbUrl().c_str();

        output+=closeTag+openTag+"review:" + musicInfo.GetAlbum().strReview.c_str();
        return SetResponse(output) ;
      }
      else
        return SetResponse(openTag+"Error:Loading musinInfo");
    }
    catch (...)
    {
      return SetResponse(openTag+"Error:Exception");
    }
}

int CXbmcHttp::xbmcDownloadInternetFile(int numParas, std::string paras[])
{
  std::string src, dest="";

  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    src=paras[0];
    if (numParas>1)
      dest=paras[1];
    if (dest=="")
      dest="special://temp/xbmcDownloadInternetFile.tmp" ;
    if (src=="")
      return SetResponse(openTag+"Error:Missing parameter");
    else
    {
      try
      {
        if (numParas>1)
        {
          StringUtils::ToLower(paras[1]);
          tempSkipWebFooterHeader=paras[1] == "bare";
        }
        if (numParas>2)
        {
          StringUtils::ToLower(paras[2]);
          tempSkipWebFooterHeader=paras[2] == "bare";
        }
        XFILE::CCurlFile http;
        http.Download(src, dest);
        std::string encoded="";
        encoded=encodeFileToBase64(dest, 80);
        if (encoded=="")
          return SetResponse(openTag+"Error:Nothing downloaded");
        {
          if (dest=="special://temp/xbmcDownloadInternetFile.tmp")
            CFile::Delete(dest);
          if (tempSkipWebFooterHeader)
          {
            std::string strHttpResponseHeaders = StringUtils::Format(
            "HTTP/1.0 200 OK\r\n"
            "Pragma: no-cache\r\n"
            "Cache-control: no-cache\r\n"
            "Content-Length: %i\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            ,encoded.length()
            );
            return SetResponse( strHttpResponseHeaders + encoded);
          }
          return SetResponse(encoded) ;
        }
      }
      catch (...)
      {
        return SetResponse(openTag+"Error:Exception");
      }
    }
  }
}

int CXbmcHttp::xbmcSetFile(int numParas, std::string paras[])
//parameter = destFilename ; base64String ; ( first | continue | last )
{
  if (numParas<2)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    StringUtils::Replace(paras[1], " ","+");
    std::string tmpFile = "special://temp/xbmcTemp.tmp";
    if (numParas>2)
    {
      StringUtils::ToLower(paras[2]);
      if (paras[2] == "first")
        decodeBase64ToFile(paras[1], tmpFile);
      else if (paras[2] == "continue")
        decodeBase64ToFile(paras[1], tmpFile, true);
      else if (paras[2] == "last")
      {
        decodeBase64ToFile(paras[1], tmpFile, true);
        CFile::Copy(tmpFile, paras[0].c_str(), NULL, NULL) ;
        CFile::Delete(tmpFile);
      }
      else
        return  SetResponse(openTag+"Error:Unknown 2nd parameter");
    }
    else
    {
      decodeBase64ToFile(paras[1], tmpFile);
      CFile::Copy(tmpFile, paras[0].c_str(), NULL, NULL) ;
      CFile::Delete(tmpFile);
    }
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcCopyFile(int numParas, std::string paras[])
//parameter = srcFilename ; destFilename
// both file names are relative to the XBox not the calling client
{
  if (numParas<2)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    if (CFile::Exists(paras[0].c_str()))
    {
      CFile::Copy(paras[0].c_str(), paras[1].c_str(), NULL, NULL) ;
      return SetResponse(openTag+"OK");
    }
    else
      return SetResponse(openTag+"Error:Source file not found");
  }
}


int CXbmcHttp::xbmcFileSize(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    __int64 filesize=fileSize(paras[0]);
    if (filesize>-1)
    {
      std::string tmp;
      tmp = StringUtils::Format("%"PRId64,filesize);
      return SetResponse(openTag+tmp);
    }
    else
      return SetResponse(openTag+"Error:Source file not found");
  }
}

int CXbmcHttp::xbmcDeleteFile(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    try
    {
      if (CFile::Exists(paras[0]))
      {
        CFile::Delete(paras[0]);
        return SetResponse(openTag+"OK");
      }
      else
        return SetResponse(openTag+"Error:File not found");
    }
    catch (...)
    {
      return SetResponse(openTag+"Error");
    }
  }
}

int CXbmcHttp::xbmcFileExists(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    try
    {
      if (CFile::Exists(paras[0]))
      {
        return SetResponse(openTag+"True");
      }
      else
        return SetResponse(openTag+"False");
    }
    catch (...)
    {
      return SetResponse(openTag+"Error");
    }
  }
}

int CXbmcHttp::xbmcShowPicture(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    if (!playableFile(paras[0]))
      return SetResponse(openTag+"Error:Unable to open file");
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PICTURE_SHOW, -1, -1, NULL, paras[0]);
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcGetCurrentSlide()
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return SetResponse(openTag+"Error:Could not access slideshown");
  else
  {
    const boost::shared_ptr<const CFileItem>& slide=pSlideShow->GetCurrentSlide();
    if (!slide)
      return SetResponse(openTag + "[None]");
    return SetResponse(openTag + slide->GetPath().c_str());
  }
}

int CXbmcHttp::xbmcExecBuiltIn(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, NULL, paras[0]);
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcGUISetting(int numParas, std::string paras[])
//parameter=type;name(;value)
//type=0->int, 1->bool, 2->float, 3->string
{
  if (numParas<2)
    return SetResponse(openTag+"Error:Missing parameters");
  else
  {
    std::string tmp;
    if (numParas<3)
      switch (atoi(paras[0].c_str()))
      {
        case 0:  //  int
          tmp = StringUtils::Format("%i", CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(paras[1]));
          return SetResponse(openTag + tmp );
          break;
        case 1: // bool
          if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(paras[1])==0)
            return SetResponse(openTag+"False");
          else
            return SetResponse(openTag+"True");
          break;
        case 2: // float
          tmp = StringUtils::Format("%f", CServiceBroker::GetSettingsComponent()->GetSettings()->GetNumber(paras[1]));
          return SetResponse(openTag + tmp);
          break;
        case 3: // string
          tmp = StringUtils::Format("%s", CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(paras[1]).c_str());
          return SetResponse(openTag + tmp);
          break;
        default:
          return SetResponse(openTag+"Error:Unknown type");
          break;
      }
    else
    {
      switch (atoi(paras[0].c_str()))
      {
        case 0:  //  int
          CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt(paras[1], atoi(paras[2].c_str()));
          return SetResponse(openTag+"OK");
          break;
        case 1: // bool
          StringUtils::ToLower(paras[2]);
          CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(paras[1], (paras[2]=="true"));
          return SetResponse(openTag+"OK");
          break;
        case 2: // float
          CServiceBroker::GetSettingsComponent()->GetSettings()->SetNumber(paras[1], (double)atof(paras[2].c_str()));
          return SetResponse(openTag+"OK");
          break;
        case 3: // string
          CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(paras[1], paras[2]);
          return SetResponse(openTag+"OK");
          break;
        default:
          return SetResponse(openTag+"Error:Unknown type");
          break;
      }
    }
  }
  return 0; // not reached
}

int CXbmcHttp::xbmcSTSetting(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameters");
  else
  {
    std::string tmp;
    std::string strInfo = "";
    int i;
    for (i=0; i<numParas; i++)
    {
      if (paras[i]=="myvideowatchmode")
      {
        CGUIWindow *window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_VIDEO_NAV);
        int watchMode = (window) ? CMediaSettings::GetInstance().GetWatchedMode(((CGUIMediaWindow *)window)->CurrentDirectory().GetContent()) : WatchedModeAll;
        tmp = StringUtils::Format("%i", watchMode);
      }
      else if (paras[i]=="mymusicstartwindow")
        tmp = StringUtils::Format("%i",CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("mymusic.startwindow"));
      else if (paras[i]=="videostartwindow")
        tmp = StringUtils::Format("%i",CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("myvideos.startwindow"));
      else if (paras[i]=="myvideostack")
        tmp = StringUtils::Format("%i",CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("myvideos.stackvideos") ? 1 : 0);
      else if (paras[i]=="additionalsubtitledirectorychecked")
        tmp = StringUtils::Format("%i",CMediaSettings::GetInstance().GetAdditionalSubtitleDirectoryChecked());
      else if (paras[i]=="httpapibroadcastport")
        tmp = StringUtils::Format("%i",CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("services.httpapibroadcastport"));
      else if (paras[i]=="httpapibroadcastlevel")
        tmp = StringUtils::Format("%i",CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("services.httpapibroadcastlevel"));
      else if (paras[i]=="volumelevel")
      {
        const CApplicationComponents &components = CServiceBroker::GetAppComponents();
        const boost::shared_ptr<const CApplicationVolumeHandling> appVolume = components.GetComponent<CApplicationVolumeHandling>();
        tmp = StringUtils::Format("%i",appVolume->GetVolumeRatio());
      }
      else if (paras[i]=="systemtimetotalup")
        tmp = StringUtils::Format("%i",g_sysinfo.GetTotalUptime());
      else if (paras[i]=="mute")
      {
        const CApplicationComponents &components = CServiceBroker::GetAppComponents();
        const boost::shared_ptr<const CApplicationVolumeHandling> appVolume = components.GetComponent<CApplicationVolumeHandling>();
        tmp = !appVolume->IsMuted() ? "False" : "True";
      }
      else if (paras[i]=="myvideonavflatten")
        tmp = (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("myvideos.flatten")==0) ? "False" : "True";
      else if (paras[i]=="zoomamount")
        tmp = StringUtils::Format("%f", CDisplaySettings::GetInstance().GetZoomAmount());
      else if (paras[i]=="pixelratio")
        tmp = StringUtils::Format("%f", CDisplaySettings::GetInstance().GetPixelRatio());
      else if (paras[i]=="pictureextensions")
        tmp = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pictureExtensions;
      else if (paras[i]=="musicextensions")
        tmp = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicExtensions;
      else if (paras[i]=="videoextensions")
        tmp = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoExtensions;
      else if (paras[i]=="logfolder")
        tmp = CSpecialProtocol::TranslatePath("special://logpath");
      else
        tmp = "Error:Unknown setting " + paras[i];
      strInfo += openTag + tmp;
    }
    return SetResponse(strInfo);
  }
  return 0; // not reached
}

int CXbmcHttp::xbmcConfig(int numParas, std::string paras[])
{
  int argc=0, ret=-1;
  char_t* argv[20];
  std::string response="";

  if (numParas<1) {
    return SetResponse(openTag+"Error:Missing paramters");
  }
  if (numParas>1){
    for (argc=0; argc<numParas-1;argc++)
      argv[argc]=(char_t*)paras[argc+1].c_str();
  }
  argv[argc]=NULL;
  bool createdWebConfigObj=XbmcWebConfigInit();
  if (paras[0]=="bookmarksize")
  {
    ret=XbmcWebsHttpAPIConfigBookmarkSize(response, argc, argv);
    if (ret!=-1)
      ret=1;
  }
  else if (paras[0]=="getbookmark")
  {
    ret=XbmcWebsHttpAPIConfigGetBookmark(response, argc, argv);
    if (ret!=-1)
      ret=1;
  }
  else if (paras[0]=="addbookmark")
    ret=XbmcWebsHttpAPIConfigAddBookmark(response, argc, argv);
  else if (paras[0]=="savebookmark")
    ret=XbmcWebsHttpAPIConfigSaveBookmark(response, argc, argv);
  else if (paras[0]=="removebookmark")
    ret=XbmcWebsHttpAPIConfigRemoveBookmark(response, argc, argv);
  else if (paras[0]=="saveconfiguration")
    ret=XbmcWebsHttpAPIConfigSaveConfiguration(response, argc, argv);
  else if (paras[0]=="getoption")
  {
    //getoption has been deprecated so the following is just to prevent (my) legacy client code breaking (to be removed later)
    if (paras[1]=="pictureextensions")
      response=openTag+CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pictureExtensions.c_str();
    else if (paras[1]=="videoextensions")
      response=openTag+CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoExtensions.c_str();
    else if (paras[1]=="musicextensions")
      response=openTag+CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicExtensions.c_str();
    else
      response=openTag+"Error:Function is deprecated";
    //ret=XbmcWebsHttpAPIConfigGetOption(response, argc, argv);
    //if (ret!=-1)
    ret=1;
  }
  else if (paras[0]=="setoption")
    ret=XbmcWebsHttpAPIConfigSetOption(response, argc, argv);
  else
  {
    return SetResponse(openTag+"Error:Unknown Config Command");
  }
  if (createdWebConfigObj)
    XbmcWebConfigRelease();
  if (ret==-1)
    return SetResponse(openTag+"Error:WebServer needs to be running - is it?");
  else
  {
    return SetResponse(openTag+response);
  }
}

int CXbmcHttp::xbmcGetSystemInfo(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing Parameter");
  else
  {
    std::string strInfo = "";
    int i;
    for (i=0; i<numParas; i++)
    {
      std::string strTemp = (std::string) CServiceBroker::GetGUI()->GetInfoManager().GetLabel(atoi(paras[i].c_str()), INFO::DEFAULT_CONTEXT);
      if (strTemp.empty())
        strTemp = "Error:No information retrieved for " + paras[i];
      strInfo += openTag + strTemp;
    }
    return SetResponse(strInfo);
  }
}

int CXbmcHttp::xbmcGetSystemInfoByName(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing Parameter");
  else
  {
    std::string strInfo = "";
    int i;
    for (i=0; i<numParas; i++)
    {
      std::string strTemp = (std::string) CServiceBroker::GetGUI()->GetInfoManager().GetLabel(CServiceBroker::GetGUI()->GetInfoManager().TranslateString(paras[i]), INFO::DEFAULT_CONTEXT);
      if (strTemp.empty())
        strTemp = "Error:No information retrieved for " + paras[i];
      strInfo += openTag + strTemp;
    }
    if(strInfo.find("°") && strInfo.find("Â"))
    {
      // The Charset Converter ToUtf8() will add. only in this case= "°" a char "Â°" during converting,
      // which is the right value for the GUI!
      // A length depending fix in CCharsetConverter::stringCharsetToUtf8() will couse a wrong char in GUI.
      // So just for http, we remove the "Â", to fix BUG ID:[1586251]
      StringUtils::Replace(strInfo, "Â","");
    }
    return SetResponse(strInfo);
  }
}

int CXbmcHttp::xbmcSpinDownHardDisk(int numParas, std::string paras[])
{
  return SetResponse(openTag+"Error:Not supported!");
}

bool CXbmcHttp::xbmcBroadcast(std::string message, int level)
{
  if  (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("services.httpapibroadcastlevel")>=level)
  {
    if (!pUdpBroadcast)
      pUdpBroadcast = new CUdpBroadcast();
    std::string msg = StringUtils::Format("%s%s;%i%s", openBroadcast.c_str(), message.c_str(), level, closeBroadcast.c_str());
    return pUdpBroadcast->broadcast(msg, CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("services.httpapibroadcastport"));
  }
  else
    return true;
}

int CXbmcHttp::xbmcBroadcast(int numParas, std::string paras[])
{
  if (numParas>0)
  {
    if (!pUdpBroadcast)
      pUdpBroadcast = new CUdpBroadcast();
    bool succ;
    if (numParas>1)
      succ=pUdpBroadcast->broadcast(paras[0], atoi(paras[1].c_str()));
    else
      succ=pUdpBroadcast->broadcast(paras[0], CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("services.httpapibroadcastport"));
    if (succ)
      return SetResponse(openTag+"OK");
    else
      return SetResponse(openTag+"Error: calling broadcast");
  }
  else
    return SetResponse(openTag+"Error:Wrong number of parameters");
}

int CXbmcHttp::xbmcSetBroadcast(int numParas, std::string paras[])
{
  if (numParas>0)
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt("services.httpapibroadcastlevel", atoi(paras[0].c_str()));
    if (numParas>1)
      CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt("services.httpapibroadcastport", atoi(paras[1].c_str()));
    return SetResponse(openTag+"OK");
  }
  else
    return SetResponse(openTag+"Error:Wrong number of parameters");
}

int CXbmcHttp::xbmcGetBroadcast()
{
  std::string tmp;
  tmp = StringUtils::Format("%i;%i", CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("services.httpapibroadcastlevel"),CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("services.httpapibroadcastport"));
  return SetResponse(openTag+tmp);
}

int CXbmcHttp::xbmcGetSkinSetting(int numParas, std::string paras[])
//parameter=type;name
//type: 0=bool, 1=string
{
  if (numParas<2)
    return SetResponse(openTag+"Error:Missing parameters");
  else
  {
    if (atoi(paras[0].c_str()) == 0)
    {
      int string = CSkinSettings::GetInstance().TranslateBool(paras[1]);
      bool value = CSkinSettings::GetInstance().GetBool(string);
      if (value==false)
        return SetResponse(openTag+"False");
      else
        return SetResponse(openTag+"True");
    }
    else
    {
      int string = CSkinSettings::GetInstance().TranslateString(paras[1]);
      std::string value = CSkinSettings::GetInstance().GetString(string);
      return SetResponse(openTag+value);
    }
  }
}

int CXbmcHttp::xbmcTakeScreenshot(int numParas, std::string paras[])
//no paras
//filename, flash, rotation, width, height, quality
//filename, flash, rotation, width, height, quality, download
//filename, flash, rotation, width, height, quality, download, imgtag
//filename can be blank
{
  if (numParas<1)
    CUtil::TakeScreenshot();
  else
    return SetResponse(openTag+"Error: xbmcTakeScreenshot with params depracated");
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcAutoGetPictureThumbs(int numParas, std::string paras[])
{
  if (numParas<1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    StringUtils::ToLower(paras[0]);
    autoGetPictureThumbs = (paras[0]=="true");
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcOnAction(int numParas, std::string paras[])
{
  if (numParas!=1)
    return SetResponse(openTag+"Error:There must be one and only one parameter");
  g_application.OnAction(CAction(atoi(paras[0].c_str())));
  return SetResponse(openTag+"OK");
}

int CXbmcHttp::xbmcRecordStatus(int numParas, std::string paras[])
{
  if (numParas!=0)
    return SetResponse(openTag+"Error:Too many parameters");

  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
  if( appPlayer->IsPlaying() && appPlayer->CanRecord())
    return SetResponse(appPlayer->IsRecording()?openTag+"Recording":openTag+"Not recording");
  else
    return SetResponse(openTag+"Can't record");
}

int CXbmcHttp::xbmcGetLogLevel()
{
  std::string level;
  level = StringUtils::Format("%i", CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel);
  return SetResponse(openTag+level);
}

int CXbmcHttp::xbmcSetLogLevel(int numParas, std::string paras[])
{
  if (numParas!=1)
    return SetResponse(openTag+"Error:Must have one parameter");
  else
  {
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel=atoi(paras[0].c_str());
    return SetResponse(openTag+"OK");
  }
}

int CXbmcHttp::xbmcWebServerStatus(int numParas, std::string paras[])
{
  if (numParas==0)
  {
    if (CNetworkServices::GetInstance().IsWebserverRunning())
      return SetResponse(openTag+"On");
    else
      return SetResponse(openTag+"Off");
  }

  StringUtils::ToLower(paras[0]);
  if (paras[0] == "on")
  {
    if (CNetworkServices::GetInstance().IsWebserverRunning())
      return SetResponse(openTag+"Already on");
    else
    {
      //g_application.StartWebServer();
      return SetResponse(openTag+"OK");
    }
  }
  else
  {
    if (paras[0] == "off")
      if (!CNetworkServices::GetInstance().IsWebserverRunning())
        return SetResponse(openTag+"Already off");
      else
      {
        //g_application.StopWebServer();
        return SetResponse(openTag+"OK");
      }
    else
        return SetResponse(openTag+"Error:Unknown parameter");
  }
}

int CXbmcHttp::xbmcSetResponseFormat(int numParas, std::string paras[])
{
  if (numParas==0)
  {
    resetTags();
    return SetResponse(openTag+"OK");
  }
  else if ((numParas % 2)==1)
    return SetResponse(openTag+"Error:Missing parameter");
  else
  {
    std::string para;
    for (int i=0; i<numParas; i+=2)
    {
      para=paras[i];
      StringUtils::ToLower(para);
      if (para=="webheader")
      {
        StringUtils::ToLower(paras[i+1]);
        incWebHeader=(paras[i+1]=="true");
      }
      else if (para=="webfooter")
      {
        StringUtils::ToLower(paras[i+1]);
        incWebFooter=(paras[i+1]=="true");
      }
      else if (para=="header")
        userHeader=paras[i+1];
      else if (para=="footer")
        userFooter=paras[i+1];
      else if (para=="opentag")
        openTag=paras[i+1];
      else if (para=="closetag")
        closeTag=paras[i+1];
      else if (para=="closefinaltag")
      {
        StringUtils::ToLower(paras[i+1]);
        closeFinalTag=(paras[i+1]=="true");
      }
      else if (para=="openrecordset")
        openRecordSet=paras[i+1];
      else if (para=="closerecordset")
        closeRecordSet=paras[i+1];
      else if (para=="openrecord")
        openRecord=paras[i+1];
      else if (para=="closerecord")
        closeRecord=paras[i+1];
      else if (para=="openfield")
        openField=paras[i+1];
      else if (para=="closefield")
        closeField=paras[i+1];
      else if (para=="openbroadcast")
        openBroadcast=paras[i+1];
      else if (para=="closebroadcast")
        closeBroadcast=paras[i+1];
      else
        return SetResponse(openTag+"Error:Unknown parameter:"+para);
    }
    return SetResponse(openTag+"OK");
  }
}


int CXbmcHttp::xbmcHelp()
{
  std::string output;
  output="<p><b>XBMC HTTP API Commands</b></p><p>There are two alternative but equivalent syntax forms:</p>";
  output+="<p><b>Syntax 1: http://xbox/xbmcCmds/xbmcHttp?command=</b>command<b>&ampparameter=</b>first_parameter<b>;</b>second_parameter<b>;...</b></p>";
  output+="<p><b>Syntax 2: http://xbox/xbmcCmds/xbmcHttp?command=</b>command<b>(</b>first_parameter<b>;</b>second_parameter<b>;...</b><b>)</b></p>";
  output+="<p>Note the use of the semi colon to separate multiple parameters.</p><p>The commands are case insensitive.</p>";
  output+= "<p>The full documentation can be found here: <a  href=\"http://xbmc.org/wiki/index.php?title=WebServerHTTP-API\">http://xbmc.org/wiki/index.php?title=WebServerHTTP-API</a></p>";
  return SetResponse(output);
}


int CXbmcHttp::xbmcCommand(const std::string &parameter)
{
  if (shuttingDown)
    return -1;
  int numParas, retVal=false;
  std::string command, paras[MAX_PARAS];
  numParas = splitParameter(parameter, command, paras, ";");
  if (parameter.length()<300)
    CLog::Log(LOGDEBUG, "HttpApi Start command: %s  paras: %s", command.c_str(), parameter.c_str());
  else
    CLog::Log(LOGDEBUG, "HttpApi Start command: %s  paras: [not recorded]", command.c_str());
  tempSkipWebFooterHeader=false;
  StringUtils::ToLower(command);
  if (numParas>=0)
  {
    if (command == "clearplaylist")                   retVal = xbmcClearPlayList(numParas, paras);
      else if (command == "addtoplaylist")            retVal = xbmcAddToPlayList(numParas, paras);
      else if (command == "addtoplaylistfromdb")      retVal = xbmcAddToPlayListFromDB(numParas, paras);
      else if (command == "swapplaylistitems")        retVal = xbmcSwapPlayListItems(numParas, paras);
      else if (command == "playfile")                 retVal = xbmcPlayerPlayFile(numParas, paras);
      else if (command == "pause")                    retVal = xbmcAction(numParas, paras,1);
      else if (command == "stop")                     retVal = xbmcAction(numParas, paras,2);
      else if (command == "playnext")                 retVal = xbmcAction(numParas, paras,3);
      else if (command == "playprev")                 retVal = xbmcAction(numParas, paras,4);
      else if (command == "rotate")                   retVal = xbmcAction(numParas, paras,5);
      else if (command == "move")                     retVal = xbmcAction(numParas, paras,6);
      else if (command == "zoom")                     retVal = xbmcAction(numParas, paras,7);
      else if (command == "restart")                  retVal = xbmcExit(1);
      else if (command == "shutdown")                 retVal = xbmcExit(2);
      else if (command == "exit")                     retVal = xbmcExit(3);
      else if (command == "reset")                    retVal = xbmcExit(4);
      else if (command == "restartapp")               retVal = xbmcExit(5);
      else if (command == "getcurrentlyplaying")      retVal = xbmcGetCurrentlyPlaying(numParas, paras);
      else if (command == "getxbeid")                 retVal = xbmcGetXBEID(numParas, paras);
      else if (command == "getxbetitle")              retVal = xbmcGetXBETitle(numParas, paras);
      else if (command == "getshares")                retVal = xbmcGetSources(numParas, paras);
      else if (command == "getdirectory")             retVal = xbmcGetDirectory(numParas, paras);
      else if (command == "getmedialocation")         retVal = xbmcGetMediaLocation(numParas, paras);
      else if (command == "gettagfromfilename")       retVal = xbmcGetTagFromFilename(numParas, paras);
      else if (command == "getcurrentplaylist")       retVal = xbmcGetCurrentPlayList();
      else if (command == "setcurrentplaylist")       retVal = xbmcSetCurrentPlayList(numParas, paras);
      else if (command == "getplaylistcontents")      retVal = xbmcGetPlayListContents(numParas, paras);
      else if (command == "getplaylistlength")        retVal = xbmcGetPlayListLength(numParas, paras);
      else if (command == "removefromplaylist")       retVal = xbmcRemoveFromPlayList(numParas, paras);
      else if (command == "setplaylistsong")          retVal = xbmcSetPlayListSong(numParas, paras);
      else if (command == "getplaylistsong")          retVal = xbmcGetPlayListSong(numParas, paras);
      else if (command == "playlistnext")             retVal = xbmcPlayListNext();
      else if (command == "playlistprev")             retVal = xbmcPlayListPrev();
      else if (command == "getmusiclabel")            retVal = xbmcGetMusicLabel(numParas, paras);
      else if (command == "getvideolabel")            retVal = xbmcGetVideoLabel(numParas, paras);
      else if (command == "getpercentage")            retVal = xbmcGetPercentage();
      else if (command == "seekpercentage")           retVal = xbmcSeekPercentage(numParas, paras, false);
      else if (command == "seekpercentagerelative")   retVal = xbmcSeekPercentage(numParas, paras, true);
      else if (command == "setvolume")                retVal = xbmcSetVolume(numParas, paras);
      else if (command == "getvolume")                retVal = xbmcGetVolume();
      else if (command == "mute")                     retVal = xbmcMute();
      else if (command == "setplayspeed")             retVal = xbmcSetPlaySpeed(numParas, paras);
      else if (command == "getplayspeed")             retVal = xbmcGetPlaySpeed();
      else if (command == "filedownload")             retVal = xbmcGetThumb(numParas, paras, false);
      else if (command == "getthumbfilename")         retVal = xbmcGetThumbFilename(numParas, paras);
      else if (command == "lookupalbum")              retVal = xbmcLookupAlbum(numParas, paras);
      else if (command == "choosealbum")              retVal = xbmcChooseAlbum(numParas, paras);
      else if (command == "filedownloadfrominternet") retVal = xbmcDownloadInternetFile(numParas, paras);
      else if (command == "filedelete")               retVal = xbmcDeleteFile(numParas, paras);
      else if (command == "filecopy")                 retVal = xbmcCopyFile(numParas, paras);
      else if (command == "filesize")                 retVal = xbmcFileSize(numParas, paras);
      else if (command == "getmoviedetails")          retVal = xbmcGetMovieDetails(numParas, paras);
      else if (command == "showpicture")              retVal = xbmcShowPicture(numParas, paras);
      else if (command == "sendkey")                  retVal = xbmcSetKey(numParas, paras);
      else if (command == "keyrepeat")                retVal = xbmcSetKeyRepeat(numParas, paras);
      else if (command == "fileexists")               retVal = xbmcFileExists(numParas, paras);
      else if (command == "fileupload")               retVal = xbmcSetFile(numParas, paras);
      else if (command == "getguistatus")             retVal = xbmcGetGUIStatus();
      else if (command == "execbuiltin")              retVal = xbmcExecBuiltIn(numParas, paras);
      else if (command == "config")                   retVal = xbmcConfig(numParas, paras);
      else if (command == "help")                     retVal = xbmcHelp();
      else if (command == "getsysteminfo")            retVal = xbmcGetSystemInfo(numParas, paras);
      else if (command == "getsysteminfobyname")      retVal = xbmcGetSystemInfoByName(numParas, paras);
      else if (command == "addtoslideshow")           retVal = xbmcAddToSlideshow(numParas, paras);
      else if (command == "clearslideshow")           retVal = xbmcClearSlideshow();
      else if (command == "playslideshow")            retVal = xbmcPlaySlideshow(numParas, paras);
      else if (command == "getslideshowcontents")     retVal = xbmcGetSlideshowContents();
      else if (command == "slideshowselect")          retVal = xbmcSlideshowSelect(numParas, paras);
      else if (command == "getcurrentslide")          retVal = xbmcGetCurrentSlide();
      else if (command == "getguisetting")            retVal = xbmcGUISetting(numParas, paras);
      else if (command == "setguisetting")            retVal = xbmcGUISetting(numParas, paras);
      else if (command == "takescreenshot")           retVal = xbmcTakeScreenshot(numParas, paras);
      else if (command == "getguidescription")        retVal = xbmcGetGUIDescription();
      else if (command == "setautogetpicturethumbs")  retVal = xbmcAutoGetPictureThumbs(numParas, paras);
      else if (command == "setresponseformat")        retVal = xbmcSetResponseFormat(numParas, paras);
      else if (command == "querymusicdatabase")       retVal = xbmcQueryMusicDataBase(numParas, paras);
      else if (command == "queryvideodatabase")       retVal = xbmcQueryVideoDataBase(numParas, paras);
      else if (command == "queryprogramdatabase")     retVal = xbmcQueryProgramDataBase(numParas, paras);
      else if (command == "execmusicdatabase")        retVal = xbmcExecMusicDataBase(numParas, paras);
      else if (command == "execvideodatabase")        retVal = xbmcExecVideoDataBase(numParas, paras);
      else if (command == "spindownharddisk")         retVal = xbmcSpinDownHardDisk(numParas, paras);
      else if (command == "broadcast")                retVal = xbmcBroadcast(numParas, paras);
      else if (command == "setbroadcast")             retVal = xbmcSetBroadcast(numParas, paras);
      else if (command == "getbroadcast")             retVal = xbmcGetBroadcast();
      else if (command == "action")                   retVal = xbmcOnAction(numParas, paras);
      else if (command == "getrecordstatus")          retVal = xbmcRecordStatus(numParas, paras);
      else if (command == "webserverstatus")          retVal = xbmcWebServerStatus(numParas, paras);
      else if (command == "setloglevel")              retVal = xbmcSetLogLevel(numParas, paras);
      else if (command == "getloglevel")              retVal = xbmcGetLogLevel();

      //only callable internally
      else if (command == "broadcastlevel")
      {
        retVal = xbmcBroadcast(paras[0], atoi(paras[1].c_str()));
        retVal = 0;
      }

      //Old command names
      else if (command == "deletefile")               retVal = xbmcDeleteFile(numParas, paras);
      else if (command == "copyfile")                 retVal = xbmcCopyFile(numParas, paras);
      else if (command == "downloadinternetfile")     retVal = xbmcDownloadInternetFile(numParas, paras);
      else if (command == "getthumb")                 retVal = xbmcGetThumb(numParas, paras, true);
      else if (command == "guisetting")               retVal = xbmcGUISetting(numParas, paras);
      else if (command == "setfile")                  retVal = xbmcSetFile(numParas, paras);
      else if (command == "setkey")                   retVal = xbmcSetKey(numParas, paras);

      else
        retVal = SetResponse(openTag+"Error:Unknown command");

  }
  else if (numParas==-2)
    retVal = SetResponse(openTag+"Error:Too many parameters");
  else
    retVal = SetResponse(openTag+"Error:Missing command");
//relinquish the remainder of time slice
  Sleep(0);
  //CLog::Log(LOGDEBUG, "HttpApi Finished command: %s", command.c_str());
  return retVal;
}

CXbmcHttpShim::CXbmcHttpShim()
{
  CLog::Log(LOGDEBUG, "xbmcHttpShim starts");
}

CXbmcHttpShim::~CXbmcHttpShim()
{
CLog::Log(LOGDEBUG, "xbmcHttpShim ends");
}

bool CXbmcHttpShim::checkForFunctionTypeParas(std::string &cmd, std::string &paras)
{
  int open, close;
  open = cmd.find("(");
  if (open>0)
  {
    close=cmd.length();
    while (close>open && cmd.substr(close,1)!=")")
      close--;
    if (close>open)
    {
      paras = cmd.substr(open + 1, close - open - 1);
      cmd = cmd.substr(0, open);
      return (close-open)>1;
    }
  }
  return false;
}

std::string CXbmcHttpShim::flushResult(int eid, webs_t wp, const std::string &output)
{
  if (output!="")
  {
    if (eid==NO_EID && wp!=NULL)
      websWriteBlock(wp, (char_t *) output.c_str(), output.length()) ;
    else if (eid!=NO_EID)
      ejSetResult( eid, (char_t *)output.c_str());
    else
      return output;
  }
  return "";
}

std::string CXbmcHttpShim::xbmcExternalCall(char *command)
{
  if (m_pXbmcHttp && m_pXbmcHttp->shuttingDown)
      return "";
  int open, close;
  std::string parameter="", cmd=command, execute;
  open = cmd.find("(");
  if (open>0)
  {
    close=cmd.length();
    while (close>open && cmd.substr(close,1)!=")")
      close--;
    if (close>open)
    {
      parameter = cmd.substr(open + 1, close - open - 1);
      StringUtils::Replace(parameter, ",",";");
      execute = cmd.substr(0, open);
    }
    else //open bracket but no close
      return "";
  }
  else //no parameters
    execute = cmd;
  CURL::Decode(parameter);
  return xbmcProcessCommand(NO_EID, NULL, (char_t *) execute.c_str(), (char_t *) parameter.c_str());
}

/* Parse an XBMC HTTP API command */
std::string CXbmcHttpShim::xbmcProcessCommand( int eid, webs_t wp, char_t *command, char_t *parameter)
{
  if (m_pXbmcHttp && m_pXbmcHttp->shuttingDown)
    return "";
  std::string cmd=command, paras=parameter, response="[No response yet]", retVal;
  bool legalCmd=true;
  //CLog::Log(LOGDEBUG, "XBMCHTTPShim: Received command %s (%s)", cmd.c_str(), paras.c_str());
  int cnt=0;

  checkForFunctionTypeParas(cmd, paras);
  if (wp!=NULL)
  {
    //we are being called via the webserver (rather than Python) so add any specific checks here
    if ((cmd=="webserverstatus") && (paras!=""))//(strcmp(parameter,XBMC_NONE)))
    {
      response=m_pXbmcHttp->GetOpenTag()+"Error:Can't turn off/on WebServer via a web call";
      legalCmd=false;
    }
  }
  if (legalCmd)
  {
    if (paras!="")
      HttpApi(cmd+"; "+paras, true);
    else
      HttpApi(cmd, true);
    //wait for response - max 20s
    Sleep(0);
    response=GetResponse();
    while (response=="[No response yet]" && cnt++<200)
    {
      response=GetResponse();
      CLog::Log(LOGDEBUG, "XBMCHTTPShim: waiting %d", cnt);
      Sleep(100);
    }
    if (cnt>199)
    {
      response=m_pXbmcHttp->GetOpenTag()+"Error:Timed out";
      CLog::Log(LOGDEBUG, "HttpApi Timed out");
    }
  }
  //flushresult
  if (wp!=NULL)
  {
    if (eid==NO_EID && m_pXbmcHttp && !m_pXbmcHttp->tempSkipWebFooterHeader)
    {
      if (m_pXbmcHttp->incWebHeader)
          websHeader(wp);
    }
  }
  retVal=flushResult(eid, wp, m_pXbmcHttp->userHeader+response+m_pXbmcHttp->userFooter);
  if (m_pXbmcHttp) //this should always be true unless something is very wrong
    if ((wp!=NULL) && (m_pXbmcHttp->incWebFooter) && eid==NO_EID && !m_pXbmcHttp->tempSkipWebFooterHeader)
      websFooter(wp);
  return retVal;
}


/* XBMC Javascript binding for ASP. This will be invoked when "APICommand" is
 *  embedded in an ASP page.
 */
int CXbmcHttpShim::xbmcCommand( int eid, webs_t wp, int argc, char_t **argv)
{
  char_t *command, *parameter;
  if (m_pXbmcHttp && m_pXbmcHttp->shuttingDown)
    return -1;

  int parameters = ejArgs(argc, argv, T("%s %s"), &command, &parameter);
  if (parameters < 1)
  {
    websError(wp, 500, T("Error:Insufficient args"));
    return -1;
  }
  else if (parameters < 2)
    parameter = (char*)"";
  xbmcProcessCommand( eid, wp, command, parameter);
  return 0;
}

/* XBMC form for posted data (in-memory CGI).
 */
void CXbmcHttpShim::xbmcForm(webs_t wp, char_t *path, char_t *query)
{
  char_t  *command, *parameter;

  if (m_pXbmcHttp && m_pXbmcHttp->shuttingDown)
      return;
  command = websGetVar(wp, WEB_COMMAND, "");
  parameter = websGetVar(wp, WEB_PARAMETER, "");

  // do the command

  xbmcProcessCommand( NO_EID, wp, command, parameter);

  if (wp->timeout!=-1)
    websDone(wp, 200);
  else
    CLog::Log(LOGERROR, "HttpApi Timeout command: %s", query);
}

void CXbmcHttpShim::HttpApi(std::string cmd, bool wait)
{
  SetResponseInternal("");
  if (wait)
    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_HTTPAPI, -1, -1, NULL, cmd);
  else
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_HTTPAPI, -1, -1, NULL, cmd);
}
