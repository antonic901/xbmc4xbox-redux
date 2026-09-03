/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "network/Network.h"
#include "system.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationXbox.h"
#include "AutoPtrHandle.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "Util.h"
#include "storage/DetectDVDType.h"
#include "Autorun.h"
#include "filesystem/HDDirectory.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/SpecialProtocol.h"
#include "FileSystem/RSSDirectory.h"
#include "filesystem/ZipManager.h"
#include "filesystem/RarManager.h"
#include "filesystem/VideoDatabaseDirectory.h"
#ifdef HAS_UPNP
#include "filesystem/UPnPDirectory.h"
#endif
#include "Shortcut.h"
#include "PlayListPlayer.h"
#include "PartyModeManager.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "interfaces/python/XBPython.h"
#include "profiles/ProfileManager.h"
#include "utils/RegExp.h"
#include "utils/AlarmClock.h"
#include "input/ButtonTranslator.h"
#include "pictures/Picture.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/TextureManager.h"
#include "guilib/IGUIContainer.h"
#include "utils/fstrcmp.h"
#include "storage/MediaManager.h"
#ifdef _XBOX
#include <xbdm.h>
#endif
#include "network/Network.h"
#include "GUIPassword.h"
#ifdef HAS_FTP_SERVER
#include "libfilezilla/xbfilezilla.h"
#endif
#include "music/MusicInfoLoader.h"
#include "XBVideoConfig.h"
#include "music/tags/MusicInfoTag.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/File.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "playlists/PlayList.h"
#include "utils/Crc32.h"
#include "utils/RssReader.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "cores/dvdplayer/DVDSubtitles/DVDSubtitleTagSami.h"
#include "cores/dvdplayer/DVDSubtitles/DVDSubtitleStream.h"
#include "URL.h"
#include "LocalizeStrings.h"
#include "utils/md5.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"
#include "programs/launchers/ProgramLauncher.h"

#include "xbresource.h"
#include "platform/xbox/Undocumented.h"
#include "platform/xbox/filesystem/MemoryUnitManager.h"
#include "platform/xbox/storage/IoSupport.h"
#include "platform/xbox/utils/FanController.h"
#include "platform/xbox/utils/FilterFlickerPatch.h"
#include "platform/xbox/utils/LED.h"
#include "platform/xbox/xbeheader.h"

using namespace std;

#define clamp(x) (x) > 255.f ? 255 : ((x) < 0 ? 0 : (BYTE)(x+0.5f)) // Valid ranges: brightness[-1 -> 1 (0 is default)] contrast[0 -> 2 (1 is default)]  gamma[0.5 -> 3.5 (1 is default)] default[ramp is linear]
static const __int64 SECS_BETWEEN_EPOCHS = 11644473600LL;
static const __int64 SECS_TO_100NS = 10000000;

using namespace AUTOPTR;
using namespace MEDIA_DETECT;
using namespace XFILE;
using namespace PLAYLIST;
using KODI::UTILITY::CDigest;
static D3DGAMMARAMP oldramp, flashramp;

XBOXDETECTION v_xboxclients;

#ifdef HAS_XBOX_HARDWARE
// This are 70 Original Data Bytes because we have to restore 70 patched Bytes, not just 57
static BYTE rawData[70] =
{
    0x55, 0x8B, 0xEC, 0x81, 0xEC, 0x04, 0x01, 0x00, 0x00, 0x8B, 0x45, 0x08, 0x3D, 0x04, 0x01, 0x00,
    0x00, 0x53, 0x75, 0x32, 0x8B, 0x4D, 0x18, 0x85, 0xC9, 0x6A, 0x04, 0x58, 0x74, 0x02, 0x89, 0x01,
    0x39, 0x45, 0x14, 0x73, 0x0A, 0xB8, 0x23, 0x00, 0x00, 0xC0, 0xE9, 0x59, 0x01, 0x00, 0x00, 0x8B,
    0x4D, 0x0C, 0x89, 0x01, 0x8B, 0x45, 0x10, 0x8B, 0x0D, 0x9C, 0xFB, 0x04, 0x80, 0x89, 0x08, 0x33,
    0xC0, 0xE9, 0x42, 0x01, 0x00, 0x00,
};
static BYTE OriginalData[57]=
{
  0x55,0x8B,0xEC,0x81,0xEC,0x04,0x01,0x00,0x00,0x8B,0x45,0x08,0x3D,0x04,0x01,0x00,
  0x00,0x53,0x75,0x32,0x8B,0x4D,0x18,0x85,0xC9,0x6A,0x04,0x58,0x74,0x02,0x89,0x01,
  0x39,0x45,0x14,0x73,0x0A,0xB8,0x23,0x00,0x00,0xC0,0xE9,0x59,0x01,0x00,0x00,0x8B,
  0x4D,0x0C,0x89,0x01,0x8B,0x45,0x10,0x8B,0x0D
};

static BYTE PatchData[70]=
{
  0x55,0x8B,0xEC,0xB9,0x04,0x01,0x00,0x00,0x2B,0xE1,0x8B,0x45,0x08,0x53,0x3B,0xC1,
  0x74,0x0C,0x49,0x3B,0xC1,0x75,0x2F,0xB8,0x00,0x03,0x80,0x00,0xEB,0x05,0xB8,0x04,
  0x00,0x00,0x00,0x50,0x8B,0x4D,0x18,0x6A,0x04,0x58,0x85,0xC9,0x74,0x02,0x89,0x01,
  0x8B,0x4D,0x0C,0x89,0x01,0x59,0x8B,0x45,0x10,0x89,0x08,0x33,0xC0,0x5B,0xC9,0xC2,
  0x14,0x00,0x00,0x00,0x00,0x00
};
#endif


CUtil::CUtil(void)
{
}

CUtil::~CUtil(void)
{}

std::string CUtil::GetTitleFromPath(const std::string& strFileNameAndPath, bool bIsFolder /* = false */)
{
  CURL pathToUrl(strFileNameAndPath);
  return GetTitleFromPath(pathToUrl, bIsFolder);
}

std::string CUtil::GetTitleFromPath(const CURL& url, bool bIsFolder /* = false */)
{
  // use above to get the filename
  std::string path(url.Get());
  URIUtils::RemoveSlashAtEnd(path);
  std::string strFilename = URIUtils::GetFileName(path);

  std::string strHostname = url.GetHostName();

#ifdef HAS_UPNP
  // UPNP
  if (url.IsProtocol("upnp"))
    strFilename = CUPnPDirectory::GetFriendlyName(url);
#endif

  if (url.IsProtocol("rss"))
  {
    CRSSDirectory dir;
    CFileItemList items;
    if(dir.GetDirectory(url, items) && !items.m_strTitle.empty())
      return items.m_strTitle;
  }

  // Shoutcast
  else if (url.IsProtocol("shout"))
  {
    const std::string strFileNameAndPath = url.Get();
    const int genre = strFileNameAndPath.find_first_of('=');
    if(genre <0)
      strFilename = g_localizeStrings.Get(260);
    else
      strFilename = g_localizeStrings.Get(260) + " - " + strFileNameAndPath.substr(genre+1).c_str();
  }

  // Windows SMB Network (SMB)
  else if (url.IsProtocol("smb") && strFilename.empty())
  {
    if (url.GetHostName().empty())
    {
      strFilename = g_localizeStrings.Get(20171);
    }
    else
    {
      strFilename = url.GetHostName();
    }
  }
  // VDR Streamdev client
  else if (url.IsProtocol("vtp"))
    strFilename = g_localizeStrings.Get(20257);

  // SAP Streams
  else if (url.IsProtocol("sap") && strFilename.empty())
    strFilename = "SAP Streams";

  // Root file views
  else if (url.IsProtocol("sources"))
    strFilename = g_localizeStrings.Get(744);

  // Music Playlists
  else if (StringUtils::StartsWith(path, "special://musicplaylists"))
    strFilename = g_localizeStrings.Get(136);

  // Video Playlists
  else if (StringUtils::StartsWith(path, "special://videoplaylists"))
    strFilename = g_localizeStrings.Get(136);

  else if (URIUtils::HasParentInHostname(url) && strFilename.empty())
    strFilename = URIUtils::GetFileName(url.GetHostName());

  // now remove the extension if needed
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("filelists.showextensions") && !bIsFolder)
  {
    URIUtils::RemoveExtension(strFilename);
    return strFilename;
  }

  // URLDecode since the original path may be an URL
  CURL::Decode(strFilename);
  return strFilename;
}

bool CUtil::GetVolumeFromFileName(const std::string& strFileName, std::string& strFileTitle, std::string& strVolumeNumber)
{
  const std::vector<std::string> &regexps = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoStackRegExps;

  std::string strFileNameTemp = strFileName;
  std::string strFileNameLower = strFileName;
  StringUtils::ToLower(strFileNameLower);

  CRegExp reg;

//  CLog::Log(LOGDEBUG, "GetVolumeFromFileName:[%s]", strFileNameLower.c_str());
  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    std::string strRegExp = regexps[i];
    if (!reg.RegComp(strRegExp.c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "Invalid RegExp:[%s]", regexps[i].c_str());
      continue;
    }
//    CLog::Log(LOGDEBUG, "Regexp:[%s]", regexps[i].c_str());

    int iFoundToken = reg.RegFind(strFileNameLower.c_str());
    if (iFoundToken >= 0)
    {
      int iRegLength = reg.GetFindLen();
      int iCount = reg.GetSubCount();

      /*
      reg.DumpOvector(LOGDEBUG);
      CLog::Log(LOGDEBUG, "Subcount=%i", iCount);
      for (int j = 0; j <= iCount; j++)
      {
        std::string str = reg.GetMatch(j);
        CLog::Log(LOGDEBUG, "Sub(%i):[%s]", j, str.c_str());
      }
      */

      // simple regexp, only the volume is captured
      if (iCount == 1)
      {
        strVolumeNumber = reg.GetMatch(1);
        if (strVolumeNumber.empty()) return false;

        // Remove the extension (if any).  We do this on the base filename, as the regexp
        // match may include some of the extension (eg the "." in particular).
        // The extension will then be added back on at the end - there is no reason
        // to clean it off here. It will be cleaned off during the display routine, if
        // the settings to hide extensions are turned on.
        std::string strFileNoExt = strFileNameTemp;
        URIUtils::RemoveExtension(strFileNoExt);
        std::string strFileExt = strFileNameTemp.substr(strFileNameTemp.length() - strFileNoExt.length());
        std::string strFileRight = strFileNoExt.substr(iFoundToken + iRegLength);
        strFileTitle = strFileName.substr(0, iFoundToken) + strFileRight + strFileExt;

        return true;
      }

      // advanced regexp with prefix (1), volume (2), and suffix (3)
      else if (iCount == 3)
      {
        // second subpatten contains the stacking volume
        strVolumeNumber = reg.GetMatch(2);
        if (strVolumeNumber.empty()) return false;

        // everything before the regexp match
        strFileTitle = strFileName.substr(0, iFoundToken);

        // first subpattern contains prefix
        strFileTitle += reg.GetMatch(1);

        // third subpattern contains suffix
        strFileTitle += reg.GetMatch(3);

        // everything after the regexp match
        strFileTitle += strFileNameTemp.substr(iFoundToken + iRegLength);

        return true;
      }

      // unknown regexp format
      else
      {
        CLog::Log(LOGERROR, "Incorrect movie stacking regexp format:[%s]", regexps[i].c_str());
      }
    }
  }
  return false;
}

namespace
{
void GetTrailingDiscNumberSegmentInfoFromPath(const std::string& pathIn,
                                              size_t& pos,
                                              std::string& number)
{
  std::string path(pathIn);
  URIUtils::RemoveSlashAtEnd(path);

  pos = std::string::npos;
  number.clear();

  // Handle Disc, Disk and locale specific spellings
  std::string discStr(StringUtils::Format("/%s ", g_localizeStrings.Get(427).c_str()));
  size_t discPos = path.rfind(discStr);

  if (discPos == std::string::npos)
  {
    discStr = "/Disc ";
    discPos = path.rfind(discStr);
  }

  if (discPos == std::string::npos)
  {
    discStr = "/Disk ";
    discPos = path.rfind(discStr);
  }

  if (discPos != std::string::npos)
  {
    // Check remainder of path is numeric (eg. Disc 1)
    const std::string discNum(path.substr(discPos + discStr.size()));
    if (discNum.find_first_not_of("0123456789") == std::string::npos)
    {
      pos = discPos;
      number = discNum;
    }
  }
}
} // unnamed namespace

std::string CUtil::RemoveTrailingDiscNumberSegmentFromPath(std::string path)
{
  size_t discPos(std::string::npos);
  std::string discNum;
  GetTrailingDiscNumberSegmentInfoFromPath(path, discPos, discNum);

  if (discPos != std::string::npos)
    path.erase(discPos);

  return path;
}

std::string CUtil::GetDiscNumberFromPath(const std::string& path)
{
  size_t discPos(std::string::npos);
  std::string discNum;
  GetTrailingDiscNumberSegmentInfoFromPath(path, discPos, discNum);
  return discNum;
}

bool CUtil::GetFilenameIdentifier(const std::string& fileName,
                                  std::string& identifierType,
                                  std::string& identifier)
{
  std::string match;
  return GetFilenameIdentifier(fileName, identifierType, identifier, match);
}

bool CUtil::GetFilenameIdentifier(const std::string& fileName,
                                  std::string& identifierType,
                                  std::string& identifier,
                                  std::string& match)
{
  CRegExp reIdentifier(true, CRegExp::autoUtf8);

  const boost::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  if (!reIdentifier.RegComp(advancedSettings->m_videoFilenameIdentifierRegExp))
  {
    CLog::Log(LOGERROR, "Invalid filename identifier RegExp:'%s'",
               advancedSettings->m_videoFilenameIdentifierRegExp.c_str());
    return false;
  }
  else
  {
    if (reIdentifier.RegComp(advancedSettings->m_videoFilenameIdentifierRegExp))
    {
      if (reIdentifier.RegFind(fileName) >= 0)
      {
        match = reIdentifier.GetMatch(0);
        identifierType = reIdentifier.GetMatch(1);
        identifier = reIdentifier.GetMatch(2);
        StringUtils::ToLower(identifierType);
        return true;
      }
    }
  }
  return false;
}

bool CUtil::HasFilenameIdentifier(const std::string& fileName)
{
  std::string identifierType;
  std::string identifier;
  return GetFilenameIdentifier(fileName, identifierType, identifier);
}

void CUtil::CleanString(const std::string& strFileName, std::string& strTitle, std::string& strTitleAndYear, std::string& strYear, bool bRemoveExtension /* = false */, bool bCleanChars /* = true */)
{
  strTitleAndYear = strFileName;

  if (strFileName == "..")
   return;

  const std::vector<std::string> &regexps = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoCleanStringRegExps;

  CRegExp reTags(true);
  CRegExp reYear;

  if (!reYear.RegComp(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoCleanDateTimeRegExp))
  {
    CLog::Log(LOGERROR, "%s: Invalid datetime clean RegExp:'%s'", __FUNCTION__, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoCleanDateTimeRegExp.c_str());
  }
  else
  {
    if (reYear.RegFind(strTitleAndYear.c_str()) >= 0)
    {
      strTitleAndYear = reYear.GetReplaceString("\\1");
      strYear = reYear.GetReplaceString("\\2");
    }
  }

  URIUtils::RemoveExtension(strTitleAndYear);

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    if (!reTags.RegComp(regexps[i].c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "%s: Invalid string clean RegExp:'%s'", __FUNCTION__, regexps[i].c_str());
      continue;
    }
    int j=0;
    if ((j=reTags.RegFind(strFileName.c_str())) > 0)
      strTitleAndYear = strTitleAndYear.substr(0, j);
  }

  // final cleanup - special characters used instead of spaces:
  // all '_' tokens should be replaced by spaces
  // if the file contains no spaces, all '.' tokens should be replaced by
  // spaces - one possibility of a mistake here could be something like:
  // "Dr..StrangeLove" - hopefully no one would have anything like this.
  if (bCleanChars)
  {
    bool initialDots = true;
    bool alreadyContainsSpace = (strTitleAndYear.find(' ') != std::string::npos);

    for (int i = 0; i < (int)strTitleAndYear.size(); i++)
    {
      char c = strTitleAndYear[i];

      if (c != '.')
        initialDots = false;

      if ((c == '_') || ((!alreadyContainsSpace) && !initialDots && (c == '.')))
      {
        strTitleAndYear[i] = ' ';
      }
    }
  }

  StringUtils::Trim(strTitleAndYear);
  strTitle = strTitleAndYear;

  // append year
  if (!strYear.empty())
    strTitleAndYear = strTitle + " (" + strYear + ")";

  // restore extension if needed
  if (!bRemoveExtension)
    strTitleAndYear += URIUtils::GetExtension(strFileName);
}

void CUtil::GetQualifiedFilename(const std::string &strBasePath, std::string &strFilename)
{
  // Check if the filename is a fully qualified URL such as protocol://path/to/file
  CURL plItemUrl(strFilename);
  if (!plItemUrl.GetProtocol().empty())
    return;

  // If the filename starts "x:", "\\" or "/" it's already fully qualified so return
  if (strFilename.size() > 1)
#ifdef TARGET_POSIX
    if ( (strFilename[1] == ':') || (strFilename[0] == '/') )
#else
    if ( strFilename[1] == ':' || (strFilename[0] == '\\' && strFilename[1] == '\\'))
#endif
      return;

  // add to base path and then clean
  strFilename = URIUtils::AddFileToFolder(strBasePath, strFilename);

  // get rid of any /./ or \.\ that happen to be there
  StringUtils::Replace(strFilename, "\\.\\", "\\");
  StringUtils::Replace(strFilename, "/./", "/");

  // now find any "\\..\\" and remove them via GetParentPath
  size_t pos;
  while ((pos = strFilename.find("/../")) != std::string::npos)
  {
    std::string basePath = strFilename.substr(0, pos + 1);
    strFilename.erase(0, pos + 4);
    basePath = URIUtils::GetParentPath(basePath);
    strFilename = URIUtils::AddFileToFolder(basePath, strFilename);
  }
  while ((pos = strFilename.find("\\..\\")) != std::string::npos)
  {
    std::string basePath = strFilename.substr(0, pos + 1);
    strFilename.erase(0, pos + 4);
    basePath = URIUtils::GetParentPath(basePath);
    strFilename = URIUtils::AddFileToFolder(basePath, strFilename);
  }
}

bool CUtil::PatchCountryVideo(F_COUNTRY Country, F_VIDEO Video)
{
#ifdef HAS_XBOX_HARDWARE
  BYTE  *Kernel=(BYTE *)0x80010000;
  DWORD i, j = 0, k;
  DWORD *CountryPtr;
  BYTE  CountryValues[4]={0, 1, 2, 4};
  BYTE  VideoTyValues[5]={0, 1, 2, 3, 3};
  BYTE  VideoFrValues[5]={0x00, 0x40, 0x40, 0x80, 0x40};

  // Skip if no change is necessary...
  // That is to avoid a situation in which our Patch *and* the EvoX patch are installed
  // Otherwise the Infinite-Reboot-Patch does not work anymore!
  if(Video == XGetVideoStandard())
    return true;

  switch (Country)
  {
    case COUNTRY_EUR:
      if (!Video)
          Video = VIDEO_PAL50;
        break;
      case COUNTRY_USA:
        Video = VIDEO_NTSCM;
      Country = COUNTRY_USA;
        break;
      case COUNTRY_JAP:
        Video = VIDEO_NTSCJ;
      Country = COUNTRY_JAP;
        break;
      default:
      Country = COUNTRY_EUR;
        Video = VIDEO_PAL50;
  };

  // Search for the original code in the Kernel.
  // Searching from 0x80011000 to 0x80024000 in order that this will work on as many Kernels
  // as possible.

  for(i=0x1000; i<0x14000; i++)
  {
    if(Kernel[i]!=OriginalData[0])
        continue;

    for(j=0; j<57; j++)
    {
        if(Kernel[i+j]!=OriginalData[j])
            break;
    }
    if(j==57)
        break;
  }

  if(j==57)
  {
    // Ok, found the code to patch. Get pointer to original Country setting.
    // This may not be strictly neccessary, but lets do it anyway for completeness.

    j=(Kernel[i+57])+(Kernel[i+58]<<8)+(Kernel[i+59]<<16)+(Kernel[i+60]<<24);
    CountryPtr=(DWORD *)j;
  }
  else
  {
    // Did not find code in the Kernel. Check if my patch is already there.

    for(i=0x1000; i<0x14000; i++)
    {
      if(Kernel[i]!=PatchData[0])
        continue;

      for(j=0; j<25; j++)
      {
        if(Kernel[i+j]!=PatchData[j])
          break;
      }
      if(j==25)
        break;
    }

    if(j==25)
    {
      // Ok, found my patch. Get pointer to original Country setting.
      // This may not be strictly neccessary, but lets do it anyway for completeness.

      j=(Kernel[i+66])+(Kernel[i+67]<<8)+(Kernel[i+68]<<16)+(Kernel[i+69]<<24);
      CountryPtr=(DWORD *)j;
    }
    else
    {
      // Did not find my patch - so I can't work with this BIOS. Exit.
      return( false );
    }
  }

  // Patch in new code.

  j=MmQueryAddressProtect(&Kernel[i]);
  MmSetAddressProtect(&Kernel[i], 70, PAGE_READWRITE);

  memcpy(&Kernel[i], &PatchData[0], 70);

  // Patch Success. Fix up values.

  *CountryPtr=(DWORD)CountryValues[Country];
  Kernel[i+0x1f]=CountryValues[Country];
  Kernel[i+0x19]=VideoTyValues[Video];
  Kernel[i+0x1a]=VideoFrValues[Video];

  k=(DWORD)CountryPtr;
  Kernel[i+66]=(BYTE)(k&0xff);
  Kernel[i+67]=(BYTE)((k>>8)&0xff);
  Kernel[i+68]=(BYTE)((k>>16)&0xff);
  Kernel[i+69]=(BYTE)((k>>24)&0xff);

  MmSetAddressProtect(&Kernel[i], 70, j);

#endif
  // All Done!
  return( true );
}

bool CUtil::IsWritable(const std::string& strFile)
{
#ifdef HAS_XBOX_HARDWARE
 if (strFile.substr(0,4) == "mem:")
 {
   return g_memoryUnitManager.IsDriveWriteable(strFile);
 }
#endif
  return ( URIUtils::IsHD(strFile) || URIUtils::IsSmb(strFile) ) && !URIUtils::IsDVD(strFile);
}

bool CUtil::IsPicture(const std::string& strFile)
{
  return URIUtils::HasExtension(strFile,
                  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pictureExtensions + "|.tbn|.dds");
}

bool CUtil::ExcludeFileOrFolder(const std::string& strFileOrFolder, const std::vector<std::string>& regexps)
{
  if (strFileOrFolder.empty())
    return false;

  std::string strExclude = strFileOrFolder;
  StringUtils::ToLower(strExclude);

  CRegExp regExExcludes;

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    if (!regExExcludes.RegComp(regexps[i].c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "%s: Invalid exclude RegExp:'%s'", __FUNCTION__, regexps[i].c_str());
      continue;
    }
    if (regExExcludes.RegFind(strExclude) > -1)
    {
      CLog::Log(LOGDEBUG, "%s: File '%s' excluded. (Matches exclude rule RegExp:'%s')", __FUNCTION__, strExclude.c_str(), regexps[i].c_str());
      return true;
    }
  }
  return false;
}

void CUtil::GetFileAndProtocol(const std::string& strURL, std::string& strDir)
{
  strDir = strURL;
  if (!URIUtils::IsRemote(strURL)) return ;
  if (URIUtils::IsDVD(strURL)) return ;

  CURL url(strURL);
  strDir = StringUtils::Format("%s://%s", url.GetProtocol().c_str(), url.GetFileName().c_str());
}

int CUtil::GetDVDIfoTitle(const std::string& strFile)
{
  std::string strFilename = URIUtils::GetFileName(strFile);
  if (StringUtils::EqualsNoCase(strFilename, "video_ts.ifo")) return 0;
  //VTS_[TITLE]_0.IFO
  return atoi(strFilename.substr(4, 2).c_str());
}

std::string CUtil::GetFileDigest(const std::string& strPath, KODI::UTILITY::CDigest::Type type)
{
  CFile file;
  std::string result;
  if (file.Open(strPath))
  {
    CDigest digest(type);
    char temp[1024];
    while (true)
    {
      ssize_t read = file.Read(temp,1024);
      if (read <= 0)
        break;
      digest.Update(temp,read);
    }
    result = digest.Finalize();
    file.Close();
  }

  return result;
}

bool CUtil::CacheXBEIcon(const std::string& strFilePath, const std::string& strIcon)
{
  bool success = false;

  Crc32 crc;
  crc.ComputeFromLowerCase(strFilePath);
  std::string strTempFile = StringUtils::Format("Z:\\%08x.tbn", (unsigned __int32) crc);

  // extract icon from .xbe
  if (URIUtils::HasExtension(strFilePath, ".xbx"))
  {
    ::CopyFile(strFilePath.c_str(), strTempFile.c_str(), FALSE);
  }
  else
  {
    std::string localFile;
    g_charsetConverter.utf8ToStringCharset(strFilePath, localFile);
    CXBE xbeReader;
    if (!xbeReader.ExtractIcon(localFile, strTempFile.c_str()))
      return false;
  }

  CXBPackedResource* pPackedResource = new CXBPackedResource();
  if ( SUCCEEDED( pPackedResource->Create( strTempFile.c_str(), 1, NULL ) ) )
  {
    LPDIRECT3DTEXTURE8 pTexture = pPackedResource->GetTexture((DWORD)0);
    if ( pTexture )
    {
      D3DSURFACE_DESC descSurface;
      if ( SUCCEEDED( pTexture->GetLevelDesc( 0, &descSurface ) ) )
      {
        int iHeight = descSurface.Height;
        int iWidth = descSurface.Width;
        DWORD dwFormat = descSurface.Format;
        success = false;

        // this part of code before was in CPicture::CreateThumbnailFromSwizzledTexture
        LPDIRECT3DTEXTURE8 linTexture = NULL;
        if (D3D_OK == D3DXCreateTexture(CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice(), iWidth, iHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &linTexture))
        {
          LPDIRECT3DSURFACE8 source;
          LPDIRECT3DSURFACE8 dest;
          pTexture->GetSurfaceLevel(0, &source);
          linTexture->GetSurfaceLevel(0, &dest);
          D3DXLoadSurfaceFromSurface(dest, NULL, NULL, source, NULL, NULL, D3DX_FILTER_NONE, 0);
          D3DLOCKED_RECT lr;
          dest->LockRect(&lr, NULL, 0);
          success = CPicture::CreateThumbnailFromSurface((BYTE *)lr.pBits, iWidth, iHeight, lr.Pitch, strIcon);
          dest->UnlockRect();
          SAFE_RELEASE(source);
          SAFE_RELEASE(dest);
          SAFE_RELEASE(linTexture);
        }
        // end of CPicture::CreateThumbnailFromSwizzledTexture
      }
      pTexture->Release();
    }
  }
  delete pPackedResource;
  return success;
}

bool CUtil::GetDirectoryName(const std::string& strFileName, std::string& strDescription)
{
  std::string strFName = URIUtils::GetFileName(strFileName);
  strDescription = strFileName.substr(0, strFileName.size() - strFName.size());
  URIUtils::RemoveSlashAtEnd(strDescription);

  int iPos = strDescription.find_last_of("\\");
  if (iPos < 0)
    iPos = strDescription.find_last_of("/");
  if (iPos >= 0)
  {
    strDescription = strDescription.substr(iPos + 1);
  }
  else if (strDescription.size() <= 0)
    strDescription = strFName;
  return true;
}

bool CUtil::GetXBEDescription(const std::string& strFileName, std::string& strDescription)
{
  _XBE_CERTIFICATE HC;
  _XBE_HEADER HS;

  FILE* hFile = fopen(strFileName.c_str(), "rb");
  if (!hFile)
  {
    strDescription = URIUtils::GetFileName(strFileName);
    return false;
  }
  fread(&HS, 1, sizeof(HS), hFile);
  fseek(hFile, HS.XbeHeaderSize, SEEK_SET);
  fread(&HC, 1, sizeof(HC), hFile);
  fclose(hFile);

  // The XBE title is stored in WCHAR (UTF16) format

  // XBE titles can in fact use all 40 characters available to them,
  // and thus are not necessarily NULL terminated
  WCHAR TitleName[41];
  wcsncpy(TitleName, HC.TitleName, 40);
  TitleName[40] = 0;
  if (wcslen(TitleName) > 0)
  {
    g_charsetConverter.wToUTF8(TitleName, strDescription);
    return true;
  }
  strDescription = URIUtils::GetFileName(strFileName);
  return false;
}

bool CUtil::SetXBEDescription(const std::string& strFileName, const std::string& strDescription)
{
  _XBE_CERTIFICATE HC;
  _XBE_HEADER HS;

  FILE* hFile = fopen(strFileName.c_str(), "r+b");
  fread(&HS, 1, sizeof(HS), hFile);
  fseek(hFile, HS.XbeHeaderSize, SEEK_SET);
  fread(&HC, 1, sizeof(HC), hFile);
  fseek(hFile,HS.XbeHeaderSize, SEEK_SET);

  // The XBE title is stored in WCHAR (UTF16)

  std::wstring shortDescription;
  g_charsetConverter.utf8ToW(strDescription, shortDescription);
  if (shortDescription.size() > 40)
    shortDescription = shortDescription.substr(0, 40);
  wcsncpy(HC.TitleName, shortDescription.c_str(), 40);  // only allow 40 chars*/
  fwrite(&HC,1,sizeof(HC),hFile);
  fclose(hFile);
  return true;
}

DWORD CUtil::GetXbeID( const std::string& strFilePath)
{
  DWORD dwReturn = 0;

  DWORD dwCertificateLocation;
  DWORD dwLoadAddress;
  DWORD dwRead;
  //  WCHAR wcTitle[41];

  CAutoPtrHandle hFile( CreateFile( strFilePath.c_str(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL ));
  if ( hFile.isValid() )
  {
    if ( SetFilePointer( (HANDLE)hFile, 0x104, NULL, FILE_BEGIN ) == 0x104 )
    {
      if ( ReadFile( (HANDLE)hFile, &dwLoadAddress, 4, &dwRead, NULL ) )
      {
        if ( SetFilePointer( (HANDLE)hFile, 0x118, NULL, FILE_BEGIN ) == 0x118 )
        {
          if ( ReadFile( (HANDLE)hFile, &dwCertificateLocation, 4, &dwRead, NULL ) )
          {
            dwCertificateLocation -= dwLoadAddress;
            // Add offset into file
            dwCertificateLocation += 8;
            if ( SetFilePointer( (HANDLE)hFile, dwCertificateLocation, NULL, FILE_BEGIN ) == dwCertificateLocation )
            {
              dwReturn = 0;
              ReadFile( (HANDLE)hFile, &dwReturn, sizeof(DWORD), &dwRead, NULL );
              if ( dwRead != sizeof(DWORD) )
              {
                dwReturn = 0;
              }
            }

          }
        }
      }
    }
  }

  return dwReturn;
}

void CUtil::CreateShortcut(CFileItem* pItem)
{
  if ( pItem->IsXBE() )
  {
    // xbe
    pItem->SetArt("icon", "defaultProgram.png");
    if ( !pItem->IsOnDVD() )
    {
      std::string strDescription;
      if (! CUtil::GetXBEDescription(pItem->GetPath(), strDescription))
      {
        CUtil::GetDirectoryName(pItem->GetPath(), strDescription);
      }
      if (strDescription.size())
      {
        std::string strFname;
        strFname = URIUtils::GetFileName(pItem->GetPath());
        StringUtils::ToLower(strFname);
        if (strFname != "dashupdate.xbe" && strFname != "downloader.xbe" && strFname != "update.xbe")
        {
          CShortcut cut;
          cut.m_strPath = pItem->GetPath();
          cut.Save(strDescription);
        }
      }
    }
  }
}

std::string CUtil::GetFatXQualifiedPath(const std::string& strPath)
{
  std::string strFileNameAndPath(strPath);
  // This routine gets rid of any "\\"'s at the start of the path.
  // Should this be the case?
  std::string strBasePath, strFileName;

  // We need to check whether we must use forward (ie. special://)
  // or backslashes (ie. Q:\)
  std::string sep;
  if (strFileNameAndPath.c_str()[1] == ':' || strFileNameAndPath.find('\\')>=0)
  {
    StringUtils::Replace(strFileNameAndPath, '/', '\\');
    sep="\\";
  }
  else
  {
//    strFileNameAndPath.Replace('\\', '/');
    sep="/";
  }

  if(strFileNameAndPath.substr(std::max(0, (int)strFileNameAndPath.size() - 1)) == sep)
  {
    strBasePath = strFileNameAndPath;
    strFileName = "";
  }
  else
  {
    strBasePath = URIUtils::GetDirectory(strFileNameAndPath);
    // TODO: GETDIR - is this required?  What happens to the tokenize below otherwise?
    strFileName = URIUtils::GetFileName(strFileNameAndPath);
  }

  std::vector<std::string> tokens = StringUtils::Split(strBasePath, sep);
  if (tokens.empty())
    return strPath; // nothing to do here (invalid path)

  strFileNameAndPath = tokens.front();
  for (std::vector<std::string>::iterator token=tokens.begin()+1;token != tokens.end();++token)
  {
    std::string strToken = token->substr(0, 42);
    if (token->size() > 42)
    {
      // remove any spaces as a result of truncation (only):
      while (strToken[strToken.size()-1] == ' ')
        strToken.erase(strToken.size()-1);
    }
    CUtil::RemoveIllegalChars(strToken);
    strFileNameAndPath += sep+strToken;
  }

  if (!strFileName.empty())
  {
    CUtil::RemoveIllegalChars(strFileName);

    if (strFileName.substr(0, 1) == sep)
      strFileName.erase(0,1);

    if (CUtil::ShortenFileName(strFileName))
    {
      std::string strExtension = URIUtils::GetExtension(strFileName);
      std::string strNoExt(URIUtils::ReplaceExtension(strFileName, ""));
      // remove any spaces as a result of truncation (only):
      while (strNoExt[strNoExt.size()-1] == ' ')
        strNoExt.erase(strNoExt.size()-1);

      strFileNameAndPath += strNoExt+strExtension;
    }
    else
      strFileNameAndPath += strFileName;
  }

  return strFileNameAndPath;
}

bool CUtil::ShortenFileName(std::string& strFileNameAndPath)
{
  std::string strFile = URIUtils::GetFileName(strFileNameAndPath);
  if (strFile.size() > 42)
  {
    std::string strExtension = URIUtils::GetExtension(strFileNameAndPath);
    std::string strPath = strFileNameAndPath.substr(0, strFileNameAndPath.size() - strFile.size());

    CRegExp reg;
    std::string strSearch=strFile; StringUtils::ToLower(strSearch);
    reg.RegComp("([_\\-\\. ](cd|part)[0-9]*)[_\\-\\. ]");          // this is to ensure that cd1, cd2 or partXXX. do not
    int matchPos = reg.RegFind(strSearch.c_str());                 // get cut from filenames when they are shortened.

    std::string strPartNumber = reg.GetReplaceString("\\1");

    int partPos = 42 - strPartNumber.size() - strExtension.size();

    if (matchPos > partPos )
    {
       strFile = strFile.substr(0, partPos);
       strFile += strPartNumber;
    }
    else
    {
       strFile = strFile.substr(0, 42 - strExtension.size());
    }
    strFile += strExtension;

    std::string strNewFile = strPath;
    if (!URIUtils::HasSlashAtEnd(strNewFile) && !strNewFile.empty())
      strNewFile += "\\";

    strNewFile += strFile;
    strFileNameAndPath = strNewFile;

    // We shortened the file:
    return true;
  }

  return false;
}


void CUtil::GetDVDDriveIcon( const std::string& strPath, std::string& strIcon )
{
  if ( !CDetectDVDMedia::IsDiscInDrive() )
  {
    strIcon = "DefaultDVDEmpty.png";
    return ;
  }

  if ( URIUtils::IsDVD(strPath) )
  {
    CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
    //  xbox DVD
    if ( pInfo != NULL && pInfo->IsUDFX( 1 ) )
    {
      strIcon = "DefaultXboxDVD.png";
      return ;
    }
    strIcon = "DefaultDVDRom.png";
    return ;
  }

  if ( URIUtils::IsISO9660(strPath) )
  {
    CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
    if ( pInfo != NULL && pInfo->IsVideoCd( 1 ) )
    {
      strIcon = "DefaultVCD.png";
      return ;
    }
    strIcon = "DefaultDVDRom.png";
    return ;
  }

  if ( URIUtils::IsCDDA(strPath) )
  {
    strIcon = "DefaultCDDA.png";
    return ;
  }
}

void CUtil::RemoveTempFiles()
{
  std::string searchPath = CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetDatabaseFolder();
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".tmp", DIR_FLAG_NO_FILE_DIRS))
    return;
  for (int i = 0; i < items.Size(); ++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    XFILE::CFile::Delete(items[i]->GetPath());
  }
}

void CUtil::DeleteGUISettings()
{
  // // Load in master code first to ensure it's setting isn't reset
  // CXBMCTinyXML doc;
  // if (doc.LoadFile(CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetSettingsFile()))
  // {
  //   g_guiSettings.LoadMasterLock(doc.RootElement());
  // }
  // // delete the settings file only
  // CLog::Log(LOGINFO, "  DeleteFile(%s)", CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetSettingsFile().c_str());
  // CFile::Delete(CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetSettingsFile());
}

void CUtil::RemoveIllegalChars(std::string& strText)
{
  char szRemoveIllegal [1024];
  strcpy(szRemoveIllegal , strText.c_str());
  static char legalChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!#$%&'()-@[]^_`{}~.��������������������������������";

  char *cursor;
  for (cursor = szRemoveIllegal; *(cursor += strspn(cursor, legalChars)); /**/ )
  {
    // Convert FatX illegal characters, if possible, to the closest "looking" character:
    if (strchr("������", (int) *cursor)) *cursor = 'A';
    else
    if (strchr("������", (int) *cursor)) *cursor = 'a';
    else
    if (strchr("�����", (int) *cursor)) *cursor = 'O';
    else
    if (strchr("�����", (int) *cursor)) *cursor = 'o';
    else
    if (strchr("����", (int) *cursor)) *cursor = 'U';
    else
    if (strchr("�����", (int) *cursor)) *cursor = 'u';
    else
    if (strchr("����", (int) *cursor)) *cursor = 'E';
    else
    if (strchr("����", (int) *cursor)) *cursor = 'e';
    else
    if (strchr("����", (int) *cursor)) *cursor = 'I';
    else
    if (strchr("����", (int) *cursor)) *cursor = 'i';
    else
    *cursor = '_';
  }

  strText = szRemoveIllegal;
}

void CUtil::ClearSubtitles()
{
  //delete cached subs
  CFileItemList items;
  CDirectory::GetDirectory("special://temp/",items, "", DIR_FLAG_DEFAULTS);
  for( int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
    {
      if (items[i]->GetPath().find("subtitle") != std::string::npos ||
          items[i]->GetPath().find("vobsub_queue") != std::string::npos)
      {
        CLog::Log(LOGDEBUG, "%s - Deleting temporary subtitle %s", __FUNCTION__, items[i]->GetPath().c_str());
        CFile::Delete(items[i]->GetPath());
      }
    }
  }
}

void CUtil::ClearTempFonts()
{
  std::string searchPath = "special://temp/fonts/";

  if (!CFile::Exists(searchPath))
    return;

  CFileItemList items;
  CDirectory::GetDirectory(searchPath, items, "", DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_BYPASS_CACHE);

  for (int i=0; i<items.Size(); ++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    CFile::Delete(items[i]->GetPath());
  }
}

void CUtil::ScanForExternalSubtitles(const std::string& strMovie, std::vector<std::string>& vecSubtitles)
{
  unsigned int start = XbmcThreads::SystemClockMillis();

  CFileItem item(strMovie, false);
  if ((item.IsInternetStream() && !URIUtils::IsOnLAN(item.GetDynPath()))
    || item.IsPlayList()
    || item.IsLiveTV()
    || !item.IsVideo())
    return;

  CLog::Log(LOGDEBUG, "%s: Searching for subtitles...", __FUNCTION__);

  std::string strBasePath;
  std::string strSubtitle;

  GetVideoBasePathAndFileName(strMovie, strBasePath, strSubtitle);

  CFileItemList items;
  std::vector<std::string> common_sub_dirs;
  common_sub_dirs.push_back("subs");
  common_sub_dirs.push_back("subtitles");
  common_sub_dirs.push_back("vobsubs");
  common_sub_dirs.push_back("sub");
  common_sub_dirs.push_back("vobsub");
  common_sub_dirs.push_back("subtitle");
  const std::string subtitleExtensions = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_subtitlesExtensions;
  GetItemsToScan(strBasePath, subtitleExtensions, common_sub_dirs, items);

  const std::string customPath = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH);

  if (!CMediaSettings::GetInstance().GetAdditionalSubtitleDirectoryChecked() && !customPath.empty()) // to avoid checking non-existent directories (network) every time..
  {
    if (!CServiceBroker::GetNetwork().IsAvailable() && !URIUtils::IsHD(customPath))
    {
      CLog::Log(LOGINFO, "CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's inaccessible");
      CMediaSettings::GetInstance().SetAdditionalSubtitleDirectoryChecked(-1); // disabled
    }
    else if (!CDirectory::Exists(customPath))
    {
      CLog::Log(LOGINFO, "CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonexistent");
      CMediaSettings::GetInstance().SetAdditionalSubtitleDirectoryChecked(-1); // disabled
    }

    CMediaSettings::GetInstance().SetAdditionalSubtitleDirectoryChecked(1);
  }

  std::vector<std::string> strLookInPaths;
  // this is last because we dont want to check any common subdirs or cd-dirs in the alternate <subtitles> dir.
  if (CMediaSettings::GetInstance().GetAdditionalSubtitleDirectoryChecked() == 1)
  {
    std::string strPath2 = customPath;
    URIUtils::AddSlashAtEnd(strPath2);
    strLookInPaths.push_back(strPath2);
  }

  int flags = DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_NO_FILE_INFO;
  for (std::vector<std::string>::const_iterator path = strLookInPaths.begin(); path != strLookInPaths.end(); ++path)
  {
    CFileItemList moreItems;
    CDirectory::GetDirectory(*path, moreItems, subtitleExtensions, flags);
    items.Append(moreItems);
  }

  std::vector<std::string> exts = StringUtils::Split(subtitleExtensions, '|');
  exts.erase(std::remove(exts.begin(), exts.end(), ".zip"), exts.end());
  exts.erase(std::remove(exts.begin(), exts.end(), ".rar"), exts.end());

  ScanPathsForAssociatedItems(strSubtitle, items, exts, vecSubtitles);

  size_t iSize = vecSubtitles.size();
  for (size_t i = 0; i < iSize; i++)
  {
    if (URIUtils::HasExtension(vecSubtitles[i], ".smi"))
    {
      //Cache multi-language sami subtitle
      CDVDSubtitleStream stream;
      if (stream.Open(vecSubtitles[i]))
      {
        CDVDSubtitleTagSami TagConv;
        TagConv.LoadHead(&stream);
        if (TagConv.m_Langclass.size() >= 2)
        {
          for (std::vector<CDVDSubtitleTagSami::SLangclass>::const_iterator lang = TagConv.m_Langclass.begin(); lang != TagConv.m_Langclass.end(); ++lang)
          {
            std::string strDest =
                StringUtils::Format("special://temp/subtitle.%s.%"PRIuS".smi", lang->Name.c_str(), i);
            if (CFile::Copy(vecSubtitles[i], strDest))
            {
              CLog::Log(LOGINFO, " cached subtitle %s->%s", CURL::GetRedacted(vecSubtitles[i]).c_str(),
                        strDest.c_str());
              vecSubtitles.push_back(strDest);
            }
          }
        }
      }
    }
  }

  CLog::Log(LOGDEBUG, "%s: END (total time: %u ms)", __FUNCTION__, XbmcThreads::SystemClockMillis() - start);
}

/*! \brief in a vector of subtitles finds the corresponding .sub file for a given .idx file
 */
bool CUtil::FindVobSubPair(const std::vector<std::string>& vecSubtitles, const std::string& strIdxPath, std::string& strSubPath)
{
  if (URIUtils::HasExtension(strIdxPath, ".idx"))
  {
    std::string strIdxFile;
    std::string strIdxDirectory;
    URIUtils::Split(strIdxPath, strIdxDirectory, strIdxFile);
    for (std::vector<std::string>::const_iterator it = vecSubtitles.begin(); it != vecSubtitles.end(); ++it)
    {
      const std::string &subtitlePath = *it;
      std::string strSubFile;
      std::string strSubDirectory;
      URIUtils::Split(subtitlePath, strSubDirectory, strSubFile);
      if (URIUtils::IsInArchive(subtitlePath))
        strSubDirectory = CURL::Decode(strSubDirectory);
      if (URIUtils::HasExtension(strSubFile, ".sub") &&
          (URIUtils::PathEquals(URIUtils::ReplaceExtension(strIdxPath,""),
                                URIUtils::ReplaceExtension(subtitlePath,"")) ||
           (strSubDirectory.size() >= 11 &&
            StringUtils::EqualsNoCase(strSubDirectory.substr(6, strSubDirectory.length()-11), URIUtils::ReplaceExtension(strIdxPath,"")))))
      {
        strSubPath = subtitlePath;
        return true;
      }
    }
  }
  return false;
}

/*! \brief checks if in the vector of subtitles the given .sub file has a corresponding idx and hence is a vobsub file
 */
bool CUtil::IsVobSub(const std::vector<std::string>& vecSubtitles, const std::string& strSubPath)
{
  if (URIUtils::HasExtension(strSubPath, ".sub"))
  {
    std::string strSubFile;
    std::string strSubDirectory;
    URIUtils::Split(strSubPath, strSubDirectory, strSubFile);
    if (URIUtils::IsInArchive(strSubPath))
      strSubDirectory = CURL::Decode(strSubDirectory);
    for (std::vector<std::string>::const_iterator it = vecSubtitles.begin(); it != vecSubtitles.end(); ++it)
    {
      const std::string &subtitlePath = *it;
      std::string strIdxFile;
      std::string strIdxDirectory;
      URIUtils::Split(subtitlePath, strIdxDirectory, strIdxFile);
      if (URIUtils::HasExtension(strIdxFile, ".idx") &&
          (URIUtils::PathEquals(URIUtils::ReplaceExtension(subtitlePath,""),
                                URIUtils::ReplaceExtension(strSubPath,"")) ||
           (strSubDirectory.size() >= 11 &&
            StringUtils::EqualsNoCase(strSubDirectory.substr(6, strSubDirectory.length()-11), URIUtils::ReplaceExtension(subtitlePath,"")))))
        return true;
    }
  }
  return false;
}

/*! \brief find a plain or archived vobsub .sub file corresponding to an .idx file
 */
std::string CUtil::GetVobSubSubFromIdx(const std::string& vobSubIdx)
{
  std::string vobSub = URIUtils::ReplaceExtension(vobSubIdx, ".sub");

  // check if a .sub file exists in the same directory
  if (CFile::Exists(vobSub))
  {
    return vobSub;
  }

  // look inside a .rar or .zip in the same directory
  const std::string archTypes[] = { "rar", "zip" };
  std::string vobSubFilename = URIUtils::GetFileName(vobSub);
  for (size_t i = 0; i < sizeof(archTypes) / sizeof(std::string); ++i)
  {
    vobSub = URIUtils::CreateArchivePath(archTypes[i],
                                         CURL(URIUtils::ReplaceExtension(vobSubIdx, std::string(".") + archTypes[i])),
                                         vobSubFilename).Get();
    if (CFile::Exists(vobSub))
      return vobSub;
  }

  return std::string();
}

/*! \brief find a .idx file from a path of a plain or archived vobsub .sub file
 */
std::string CUtil::GetVobSubIdxFromSub(const std::string& vobSub)
{
  std::string vobSubIdx = URIUtils::ReplaceExtension(vobSub, ".idx");

  // check if a .idx file exists in the same directory
  if (CFile::Exists(vobSubIdx))
  {
    return vobSubIdx;
  }

  // look outside archive (usually .rar) if the .sub is inside one
  if (URIUtils::IsInArchive(vobSub))
  {

    std::string archiveFile = URIUtils::GetDirectory(vobSub);
    std::string vobSubIdxDir = URIUtils::GetParentPath(archiveFile);

    if (!vobSubIdxDir.empty())
    {
      std::string vobSubIdxFilename = URIUtils::GetFileName(vobSubIdx);
      std::string vobSubIdx = URIUtils::AddFileToFolder(vobSubIdxDir, vobSubIdxFilename);

      if (CFile::Exists(vobSubIdx))
        return vobSubIdx;
    }
  }

  return std::string();
}

void CUtil::PrepareSubtitleFonts()
{
  std::string strFontPath = "special://xbmc/system/players/mplayer/font";

  if( IsUsingTTFSubtitles()
    || CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_SUBTITLES_FONTSIZE) == 0
    || CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SUBTITLES_FONT).size() == 0)
  {
    /* delete all files in the font dir, so mplayer doesn't try to load them */

    std::string strSearchMask = strFontPath + "\\*.*";
    WIN32_FIND_DATA wfd;
    CAutoPtrFind hFind ( FindFirstFile(CSpecialProtocol::TranslatePath(strSearchMask).c_str(), &wfd));
    if (hFind.isValid())
    {
      do
      {
        if(wfd.cFileName[0] == 0) continue;
        if( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
          CFile::Delete(URIUtils::AddFileToFolder(strFontPath, wfd.cFileName));
      }
      while (FindNextFile((HANDLE)hFind, &wfd));
    }
  }
  else
  {
    std::string strPath;
    strPath = StringUtils::Format("%s\\%s\\%i",
                  strFontPath.c_str(),
                  CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("Subtitles.Font").c_str(),
                  CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("Subtitles.Height"));

    std::string strSearchMask = strPath + "\\*.*";
    WIN32_FIND_DATA wfd;
    CAutoPtrFind hFind ( FindFirstFile(CSpecialProtocol::TranslatePath(strSearchMask).c_str(), &wfd));
    if (hFind.isValid())
    {
      do
      {
        if (wfd.cFileName[0] == 0) continue;
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
        {
          std::string strSource = URIUtils::AddFileToFolder(strPath, wfd.cFileName);
          std::string strDest = URIUtils::AddFileToFolder(strFontPath, wfd.cFileName);
          CFile::Copy(strSource, strDest);
        }
      }
      while (FindNextFile((HANDLE)hFind, &wfd));
    }
  }
}

__int64 CUtil::ToInt64(DWORD dwHigh, DWORD dwLow)
{
  __int64 n;
  n = dwHigh;
  n <<= 32;
  n += dwLow;
  return n;
}

void CUtil::PlayDVD(const std::string& strProtocol, bool restart)
{
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("dvds.useexternaldvdplayer") && !CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("dvds.externaldvdplayer").empty())
  {
    LAUNCHERS::CProgramLauncher::LaunchProgram(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("dvds.externaldvdplayer"));
  }
  else
  {
    CIoSupport::Dismount("Cdrom0");
    CIoSupport::RemapDriveLetter('D', "Cdrom0");
    std::string strPath;
    strPath = StringUtils::Format("%s://1", strProtocol.c_str());
    CFileItem item(strPath, false);
    item.SetLabel(CDetectDVDMedia::GetDVDLabel());
    item.GetVideoInfoTag()->m_strFileNameAndPath = "removable://"; // need to put volume label for resume point in videoInfoTag
    item.GetVideoInfoTag()->m_strFileNameAndPath += CDetectDVDMedia::GetDVDLabel();
    if (!restart) item.SetStartOffset(STARTOFFSET_RESUME);
    g_application.PlayFile(item, "", restart);
  }
}

std::string CUtil::GetNextFilename(const std::string &fn_template, int max)
{
  if (!fn_template.find("%03d"))
    return "";

  std::string searchPath = URIUtils::GetDirectory(fn_template);
  std::string mask = URIUtils::GetExtension(fn_template);

  std::string name;
  name = StringUtils::Format(fn_template.c_str(), 0);

  CFileItemList items;
  if (!CDirectory::GetDirectory(searchPath, items, mask, DIR_FLAG_NO_FILE_DIRS))
    return name;

  items.SetFastLookup(true);
  for (int i = 0; i <= max; i++)
  {
    std::string name;
    name = StringUtils::Format(fn_template.c_str(), i);
    if (!items.Get(name))
      return name;
  }
  return "";
}

std::string CUtil::GetNextPathname(const std::string &path_template, int max)
{
  if (!path_template.find("%04d"))
    return "";

  for (int i = 0; i <= max; i++)
  {
    std::string name;
    name = StringUtils::Format(path_template.c_str(), i);
    if (!CFile::Exists(name))
      return name;
  }
  return "";
}

void CUtil::InitGamma()
{
  CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->GetGammaRamp(&oldramp);
}
void CUtil::RestoreBrightnessContrastGamma()
{
  CServiceBroker::GetWinSystem()->GetGfxContext().Lock();
  CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->SetGammaRamp(GAMMA_RAMP_FLAG, &oldramp);
  CServiceBroker::GetWinSystem()->GetGfxContext().Unlock();
}

void CUtil::SetBrightnessContrastGammaPercent(float brightness, float contrast, float gamma, bool immediate)
{
  if (brightness < 0) brightness = 0;
  if (brightness > 100) brightness = 100;
  if (contrast < 0) contrast = 0;
  if (contrast > 100) contrast = 100;
  if (gamma < 0) gamma = 0;
  if (gamma > 100) gamma = 100;

  float fBrightNess = brightness / 50.0f - 1.0f; // -1..1    Default: 0
  float fContrast = contrast / 50.0f;            // 0..2     Default: 1
  float fGamma = gamma / 40.0f + 0.5f;           // 0.5..3.0 Default: 1
  CUtil::SetBrightnessContrastGamma(fBrightNess, fContrast, fGamma, immediate);
}

void CUtil::SetBrightnessContrastGamma(float Brightness, float Contrast, float Gamma, bool bImmediate)
{
  // calculate ramp
  D3DGAMMARAMP ramp;

  Gamma = 1.0f / Gamma;
  for (int i = 0; i < 256; ++i)
  {
    float f = (powf((float)i / 255.f, Gamma) * Contrast + Brightness) * 255.f;
    ramp.blue[i] = ramp.green[i] = ramp.red[i] = clamp(f);
  }

  // set ramp next v sync
  CServiceBroker::GetWinSystem()->GetGfxContext().Lock();
  CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->SetGammaRamp(bImmediate ? GAMMA_RAMP_FLAG : 0, &ramp);
  CServiceBroker::GetWinSystem()->GetGfxContext().Unlock();
}

void CUtil::FlashScreen(bool bImmediate, bool bOn)
{
  static bool bInFlash = false;

  if (bInFlash == bOn)
    return ;
  bInFlash = bOn;
  CServiceBroker::GetWinSystem()->GetGfxContext().Lock();
  if (bOn)
  {
    CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->GetGammaRamp(&flashramp);
    SetBrightnessContrastGamma(0.5f, 1.2f, 2.0f, bImmediate);
  }
  else
    CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->SetGammaRamp(bImmediate ? GAMMA_RAMP_FLAG : 0, &flashramp);
  CServiceBroker::GetWinSystem()->GetGfxContext().Unlock();
}

void CUtil::TakeScreenshot(const std::string& strFileName, bool flashScreen)
{
    LPDIRECT3DSURFACE8 lpSurface = NULL;
    CServiceBroker::GetWinSystem()->GetGfxContext().Lock();
    std::string strFileNameTranslated = CSpecialProtocol::TranslatePath(strFileName);
    const CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
    if (appPlayer->IsPlayingVideo())
    {
#ifdef HAS_VIDEO_PLAYBACK
      g_renderManager.SetupScreenshot();
#endif
    }
    if (0)
    { // reset calibration to defaults
      OVERSCAN oscan;
      memcpy(&oscan, &CDisplaySettings::GetInstance().GetResolutionInfo(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution()).Overscan, sizeof(OVERSCAN));
      CServiceBroker::GetWinSystem()->GetGfxContext().ResetOverscan(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution(), CDisplaySettings::GetInstance().GetResolutionInfo(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution()).Overscan);
      g_application.Render();
      memcpy(&CDisplaySettings::GetInstance().GetResolutionInfo(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution()).Overscan, &oscan, sizeof(OVERSCAN));
    }
    // now take screenshot
#ifdef HAS_XBOX_D3D
    CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->BlockUntilVerticalBlank();
#endif
#ifdef HAS_XBOX_D3D
    if (SUCCEEDED(CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->GetBackBuffer( -1, D3DBACKBUFFER_TYPE_MONO, &lpSurface)))
#else
    g_application.RenderNoPresent();
    if (SUCCEEDED(CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &lpSurface)))
#endif
    {
      if (FAILED(XGWriteSurfaceToFile(lpSurface, strFileNameTranslated.c_str())))
      {
        CLog::Log(LOGERROR, "Failed to Generate Screenshot");
      }
      else
      {
        // hack - need to add it manually to the directory cache for both the special:// and the mapped
        // folder due to CFile::Open on a special:// path being checked against both. Ideally we would use
        // the XBMC Fileystem for writing the file. TODO.
        g_directoryCache.AddFile(strFileName);
        g_directoryCache.AddFile(strFileNameTranslated);
        CLog::Log(LOGINFO, "Screen shot saved as %s", strFileNameTranslated.c_str());
      }
      lpSurface->Release();
    }
    CServiceBroker::GetWinSystem()->GetGfxContext().Unlock();
    if (flashScreen)
    {
#ifdef HAS_XBOX_D3D
      CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->BlockUntilVerticalBlank();
#endif
      FlashScreen(true, true);
      Sleep(10);
#ifdef HAS_XBOX_D3D
      CServiceBroker::GetWinSystem()->GetGfxContext().Get3DDevice()->BlockUntilVerticalBlank();
#endif
      FlashScreen(true, false);
    }
}

void CUtil::TakeScreenshot()
{
  static bool savingScreenshots = false;
  static vector<std::string> screenShots;

  bool promptUser = false;
  // check to see if we have a screenshot folder yet
  std::string strDir/* = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("debug.screenshotpath", false)*/;
  if (strDir.empty())
  {
    strDir = "special://temp/";
    if (!savingScreenshots)
    {
      promptUser = true;
      savingScreenshots = true;
      screenShots.clear();
    }
  }
  URIUtils::RemoveSlashAtEnd(strDir);

  if (!strDir.empty())
  {
    std::string file = CUtil::GetNextFilename(URIUtils::AddFileToFolder(strDir, "screenshot%03d.bmp"), 999);

    if (!file.empty())
    {
      TakeScreenshot(file.c_str(), true);
      if (savingScreenshots)
        screenShots.push_back(file);
      if (promptUser)
      { // grab the real directory
        std::string newDir = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("debug.screenshotpath");
        if (!newDir.empty())
        {
          for (unsigned int i = 0; i < screenShots.size(); i++)
          {
            std::string file = CUtil::GetNextFilename(URIUtils::AddFileToFolder(newDir, "screenshot%03d.bmp"), 999);
            CFile::Copy(screenShots[i], file);
          }
          screenShots.clear();
        }
        savingScreenshots = false;
      }
    }
    else
    {
      CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
    }
  }
}

void CUtil::StatToStatI64(struct _stati64 *result, struct stat *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = (__int64)stat->st_size;

#ifndef _LINUX
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#else
  result->_st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->_st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->_st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#endif
}

void CUtil::Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = stat->st_size;
#ifndef _LINUX
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#else
  result->_st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->_st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->_st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#endif
}

void CUtil::StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = stat->st_size;
#ifndef _LINUX
  result->st_atime = stat->st_atime;
  result->st_mtime = stat->st_mtime;
  result->st_ctime = stat->st_ctime;
#else
  result->st_atime = stat->_st_atime;
  result->st_mtime = stat->_st_mtime;
  result->st_ctime = stat->_st_ctime;
#endif
}

void CUtil::Stat64ToStat(struct _stat *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
#ifndef _LINUX
  if (stat->st_size <= LONG_MAX)
    result->st_size = (_off_t)stat->st_size;
#else
  if (sizeof(stat->st_size) <= sizeof(result->st_size) )
    result->st_size = (off_t)stat->st_size;
#endif
  else
  {
    result->st_size = 0;
    CLog::Log(LOGWARNING, "WARNING: File is larger than 32bit stat can handle, file size will be reported as 0 bytes");
  }
  result->st_atime = (time_t)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (time_t)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (time_t)(stat->st_ctime & 0xFFFFFFFF);
}

bool CUtil::CreateDirectoryEx(const std::string& strPath)
{
  // Function to create all directories at once instead
  // of calling CreateDirectory for every subdir.
  // Creates the directory and subdirectories if needed.

  // return true if directory already exist
  if (CDirectory::Exists(strPath)) return true;

  // we currently only allow HD and smb paths
  if (!URIUtils::IsHD(strPath) && !URIUtils::IsSmb(strPath))
  {
    CLog::Log(LOGERROR,"%s called with an unsupported path: %s", __FUNCTION__, strPath.c_str());
    return false;
  }

  std::vector<std::string> dirs = URIUtils::SplitPath(strPath);
  std::string dir(dirs.front());
  URIUtils::AddSlashAtEnd(dir);
  for (std::vector<std::string>::iterator it = dirs.begin() + 1; it != dirs.end(); it ++)
  {
    dir = URIUtils::AddFileToFolder(dir, *it);
    CDirectory::Create(dir);
  }

  // was the final destination directory successfully created ?
  if (!CDirectory::Exists(strPath)) return false;
  return true;
}

std::string CUtil::MakeLegalFileName(const std::string &strFile, int LegalType)
{
  std::string result = strFile;

  StringUtils::Replace(result, '/', '_');
  StringUtils::Replace(result, '\\', '_');
  StringUtils::Replace(result, '?', '_');

  if (LegalType == LEGAL_WIN32_COMPAT)
  {
    // just filter out some illegal characters on windows
    StringUtils::Replace(result, ':', '_');
    StringUtils::Replace(result, '*', '_');
    StringUtils::Replace(result, '?', '_');
    StringUtils::Replace(result, '\"', '_');
    StringUtils::Replace(result, '<', '_');
    StringUtils::Replace(result, '>', '_');
    StringUtils::Replace(result, '|', '_');
    StringUtils::TrimRight(result, ".");
    StringUtils::TrimRight(result, " ");
  }

  // check if the filename is a legal FATX one.
  if (LegalType == LEGAL_FATX)
  {
    StringUtils::Replace(result, ':', '_');
    StringUtils::Replace(result, '*', '_');
    StringUtils::Replace(result, '?', '_');
    StringUtils::Replace(result, '\"', '_');
    StringUtils::Replace(result, '<', '_');
    StringUtils::Replace(result, '>', '_');
    StringUtils::Replace(result, '|', '_');
    StringUtils::Replace(result, ',', '_');
    StringUtils::Replace(result, '=', '_');
    StringUtils::Replace(result, '+', '_');
    StringUtils::Replace(result, ';', '_');
    StringUtils::Replace(result, '"', '_');
    StringUtils::Replace(result, '\'', '_');
    StringUtils::TrimRight(result, ".");
    StringUtils::TrimRight(result, " ");

    GetFatXQualifiedPath(result);
  }

  return result;
}

// legalize entire path
std::string CUtil::MakeLegalPath(const std::string &strPathAndFile, int LegalType)
{
  if (URIUtils::IsStack(strPathAndFile))
    return MakeLegalPath(CStackDirectory::GetFirstStackedFile(strPathAndFile));
  if (URIUtils::IsMultiPath(strPathAndFile))
    return MakeLegalPath(CMultiPathDirectory::GetFirstPath(strPathAndFile));
  if (!URIUtils::IsHD(strPathAndFile) && !URIUtils::IsSmb(strPathAndFile))
    return strPathAndFile; // we don't support writing anywhere except HD, SMB and NFS - no need to legalize path

  bool trailingSlash = URIUtils::HasSlashAtEnd(strPathAndFile);
  std::vector<std::string> dirs = URIUtils::SplitPath(strPathAndFile);
  // we just add first token to path and don't legalize it - possible values:
  // "X:" (local win32), "" (local unix - empty string before '/') or
  // "protocol://domain"
  std::string dir(dirs.front());
  URIUtils::AddSlashAtEnd(dir);
  for (std::vector<std::string>::iterator it = dirs.begin() + 1; it != dirs.end(); it ++)
    dir = URIUtils::AddFileToFolder(dir, MakeLegalFileName(*it, LegalType));
  if (trailingSlash) URIUtils::AddSlashAtEnd(dir);
  return dir;
}

std::string CUtil::ValidatePath(const std::string &path, bool bFixDoubleSlashes /* = false */)
{
  std::string result = path;

  // Don't do any stuff on URLs containing %-characters or protocols that embed
  // filenames. NOTE: Don't use IsInZip or IsInRar here since it will infinitely
  // recurse and crash XBMC
  if (URIUtils::IsURL(path) &&
     (path.find('%') >= 0 ||
      StringUtils::StartsWithNoCase(path, "apk:") ||
      StringUtils::StartsWithNoCase(path, "zip:") ||
      StringUtils::StartsWithNoCase(path, "rar:") ||
      StringUtils::StartsWithNoCase(path, "stack:") ||
      StringUtils::StartsWithNoCase(path, "bluray:") ||
      StringUtils::StartsWithNoCase(path, "multipath:") ))
    return result;

  // check the path for incorrect slashes
  if (URIUtils::IsDOSPath(path))
  {
    StringUtils::Replace(result, '/', '\\');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes)
    {
      // Fixup for double back slashes (but ignore the \\ of unc-paths)
      for (unsigned int x = 1; x < result.size() - 1; x++)
      {
        if (result[x] == '\\' && result[x+1] == '\\')
          result.erase(x);
      }
    }
  }
  else if (path.find("://") >= 0 || path.find(":\\\\") >= 0)
  {
    StringUtils::Replace(result, '\\', '/');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes)
    {
      // Fixup for double forward slashes(/) but don't touch the :// of URLs
      for (unsigned int x = 2; x < result.size() - 1; x++)
      {
        if ( result[x] == '/' && result[x + 1] == '/' && !(result[x - 1] == ':' || (result[x - 1] == '/' && result[x - 2] == ':')) )
          result.erase(x);
      }
    }
  }
  return result;
}

bool CUtil::IsUsingTTFSubtitles()
{
  return URIUtils::HasExtension(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SUBTITLES_FONT), ".ttf");
}

void CUtil::SplitExecFunction(const std::string &execString, std::string &function, vector<string> &parameters)
{
  std::string paramString;

  size_t iPos = execString.find("(");
  size_t iPos2 = execString.rfind(")");
  if (iPos != std::string::npos && iPos2 != std::string::npos)
  {
    paramString = execString.substr(iPos + 1, iPos2 - iPos - 1);
    function = execString.substr(0, iPos);
  }
  else
    function = execString;

  // remove any whitespace, and the standard prefix (if it exists)
  StringUtils::Trim(function);
  if( StringUtils::StartsWithNoCase(function, "xbmc.") )
    function.erase(0, 5);

  SplitParams(paramString, parameters);
}

void CUtil::SplitParams(const std::string &paramString, std::vector<std::string> &parameters)
{
  bool inQuotes = false;
  bool lastEscaped = false; // only every second character can be escaped
  int inFunction = 0;
  size_t whiteSpacePos = 0;
  std::string parameter;
  parameters.clear();
  for (size_t pos = 0; pos < paramString.size(); pos++)
  {
    char ch = paramString[pos];
    bool escaped = (pos > 0 && paramString[pos - 1] == '\\' && !lastEscaped);
    lastEscaped = escaped;
    if (inQuotes)
    { // if we're in a quote, we accept everything until the closing quote
      if (ch == '"' && !escaped)
      { // finished a quote - no need to add the end quote to our string
        inQuotes = false;
      }
    }
    else
    { // not in a quote, so check if we should be starting one
      if (ch == '"' && !escaped)
      { // start of quote - no need to add the quote to our string
        inQuotes = true;
      }
      if (inFunction && ch == ')')
      { // end of a function
        inFunction--;
      }
      if (ch == '(')
      { // start of function
        inFunction++;
      }
      if (!inFunction && ch == ',')
      { // not in a function, so a comma signfies the end of this parameter
        if (whiteSpacePos)
          parameter = parameter.substr(0, whiteSpacePos);
        // trim off start and end quotes
        if (parameter.length() > 1 && parameter[0] == '"' && parameter[parameter.length() - 1] == '"')
          parameter = parameter.substr(1, parameter.length() - 2);
        else if (parameter.length() > 3 && parameter[parameter.length() - 1] == '"')
        {
          // check name="value" style param.
          size_t quotaPos = parameter.find('"');
          if (quotaPos > 1 && quotaPos < parameter.length() - 1 && parameter[quotaPos - 1] == '=')
          {
            parameter.erase(parameter.length() - 1);
            parameter.erase(quotaPos);
          }
        }
        parameters.push_back(parameter);
        parameter.clear();
        whiteSpacePos = 0;
        continue;
      }
    }
    if ((ch == '"' || ch == '\\') && escaped)
    { // escaped quote or backslash
      parameter[parameter.size()-1] = ch;
      continue;
    }
    // whitespace handling - we skip any whitespace at the left or right of an unquoted parameter
    if (ch == ' ' && !inQuotes)
    {
      if (parameter.empty()) // skip whitespace on left
        continue;
      if (!whiteSpacePos) // make a note of where whitespace starts on the right
        whiteSpacePos = parameter.size();
    }
    else
      whiteSpacePos = 0;
    parameter += ch;
  }
  if (inFunction || inQuotes)
    CLog::Log(LOGWARNING, "%s(%s) - end of string while searching for ) or \"", __FUNCTION__, paramString.c_str());
  if (whiteSpacePos)
    parameter.erase(whiteSpacePos);
  // trim off start and end quotes
  if (parameter.size() > 1 && parameter[0] == '"' && parameter[parameter.size() - 1] == '"')
    parameter = parameter.substr(1,parameter.size() - 2);
  else if (parameter.size() > 3 && parameter[parameter.size() - 1] == '"')
  {
    // check name="value" style param.
    size_t quotaPos = parameter.find('"');
    if (quotaPos > 1 && quotaPos < parameter.length() - 1 && parameter[quotaPos - 1] == '=')
    {
      parameter.erase(parameter.length() - 1);
      parameter.erase(quotaPos);
    }
  }
  if (!parameter.empty() || parameters.size())
    parameters.push_back(parameter);
}

int CUtil::GetMatchingSource(const std::string& strPath1, VECSOURCES& VECSOURCES, bool& bIsSourceName)
{
  if (strPath1.empty())
    return -1;

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing original path/name [%s]", strPath1.c_str());

  // copy as we may change strPath
  std::string strPath = strPath1;

  // Check for special protocols
  CURL checkURL(strPath);

  // stack://
  if (checkURL.IsProtocol("stack"))
    strPath.erase(0, 8); // remove the stack protocol

  if (checkURL.IsProtocol("shout"))
    strPath = checkURL.GetHostName();
  if (checkURL.IsProtocol("plugin"))
    return 1;
  if (checkURL.IsProtocol("multipath"))
    strPath = CMultiPathDirectory::GetFirstPath(strPath);

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing for matching name [%s]", strPath.c_str());
  bIsSourceName = false;
  int iIndex = -1;
  int iLength = -1;
  // we first test the NAME of a source
  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);
    std::string strName = share.strName;

    // special cases for dvds
    if (URIUtils::IsOnDVD(share.strPath))
    {
      if (URIUtils::IsOnDVD(strPath))
        return i;

      // not a path, so we need to modify the source name
      // since we add the drive status and disc name to the source
      // "Name (Drive Status/Disc Name)"
      int iPos = strName.rfind('(');
      if (iPos > 1)
        strName = strName.substr(0, iPos - 1);
    }
    //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, comparing name [%s]", strName.c_str());
    if (strPath == strName)
    {
      bIsSourceName = true;
      return i;
    }
  }

  // now test the paths

  // remove user details, and ensure path only uses forward slashes
  // and ends with a trailing slash so as not to match a substring
  CURL urlDest(strPath);
  urlDest.SetOptions("");
  std::string strDest = urlDest.GetWithoutUserDetails();
  ForceForwardSlashes(strDest);
  if (!URIUtils::HasSlashAtEnd(strDest))
    strDest += "/";
  int iLenPath = strDest.size();

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing url [%s]", strDest.c_str());

  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);

    // does it match a source name?
    if (share.strPath.substr(0,8) == "shout://")
    {
      CURL url(share.strPath);
      if (strPath == url.GetHostName())
        return i;
    }

    // doesnt match a name, so try the source path
    std::vector<std::string> vecPaths;

    // add any concatenated paths if they exist
    if (share.vecPaths.size() > 0)
      vecPaths = share.vecPaths;

    // add the actual share path at the front of the vector
    vecPaths.insert(vecPaths.begin(), share.strPath);

    // test each path
    for (int j = 0; j < (int)vecPaths.size(); ++j)
    {
      // remove user details, and ensure path only uses forward slashes
      // and ends with a trailing slash so as not to match a substring
      CURL urlShare(vecPaths[j]);
      urlShare.SetOptions("");
      std::string strShare = urlShare.GetWithoutUserDetails();
      ForceForwardSlashes(strShare);
      if (!URIUtils::HasSlashAtEnd(strShare))
        strShare += "/";
      int iLenShare = strShare.size();
      //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, comparing url [%s]", strShare.c_str());

      if ((iLenPath >= iLenShare) && StringUtils::StartsWithNoCase(strDest, strShare) && (iLenShare > iLength))
      {
        //CLog::Log(LOGDEBUG,"Found matching source at index %i: [%s], Len = [%i]", i, strShare.c_str(), iLenShare);

        // if exact match, return it immediately
        if (iLenPath == iLenShare)
        {
          // if the path EXACTLY matches an item in a concatentated path
          // set source name to true to load the full virtualpath
          bIsSourceName = false;
          if (vecPaths.size() > 1)
            bIsSourceName = true;
          return i;
        }
        iIndex = i;
        iLength = iLenShare;
      }
    }
  }

  // return the index of the share with the longest match
  if (iIndex == -1)
  {

    // rar:// and zip://
    // if archive wasn't mounted, look for a matching share for the archive instead
    if( StringUtils::StartsWithNoCase(strPath, "rar://") || StringUtils::StartsWithNoCase(strPath, "zip://") )
    {
      // get the hostname portion of the url since it contains the archive file
      strPath = checkURL.GetHostName();

      bIsSourceName = false;
      bool bDummy;
      return GetMatchingSource(strPath, VECSOURCES, bDummy);
    }

    CLog::Log(LOGWARNING,"CUtil::GetMatchingSource... no matching source found for [%s]", strPath1.c_str());
  }
  return iIndex;
}

std::string CUtil::TranslateSpecialSource(const std::string &strSpecial)
{
  if (!strSpecial.empty() && strSpecial[0] == '$')
  {
    if (StringUtils::StartsWithNoCase(strSpecial, "$home"))
      return URIUtils::AddFileToFolder("special://home/", strSpecial.substr(5));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$subtitles"))
      return URIUtils::AddFileToFolder("special://subtitles/", strSpecial.substr(10));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$userdata"))
      return URIUtils::AddFileToFolder("special://userdata/", strSpecial.substr(9));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$database"))
      return URIUtils::AddFileToFolder("special://database/", strSpecial.substr(9));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$thumbnails"))
      return URIUtils::AddFileToFolder("special://thumbnails/", strSpecial.substr(11));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$recordings"))
      return URIUtils::AddFileToFolder("special://recordings/", strSpecial.substr(11));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$screenshots"))
      return URIUtils::AddFileToFolder("special://screenshots/", strSpecial.substr(12));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$musicplaylists"))
      return URIUtils::AddFileToFolder("special://musicplaylists/", strSpecial.substr(15));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$videoplaylists"))
      return URIUtils::AddFileToFolder("special://videoplaylists/", strSpecial.substr(15));
    else if (StringUtils::StartsWithNoCase(strSpecial, "$cdrips"))
      return URIUtils::AddFileToFolder("special://cdrips/", strSpecial.substr(7));
    // this one will be removed post 2.0
    else if (StringUtils::StartsWithNoCase(strSpecial, "$playlists"))
      return URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("system.playlistspath"), strSpecial.substr(10));
  }
  return strSpecial;
}

std::string CUtil::MusicPlaylistsLocation()
{
  std::vector<std::string> vec;
  vec.push_back(URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("system.playlistspath"), "music"));
  vec.push_back(URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("system.playlistspath"), "mixed"));
  return XFILE::CMultiPathDirectory::ConstructMultiPath(vec);
}

std::string CUtil::VideoPlaylistsLocation()
{
  std::vector<std::string> vec;
  vec.push_back(URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("system.playlistspath"), "video"));
  vec.push_back(URIUtils::AddFileToFolder(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("system.playlistspath"), "mixed"));
  return XFILE::CMultiPathDirectory::ConstructMultiPath(vec);
}

void CUtil::DeleteMusicDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("mdb-");
}

void CUtil::DeleteVideoDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("vdb-");
}

void CUtil::DeleteProgramDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("10001-");
}

void CUtil::DeleteDirectoryCache(const std::string &prefix)
{
  std::string searchPath = "special://temp/";
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".fi", DIR_FLAG_NO_FILE_DIRS))
    return;

  for (int i = 0; i < items.Size(); ++i)
  {
    const CFileItemPtr &item = items[i];
    if (item->m_bIsFolder)
      continue;
    std::string fileName = URIUtils::GetFileName(item->GetPath());
    if (StringUtils::StartsWith(fileName, prefix))
      XFILE::CFile::Delete(item->GetPath());
  }
}

bool CUtil::SetSysDateTimeYear(int iYear, int iMonth, int iDay, int iHour, int iMinute)
{
  TIME_ZONE_INFORMATION tziNew;
  SYSTEMTIME CurTime;
  SYSTEMTIME NewTime;
  GetLocalTime(&CurTime);
  GetLocalTime(&NewTime);
  int iRescBiases, iHourUTC;
  int iMinuteNew;

  DWORD dwRet = GetTimeZoneInformation(&tziNew);  // Get TimeZone Informations
  float iGMTZone = (float(tziNew.Bias)/(60));     // Calc's the GMT Time

  CLog::Log(LOGDEBUG, "------------ TimeZone -------------");
  CLog::Log(LOGDEBUG, "-      GMT Zone: GMT %.1f",iGMTZone);
  CLog::Log(LOGDEBUG, "-          Bias: %lu minutes",tziNew.Bias);
  CLog::Log(LOGDEBUG, "-  DaylightBias: %lu",tziNew.DaylightBias);
  CLog::Log(LOGDEBUG, "-  StandardBias: %lu",tziNew.StandardBias);

  switch (dwRet)
  {
    case TIME_ZONE_ID_STANDARD:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 1, Standart");
      }
      break;
    case TIME_ZONE_ID_DAYLIGHT:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias + tziNew.DaylightBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 2, Daylight");
      }
      break;
    case TIME_ZONE_ID_UNKNOWN:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 0, Unknown");
      }
      break;
    case TIME_ZONE_ID_INVALID:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: Invalid");
      }
      break;
    default:
      iRescBiases   = tziNew.Bias + tziNew.StandardBias;
  }
    CLog::Log(LOGDEBUG, "--------------- END ---------------");

  // Calculation
  iHourUTC = GMTZoneCalc(iRescBiases, iHour, iMinute, iMinuteNew);
  iMinute = iMinuteNew;
  if(iHourUTC <0)
  {
    iDay = iDay - 1;
    iHourUTC =iHourUTC + 24;
  }
  if(iHourUTC >23)
  {
    iDay = iDay + 1;
    iHourUTC =iHourUTC - 24;
  }

  // Set the New-,Detected Time Values to System Time!
  NewTime.wYear     = (WORD)iYear;
  NewTime.wMonth    = (WORD)iMonth;
  NewTime.wDay      = (WORD)iDay;
  NewTime.wHour     = (WORD)iHourUTC;
  NewTime.wMinute   = (WORD)iMinute;

  FILETIME stNewTime, stCurTime;
  SystemTimeToFileTime(&NewTime, &stNewTime);
  SystemTimeToFileTime(&CurTime, &stCurTime);
#ifdef HAS_XBOX_HARDWARE
  bool bReturn=NT_SUCCESS(NtSetSystemTime(&stNewTime, &stCurTime));
#else
  bool bReturn(false);
#endif
  return bReturn;
}
int CUtil::GMTZoneCalc(int iRescBiases, int iHour, int iMinute, int &iMinuteNew)
{
  int iHourUTC, iTemp;
  iMinuteNew = iMinute;
  iTemp = iRescBiases/60;

  if (iRescBiases == 0 )return iHour;   // GMT Zone 0, no need calculate
  if (iRescBiases > 0)
    iHourUTC = iHour + abs(iTemp);
  else
    iHourUTC = iHour - abs(iTemp);

  if ((iTemp*60) != iRescBiases)
  {
    if (iRescBiases > 0)
      iMinuteNew = iMinute + abs(iTemp*60 - iRescBiases);
    else
      iMinuteNew = iMinute - abs(iTemp*60 - iRescBiases);

    if (iMinuteNew >= 60)
    {
      iMinuteNew = iMinuteNew -60;
      iHourUTC = iHourUTC + 1;
    }
    else if (iMinuteNew < 0)
    {
      iMinuteNew = iMinuteNew +60;
      iHourUTC = iHourUTC - 1;
    }
  }
  return iHourUTC;
}

bool CUtil::AutoDetection()
{
  bool bReturn=false;
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("autodetect.onoff"))
  {
    static unsigned int pingTimer = 0;
    if( XbmcThreads::SystemClockMillis() - pingTimer < (unsigned int)CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_autoDetectPingTime * 1000)
      return false;
    pingTimer = XbmcThreads::SystemClockMillis();

  // send ping and request new client info
  if ( CUtil::AutoDetectionPing(
    CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("Autodetect.senduserpw") ? CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SERVICES_FTPSERVER_USER):"anonymous",
    CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("Autodetect.senduserpw") ? CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SERVICES_FTPSERVER_PASSWORD):"anonymous",
    CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("autodetect.nickname"),21 /*Our FTP Port! TODO: Extract FTP from FTP Server settings!*/) )
  {
    std::string strFTPPath, strNickName, strFtpUserName, strFtpPassword, strFtpPort, strBoosMode;
    std::vector<std::string> arSplit;
    // do we have clients in our list ?
    for(unsigned int i=0; i < v_xboxclients.client_ip.size(); i++)
    {
      // extract client informations
      arSplit = StringUtils::Split(v_xboxclients.client_info[i],";");
      if ((int)arSplit.size() > 1 && !v_xboxclients.client_informed[i])
      {
        //extract client info and build the ftp link!
        strNickName     = arSplit[0].c_str();
        strFtpUserName  = arSplit[1].c_str();
        strFtpPassword  = arSplit[2].c_str();
        strFtpPort      = arSplit[3].c_str();
        strBoosMode     = arSplit[4].c_str();
        strFTPPath = StringUtils::Format("ftp://%s:%s@%s:%s/",strFtpUserName.c_str(),strFtpPassword.c_str(),v_xboxclients.client_ip[i].c_str(),strFtpPort.c_str());

        //Do Notification for this Client
        std::string strtemplbl;
        strtemplbl = StringUtils::Format("%s %s",strNickName.c_str(), v_xboxclients.client_ip[i].c_str());
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(38703), strtemplbl);

        //Debug Log
        CLog::Log(LOGDEBUG,"%s: %s FTP-Link: %s", g_localizeStrings.Get(38703).c_str(), strNickName.c_str(), strFTPPath.c_str());

        //set the client_informed to TRUE, to prevent loop Notification
        v_xboxclients.client_informed[i]=true;

        //YES NO PopUP: ask for connecting to the detected client via Filemanger!
        if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("autodetect.popupinfo") && CGUIDialogYesNo::ShowAndGetInput(38703, 0, 38708, 0))
        {
          CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_FILES, strFTPPath); //Open in MyFiles
        }
        bReturn = true;
      }
    }
  }
  }
  return bReturn;
}

bool CUtil::AutoDetectionPing(std::string strFTPUserName, std::string strFTPPass, std::string strNickName, int iFTPPort)
{
  bool bFoundNewClient= false;
  std::string strLocalIP;
  std::string strSendMessage = "ping\0";
  std::string strReceiveMessage = "ping";
  int iUDPPort = 4905;
  char sztmp[512];

  static int udp_server_socket, inited=0;
    int cliLen, t1,t2,t3,t4, init_counter=0, life=0;

  struct sockaddr_in    server;
  struct sockaddr_in    cliAddr;
  struct timeval timeout={0,500};
  fd_set readfds;
#ifdef HAS_XBOX_HARDWARE
    XNADDR xna;
    DWORD dwState = XNetGetTitleXnAddr(&xna);
    XNetInAddrToString(xna.ina,(char *)strLocalIP.c_str(),64);
#else
    char hostname[255];
    WORD wVer;
    WSADATA wData;
    PHOSTENT hostinfo;
    wVer = MAKEWORD( 2, 0 );
    if (WSAStartup(wVer,&wData) == 0)
    {
      if(gethostname(hostname,sizeof(hostname)) == 0)
      {
        if((hostinfo = gethostbyname(hostname)) != NULL)
        {
          strLocalIP = inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list);
          strNickName = StringUtils::Format("%s",hostname);
        }
      }
      WSACleanup();
    }
#endif
  // get IP address
  sscanf( (char *)strLocalIP.c_str(), "%d.%d.%d.%d", &t1, &t2, &t3, &t4 );
  if( !t1 ) return false;
  cliLen = sizeof( cliAddr);
  // setup UDP socket
  if( !inited )
  {
    int tUDPsocket  = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
      char value      = 1;
      setsockopt( tUDPsocket, SOL_SOCKET, SO_BROADCAST, &value, value );
      struct sockaddr_in addr;
      memset(&(addr),0,sizeof(addr));
      addr.sin_family       = AF_INET;
      addr.sin_addr.s_addr  = INADDR_ANY;
      addr.sin_port         = htons(iUDPPort);
      bind(tUDPsocket,(struct sockaddr *)(&addr),sizeof(addr));
    udp_server_socket = tUDPsocket;
    inited = 1;
  }
  FD_ZERO(&readfds);
  FD_SET(udp_server_socket, &readfds);
  life = select( 0,&readfds, NULL, NULL, &timeout );
  if (life == SOCKET_ERROR )
    return false;
  memset(&(server),0,sizeof(server));
  server.sin_family = AF_INET;
#ifndef _LINUX
  server.sin_addr.S_un.S_addr = INADDR_BROADCAST;
#else
  server.sin_addr.s_addr = INADDR_BROADCAST;
#endif
  server.sin_port = htons(iUDPPort);
  sendto(udp_server_socket,(char *)strSendMessage.c_str(),5,0,(struct sockaddr *)(&server),sizeof(server));
    FD_ZERO(&readfds);
    FD_SET(udp_server_socket, &readfds);
    life = select( 0,&readfds, NULL, NULL, &timeout );

  unsigned int iLookUpCountMax = 2;
  unsigned int i=0;
  bool bUpdateShares=false;

  // Ping able clients? 0:false
  if (life == 0 )
  {
    if(v_xboxclients.client_ip.size() > 0)
    {
      // clients in list without life signal!
      // calculate iLookUpCountMax value counter dependence on clients size!
      if(v_xboxclients.client_ip.size() > iLookUpCountMax)
        iLookUpCountMax += (v_xboxclients.client_ip.size()-iLookUpCountMax);

      for (i=0; i<v_xboxclients.client_ip.size(); i++)
      {
        bUpdateShares=false;
        //only 1 client, clear our list
        if(v_xboxclients.client_lookup_count[i] >= iLookUpCountMax && v_xboxclients.client_ip.size() == 1 )
        {
          v_xboxclients.client_ip.clear();
          v_xboxclients.client_info.clear();
          v_xboxclients.client_lookup_count.clear();
          v_xboxclients.client_informed.clear();

          // debug log, clients removed from our list
          CLog::Log(LOGDEBUG,"Autodetection: all Clients Removed! (mode LIFE 0)");
          bUpdateShares = true;
        }
        else
        {
          // check client lookup counter! Not reached the CountMax, Add +1!
          if(v_xboxclients.client_lookup_count[i] < iLookUpCountMax )
            v_xboxclients.client_lookup_count[i] = v_xboxclients.client_lookup_count[i]+1;
          else
          {
            // client lookup counter REACHED CountMax, remove this client
            v_xboxclients.client_ip.erase(v_xboxclients.client_ip.begin()+i);
            v_xboxclients.client_info.erase(v_xboxclients.client_info.begin()+i);
            v_xboxclients.client_lookup_count.erase(v_xboxclients.client_lookup_count.begin()+i);
            v_xboxclients.client_informed.erase(v_xboxclients.client_informed.begin()+i);

            // debug log, clients removed from our list
            CLog::Log(LOGDEBUG,"Autodetection: Client ID:[%i] Removed! (mode LIFE 0)",i );
            bUpdateShares = true;
          }
        }
        if(bUpdateShares)
        {
          // a client is removed from our list, update our shares
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
          CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
        }
      }
    }
  }
  // life !=0 we are online and ready to receive and send
  while( life )
    {
    bFoundNewClient = false;
    bUpdateShares = false;
    // Receive ping request or Info
    int iSockRet = recvfrom(udp_server_socket, sztmp, 512, 0,(struct sockaddr *) &cliAddr, &cliLen);
    if (iSockRet != SOCKET_ERROR)
    {
      std::string strTmp;
      // do we received a new Client info or just a "ping" request
      if(strReceiveMessage == sztmp)
      {
        // we received a "ping" request, sending our informations
        strTmp = StringUtils::Format("%s;%s;%s;%d;%d\r\n\0",
          strNickName.c_str(),  // Our Nick-, Device Name!
          strFTPUserName.c_str(), // User Name for our FTP Server
          strFTPPass.c_str(), // Password for our FTP Server
          iFTPPort, // FTP PORT Adress for our FTP Server
          0 ); // BOOSMODE, for our FTP Server!
        sendto(udp_server_socket,(char *)strTmp.c_str(),strlen((char *)strTmp.c_str())+1,0,(struct sockaddr *)(&cliAddr),sizeof(cliAddr));
      }
      else
      {
        //We received new client information, extracting information
        std::string strInfo, strIP;
        strInfo = StringUtils::Format("%s",sztmp); //this is the client info
        strIP = StringUtils::Format("%d.%d.%d.%d",
#ifndef _LINUX
          cliAddr.sin_addr.S_un.S_un_b.s_b1,
          cliAddr.sin_addr.S_un.S_un_b.s_b2,
          cliAddr.sin_addr.S_un.S_un_b.s_b3,
          cliAddr.sin_addr.S_un.S_un_b.s_b4
#else
          (int)((char *)(cliAddr.sin_addr.s_addr))[0],
          (int)((char *)(cliAddr.sin_addr.s_addr))[1],
          (int)((char *)(cliAddr.sin_addr.s_addr))[2],
          (int)((char *)(cliAddr.sin_addr.s_addr))[3]
#endif
        ); //this is the client IP

        //Is this our Local IP ?
        if ( strIP != strLocalIP )
        {
          //is our list empty?
          if(v_xboxclients.client_ip.size() <= 0 )
          {
            // the list is empty, add. this client to the list!
            v_xboxclients.client_ip.push_back(strIP);
            v_xboxclients.client_info.push_back(strInfo);
            v_xboxclients.client_lookup_count.push_back(0);
            v_xboxclients.client_informed.push_back(false);
            bFoundNewClient = true;
            bUpdateShares = true;
          }
          // our list is not empty, check if we allready have this client in our list!
          else
          {
            // this should be a new client or?
            // check list
            bFoundNewClient = true;
            for (i=0; i<v_xboxclients.client_ip.size(); i++)
            {
              if(strIP == v_xboxclients.client_ip[i].c_str())
                bFoundNewClient=false;
            }
            if(bFoundNewClient)
            {
              // bFoundNewClient is still true, the client is not in our list!
              // add. this client to our list!
              v_xboxclients.client_ip.push_back(strIP);
              v_xboxclients.client_info.push_back(strInfo);
              v_xboxclients.client_lookup_count.push_back(0);
              v_xboxclients.client_informed.push_back(false);
              bUpdateShares = true;
            }
            else // this is a existing client! check for LIFE & lookup counter
            {
              // calculate iLookUpCountMax value counter dependence on clients size!
              if(v_xboxclients.client_ip.size() > iLookUpCountMax)
                iLookUpCountMax += (v_xboxclients.client_ip.size()-iLookUpCountMax);

              for (i=0; i<v_xboxclients.client_ip.size(); i++)
              {
                if(strIP == v_xboxclients.client_ip[i].c_str())
                {
                  // found client in list, reset looup_Count and the client_info
                  v_xboxclients.client_info[i]=strInfo;
                  v_xboxclients.client_lookup_count[i] = 0;
                }
                else
                {
                  // check client lookup counter! Not reached the CountMax, Add +1!
                  if(v_xboxclients.client_lookup_count[i] < iLookUpCountMax )
                    v_xboxclients.client_lookup_count[i] = v_xboxclients.client_lookup_count[i]+1;
                  else
                  {
                    // client lookup counter REACHED CountMax, remove this client
                    v_xboxclients.client_ip.erase(v_xboxclients.client_ip.begin()+i);
                    v_xboxclients.client_info.erase(v_xboxclients.client_info.begin()+i);
                    v_xboxclients.client_lookup_count.erase(v_xboxclients.client_lookup_count.begin()+i);
                    v_xboxclients.client_informed.erase(v_xboxclients.client_informed.begin()+i);

                    // debug log, clients removed from our list
                    CLog::Log(LOGDEBUG,"Autodetection: Client ID:[%i] Removed! (mode LIFE 1)",i );

                    // client is removed from our list, update our shares
                    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
                    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
                  }
                }
              }
              // here comes our list for debug log
              for (i=0; i<v_xboxclients.client_ip.size(); i++)
              {
                CLog::Log(LOGDEBUG,"Autodetection: Client ID:[%i] (mode LIFE=1)",i );
                CLog::Log(LOGDEBUG,"----------------------------------------------------------------" );
                CLog::Log(LOGDEBUG,"IP:%s Info:%s LookUpCount:%i Informed:%s",
                  v_xboxclients.client_ip[i].c_str(),
                  v_xboxclients.client_info[i].c_str(),
                  v_xboxclients.client_lookup_count[i],
                  v_xboxclients.client_informed[i] ? "true":"false");
                CLog::Log(LOGDEBUG,"----------------------------------------------------------------" );
              }
            }
          }
          if(bUpdateShares)
          {
            // a client is add or removed from our list, update our shares
            CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
            CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
          }
        }
      }
    }
    else
    {
       CLog::Log(LOGDEBUG, "Autodetection: Socket error %u", WSAGetLastError());
    }
    timeout.tv_sec=0;
    timeout.tv_usec = 5000;
    FD_ZERO(&readfds);
    FD_SET(udp_server_socket, &readfds);
    life = select( 0,&readfds, NULL, NULL, &timeout );
  }
  return bFoundNewClient;
}

void CUtil::AutoDetectionGetSource(VECSOURCES &shares)
{
  if(v_xboxclients.client_ip.size() > 0)
  {
    // client list is not empty, add to shares
    CMediaSource share;
    for (unsigned int i=0; i< v_xboxclients.client_ip.size(); i++)
    {
      //extract client info string: NickName;FTP_USER;FTP_Password;FTP_PORT;BOOST_MODE
      std::string strFTPPath, strNickName, strFtpUserName, strFtpPassword, strFtpPort, strBoosMode;
      std::vector<std::string> arSplit = StringUtils::Split(v_xboxclients.client_info[i],";");
      if ((int)arSplit.size() > 1)
      {
        strNickName     = arSplit[0].c_str();
        strFtpUserName  = arSplit[1].c_str();
        strFtpPassword  = arSplit[2].c_str();
        strFtpPort      = arSplit[3].c_str();
        strBoosMode     = arSplit[4].c_str();
        strFTPPath = StringUtils::Format("ftp://%s:%s@%s:%s/",strFtpUserName.c_str(),strFtpPassword.c_str(),v_xboxclients.client_ip[i].c_str(),strFtpPort.c_str());

        StringUtils::TrimRight(strNickName, " ");
#ifdef HAS_XBOX_HARDWARE
        share.strName = StringUtils::Format("FTP XBMC (%s)", strNickName.c_str());
#else
        share.strName = StringUtils::Format("FTP XBMC_PC (%s)", strNickName.c_str());
#endif
        share.strPath = StringUtils::Format("%s",strFTPPath.c_str());
        shares.push_back(share);
      }
    }
  }
}

//strXboxNickNameIn: New NickName to write
//strXboxNickNameOut: Same if it is in NICKNAME Cache
bool CUtil::SetXBOXNickName(std::string strXboxNickNameIn, std::string &strXboxNickNameOut)
{
#ifdef HAS_XBOX_HARDWARE
  WCHAR pszNickName[MAX_NICKNAME];
  unsigned int uiSize = MAX_NICKNAME;
  bool bfound= false;
  HANDLE hNickName = XFindFirstNickname(false,pszNickName,MAX_NICKNAME);
  if (hNickName != INVALID_HANDLE_VALUE)
  { do
      {
        strXboxNickNameOut = StringUtils::Format("%ls",pszNickName );
        if (strXboxNickNameIn == strXboxNickNameOut)
        {
          bfound = true;
          break;
        }
        else if (strXboxNickNameIn.empty()) strXboxNickNameOut = "XbMediaCenter";
      }while(XFindNextNickname(hNickName,pszNickName,uiSize) != false);
    XFindClose(hNickName);
  }
  if(!bfound)
  {
    std::wstring wstrName;
    g_charsetConverter.utf8ToW(strXboxNickNameIn, wstrName);
    XSetNickname(wstrName.c_str(), false);
  }
#endif
  return true;
}
//strXboxNickNameOut: Will fast receive the last XBOX NICKNAME from Cache
bool CUtil::GetXBOXNickName(std::string &strXboxNickNameOut)
{
#ifdef HAS_XBOX_HARDWARE
  WCHAR wszXboxNickname[MAX_NICKNAME];
  HANDLE hNickName = XFindFirstNickname( FALSE, wszXboxNickname, MAX_NICKNAME );
    if ( hNickName != INVALID_HANDLE_VALUE )
    {
    strXboxNickNameOut = StringUtils::Format("%ls",wszXboxNickname);
        XFindClose( hNickName );
    return true;
    }
  else
#endif
  {
    // it seems to be empty? should we create one? or the user
    strXboxNickNameOut = "";
    return false;
  }
}

void CUtil::GetRecursiveListing(const std::string& strPath, CFileItemList& items, const std::string& strMask, unsigned int flags /* = DIR_FLAG_DEFAULTS */)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,strMask,flags);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder)
      CUtil::GetRecursiveListing(myItems[i]->GetPath(),items,strMask,flags);
    else
//    if (!myItems[i]->IsRAR() && !myItems[i]->IsZIP())
      items.Add(myItems[i]);
  }
}

void CUtil::GetRecursiveDirsListing(const std::string& strPath, CFileItemList& item, unsigned int flags /* = DIR_FLAG_DEFAULTS */)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,"",flags);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder && !myItems[i]->IsPath(".."))
    {
      item.Add(myItems[i]);
      CUtil::GetRecursiveDirsListing(myItems[i]->GetPath(),item,flags);
    }
  }
}

void CUtil::ForceForwardSlashes(std::string& strPath)
{
  int iPos = strPath.rfind('\\');
  while (iPos > 0)
  {
    strPath.at(iPos) = '/';
    iPos = strPath.rfind('\\');
  }
}

double CUtil::AlbumRelevance(const std::string& strAlbumTemp1, const std::string& strAlbum1, const std::string& strArtistTemp1, const std::string& strArtist1)
{
  // case-insensitive fuzzy string comparison on the album and artist for relevance
  // weighting is identical, both album and artist are 50% of the total relevance
  // a missing artist means the maximum relevance can only be 0.50
  std::string strAlbumTemp = strAlbumTemp1;
  StringUtils::ToLower(strAlbumTemp);
  std::string strAlbum = strAlbum1;
  StringUtils::ToLower(strAlbum);
  double fAlbumPercentage = fstrcmp(strAlbumTemp.c_str(), strAlbum.c_str(), 0.0f);
  double fArtistPercentage = 0.0f;
  if (!strArtist1.empty())
  {
    std::string strArtistTemp = strArtistTemp1;
    StringUtils::ToLower(strArtistTemp);
    std::string strArtist = strArtist1;
    StringUtils::ToLower(strArtist);
    fArtistPercentage = fstrcmp(strArtistTemp.c_str(), strArtist.c_str(), 0.0f);
  }
  double fRelevance = fAlbumPercentage * 0.5f + fArtistPercentage * 0.5f;
  return fRelevance;
}

bool CUtil::MakeShortenPath(std::string StrInput, std::string& StrOutput, size_t iTextMaxLength)
{
  size_t iStrInputSize = StrInput.size();
  if(iStrInputSize <= 0 || iTextMaxLength >= iStrInputSize)
  {
    StrOutput = StrInput;
    return true;
  }

  char cDelim = '\0';
  size_t nGreaterDelim, nPos;

  nPos = StrInput.find_last_of( '\\' );
  if (nPos != std::string::npos)
    cDelim = '\\';
  else
  {
    nPos = StrInput.find_last_of( '/' );
    if (nPos != std::string::npos)
      cDelim = '/';
  }
  if ( cDelim == '\0' )
    return false;

  if (nPos == StrInput.size() - 1)
  {
    StrInput.erase(StrInput.size() - 1);
    nPos = StrInput.find_last_of(cDelim);
  }
  while( iTextMaxLength < iStrInputSize )
  {
    nPos = StrInput.find_last_of( cDelim, nPos );
    nGreaterDelim = nPos;

    if (nPos == std::string::npos || nPos == 0)
      break;

    nPos = StrInput.find_last_of( cDelim, nPos - 1 );

    if ( nPos == std::string::npos)
      break;
    if ( nGreaterDelim > nPos ) StrInput.replace( nPos + 1, nGreaterDelim - nPos - 1, ".." );
    iStrInputSize = StrInput.size();
  }
  // replace any additional /../../ with just /../ if necessary
  std::string replaceDots = StringUtils::Format("..%c..", cDelim);
  while (StrInput.size() > (unsigned int)iTextMaxLength)
    if (!StringUtils::Replace(StrInput, replaceDots, ".."))
      break;
  // finally, truncate our string to force inside our max text length,
  // replacing the last 2 characters with ".."

  // eg end up with:
  // "smb://../Playboy Swimsuit Cal.."
  if (iTextMaxLength > 2 && StrInput.size() > (unsigned int)iTextMaxLength)
  {
    StrInput.erase(iTextMaxLength - 2);
    StrInput += "..";
  }
  StrOutput = StrInput;
  return true;
}

bool CUtil::SupportsWriteFileOperations(const std::string& strPath)
{
  // currently only hd,smb and dav support delete and rename
  if (URIUtils::IsHD(strPath))
    return true;
  if (URIUtils::IsSmb(strPath))
    return true;
  if (URIUtils::IsDAV(strPath))
    return true;
  if (URIUtils::IsStack(strPath))
    return SupportsWriteFileOperations(CStackDirectory::GetFirstStackedFile(strPath));
  if (URIUtils::IsMultiPath(strPath))
    return CMultiPathDirectory::SupportsWriteFileOperations(strPath);
#ifdef HAS_XBOX_HARDWARE
  if (URIUtils::IsMemoryCard(strPath) && g_memoryUnitManager.IsDriveWriteable(strPath))
    return true;
#endif
  return false;
}

bool CUtil::SupportsReadFileOperations(const std::string& strPath)
{
  if (URIUtils::IsVideoDb(strPath))
    return false;

  return true;
}

std::string CUtil::GetDefaultFolderThumb(const std::string &folderThumb)
{
  if (CServiceBroker::GetGUI()->GetTextureManager().HasTexture(folderThumb))
    return folderThumb;
  return "";
}

void CUtil::GetSkinThemes(std::vector<std::string>& vecTheme)
{
  std::string strPath = URIUtils::AddFileToFolder(CServiceBroker::GetWinSystem()->GetGfxContext().GetMediaDir(), "media");
  CFileItemList items;
  CDirectory::GetDirectory(strPath, items, "", DIR_FLAG_DEFAULTS);
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      std::string strExtension = URIUtils::GetExtension(pItem->GetPath());
      if (strExtension == ".xpr" && StringUtils::EqualsNoCase(pItem->GetLabel(), "Textures.xpr"))
      {
        std::string strLabel = pItem->GetLabel();
        vecTheme.push_back(strLabel.substr(0, strLabel.size() - 4));
      }
    }
  }
  sort(vecTheme.begin(), vecTheme.end(), sortstringbyname());
}

void CUtil::WipeDir(const std::string& strPath) // DANGEROUS!!!!
{
  if (!CDirectory::Exists(strPath)) return;

  CFileItemList items;
  GetRecursiveListing(strPath,items,"");
  for (int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
      CFile::Delete(items[i]->GetPath());
  }
  items.Clear();
  GetRecursiveDirsListing(strPath,items);
  for (int i=items.Size()-1;i>-1;--i) // need to wipe them backwards
  {
    std::string strDir = items[i]->GetPath();
    URIUtils::AddSlashAtEnd(strDir);
    CDirectory::Remove(strDir);
  }

  if (!URIUtils::HasSlashAtEnd(strPath))
  {
    std::string tmpPath = strPath;
    URIUtils::AddSlashAtEnd(tmpPath);
    CDirectory::Remove(tmpPath);
  }
}

bool CUtil::PWMControl(const std::string &strRGBa, const std::string &strRGBb, const std::string &strWhiteA, const std::string &strWhiteB, const std::string &strTransition, int iTrTime)
{
#ifdef HAS_XBOX_HARDWARE
    if (strRGBa.empty() && strRGBb.empty() && strWhiteA.empty() && strWhiteB.empty()) // no color, return false!
      return false;
  if(g_iledSmartxxrgb.IsRunning())
  {
    return g_iledSmartxxrgb.SetRGBState(strRGBa,strRGBb, strWhiteA, strWhiteB, strTransition, iTrTime);
  }
  g_iledSmartxxrgb.Start();
  return g_iledSmartxxrgb.SetRGBState(strRGBa,strRGBb, strWhiteA, strWhiteB, strTransition, iTrTime);
#else
  return false;
#endif
}

// We check if the MediaCenter-Video-patch is already installed.
// To do this we search for the original code in the Kernel.
// This is done by searching from 0x80011000 to 0x80024000.
bool CUtil::LookForKernelPatch()
{
#ifdef HAS_XBOX_HARDWARE
  BYTE    *Kernel=(BYTE *)0x80010000;
  DWORD    i, j = 0;

  for(i=0x1000; i<0x14000; i++)
  {
    if(Kernel[i]!=PatchData[0])
      continue;
    for(j=0; j<25; j++)
    {
      if(Kernel[i+j]!=PatchData[j])
        break;
    }
    if(j==25)
      return true;
  }
#endif
  return false;
}
// This routine removes our patch if it is not used.
// This is to ensure proper testing whether we are responsible
// for a mismatch of eeprom setting and current resolution.
void CUtil::RemoveKernelPatch()
{
#ifdef HAS_XBOX_HARDWARE
  BYTE  *Kernel=(BYTE *)0x80010000;
  DWORD i, j = 0;

  for(i=0x1000; i<0x14000; i++)
  {
    if(Kernel[i]!=PatchData[0])
      continue;

    for(j=0; j<25; j++)
    {
      if(Kernel[i+j]!=PatchData[j])
        break;
    }
    if(j==25)
    {
      j=MmQueryAddressProtect(&Kernel[i]);
      MmSetAddressProtect(&Kernel[i], 70, PAGE_READWRITE);
      memcpy(&Kernel[i], &rawData[0], 70); // Reset Kernel
      MmSetAddressProtect(&Kernel[i], 70, j);
    }
  }
#endif
}

void CUtil::BootToDash()
{
#ifdef HAS_XBOX_HARDWARE
  LD_LAUNCH_DASHBOARD ld;

  ZeroMemory(&ld, sizeof(LD_LAUNCH_DASHBOARD));

  ld.dwReason = XLD_LAUNCH_DASHBOARD_MAIN_MENU;
  XLaunchNewImage(0, (PLAUNCH_DATA)&ld);
#endif
}

void CUtil::InitRandomSeed()
{
  // Init random seed
  int64_t now;
  now = CurrentHostCounter();
  unsigned int seed = (unsigned int)now;
//  CLog::Log(LOGDEBUG, "%s - Initializing random seed with %u", __FUNCTION__, seed);
  srand(seed);
}

void CUtil::RunShortcut(const char* szShortcutPath)
{
  CShortcut shortcut;
  char szPath[1024];
  char szParameters[1024];
  if ( shortcut.Create(szShortcutPath))
  {
    CFileItem item(shortcut.m_strPath, false);
    // if another shortcut is specified, load this up and use it
    if (item.IsShortCut())
    {
      CHAR szNewPath[1024];
      strcpy(szNewPath, szShortcutPath);
      CHAR* szFile = strrchr(szNewPath, '\\');
      strcpy(&szFile[1], shortcut.m_strPath.c_str());

      CShortcut targetShortcut;
      if (FAILED(targetShortcut.Create(szNewPath)))
        return;

      shortcut.m_strPath = targetShortcut.m_strPath;
    }

    strcpy( szPath, shortcut.m_strPath.c_str() );

    CHAR szMode[16];
    strcpy( szMode, shortcut.m_strVideo.c_str() );
    strlwr( szMode );

    strcpy(szParameters, shortcut.m_strParameters.c_str());

    BOOL bRow = strstr(szMode, "pal") != NULL;
    BOOL bJap = strstr(szMode, "ntsc-j") != NULL;
    BOOL bUsa = strstr(szMode, "ntsc") != NULL;

    F_VIDEO video = VIDEO_NULL;
    if (bRow)
      video = VIDEO_PAL50;
    if (bJap)
      video = (F_VIDEO)CXBE::FilterRegion(VIDEO_NTSCJ);
    if (bUsa)
      video = (F_VIDEO)CXBE::FilterRegion(VIDEO_NTSCM);

#ifdef HAS_XBOX_HARDWARE
    CUSTOM_LAUNCH_DATA data;
    if (!shortcut.m_strCustomGame.empty())
    {
      char remap_path[MAX_PATH] = "";
      char remap_xbe[MAX_PATH] = "";

      memset(&data,0,sizeof(CUSTOM_LAUNCH_DATA));

      strcpy(data.szFilename,shortcut.m_strCustomGame.c_str());

      strncpy(remap_path, XeImageFileName->Buffer, XeImageFileName->Length);
      for (int i = strlen(remap_path) - 1; i >=0; i--)
        if (remap_path[i] == '\\' || remap_path[i] == '/')
        {
          break;
        }
      strcpy(remap_xbe, &remap_path[i+1]);
      remap_path[i+1] = 0;

      strcpy(data.szRemap_D_As, remap_path);
      strcpy(data.szLaunchXBEOnExit, remap_xbe);

      data.executionType = 0;

      // not the actual "magic" value - used to pass XbeId for some reason?
      data.magic = GetXbeID(szPath);
    }

    CUtil::RunXBE(szPath,strcmp(szParameters,"")?szParameters:NULL,video,COUNTRY_NULL,shortcut.m_strCustomGame.empty()?NULL:&data);
#endif
  }
}

void CUtil::GetHomePath(std::string& strPath)
{
  char szXBEFileName[1024];
  CIoSupport::GetXbePath(szXBEFileName);
  char *szFileName = strrchr(szXBEFileName, '\\');
  *szFileName = 0;
  strPath = szXBEFileName;
}

bool CUtil::RunFFPatchedXBE(std::string szPath1, std::string& szNewPath)
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("myprograms.autoffpatch"))
  {
    CLog::Log(LOGDEBUG, "%s - Auto Filter Flicker is off. Skipping Filter Flicker Patching.", __FUNCTION__);
    return false;
  }
  std::string strIsPMode = CDisplaySettings::GetInstance().GetCurrentResolutionInfo().strMode;
  if ( strIsPMode == "480p 16:9" || strIsPMode == "480p 4:3" || strIsPMode == "720p 16:9")
  {
    CLog::Log(LOGDEBUG, "%s - Progressive Mode detected: Skipping Auto Filter Flicker Patching!", __FUNCTION__);
    return false;
  }
  if (strncmp(szPath1.c_str(), "D:", 2) == 0)
  {
    CLog::Log(LOGDEBUG, "%s - Source is DVD-ROM! Skipping Filter Flicker Patching.", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s - Auto Filter Flicker is ON. Starting Filter Flicker Patching.", __FUNCTION__);

  // Test if we already have a patched _ffp XBE
  // Since the FF can be changed in XBMC, we will not check for a pre patched _ffp xbe!
  /* // May we can add. a changed FF detection.. then we can actived this!
  CFile    xbe;
    if (xbe.Exists(szPath1))
  {
    char szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szFname[_MAX_FNAME], szExt[_MAX_EXT];
        _splitpath(szPath1, szDrive, szDir, szFname, szExt);
        strncat(szFname, "_ffp", 4);
        _makepath(szNewPath.GetBuffer(MAX_PATH), szDrive, szDir, szFname, szExt);
        szNewPath.ReleaseBuffer();
        if (xbe.Exists(szNewPath))
            return true;
    } */


  CXBE m_xbe;
  if((int)m_xbe.ExtractGameRegion(szPath1.c_str()) <= 0) // Reading the GameRegion is enought to detect a Patchable xbe!
  {
    CLog::Log(LOGDEBUG, "%s - %s",szPath1.c_str(), __FUNCTION__);
    CLog::Log(LOGDEBUG, "%s - Not Patchable xbe detected (Homebrew?)! Skipping Filter Flicker Patching.", __FUNCTION__);
    return false;
  }
#ifdef HAS_XBOX_HARDWARE
  CGFFPatch m_ffp;
  if (!m_ffp.FFPatch(szPath1, szNewPath))
  {
    CLog::Log(LOGDEBUG, "%s - ERROR during Filter Flicker Patching. Falling back to the original source.", __FUNCTION__);
    return false;
  }
#endif
  if(szNewPath.empty())
  {
    CLog::Log(LOGDEBUG, "%s - ERROR NO Patchfile Path is empty! Falling back to the original source.", __FUNCTION__);
    return false;
  }
  CLog::Log(LOGDEBUG, "%s - Filter Flicker Patching done. Starting %s.", __FUNCTION__, szNewPath.c_str());
  return true;
}

void CUtil::RunXBE(const char* szPath1, char* szParameters, F_VIDEO ForceVideo, F_COUNTRY ForceCountry, CUSTOM_LAUNCH_DATA* pData)
{
  // check if locked
  if (CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetCurrentProfile().programsLocked() &&
      CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;

  /// \brief Runs an executable file
  /// \param szPath1 Path of executeable to run
  /// \param szParameters Any parameters to pass to the executeable being run
  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationXbox> appXbox = components.GetComponent<CApplicationXbox>();
  appXbox->PrintXBETitleToLCD(szPath1); //write to LCD
  Sleep(600);        //and wait a little bit to execute

  char szPath[1024];
  strcpy(szPath, CSpecialProtocol::TranslatePath(szPath1).c_str());

  std::string szNewPath;
  if (RunFFPatchedXBE(szPath, szNewPath))
  {
    strcpy(szPath, szNewPath.c_str());
  }

  if (strncmp(szPath, "Q:", 2) == 0)
  { // may aswell support the virtual drive as well...
    std::string strPath;
    // home dir is xbe dir
    GetHomePath(strPath);
    if (!URIUtils::HasSlashAtEnd(strPath))
      strPath += "\\";
    if (szPath[2] == '\\')
      strPath += szPath + 3;
    else
      strPath += szPath + 2;
    strcpy(szPath, strPath.c_str());
  }

  char* szBackslash = strrchr(szPath, '\\');
  if (szBackslash)
  {
    *szBackslash = 0x00;
    char* szXbe = &szBackslash[1];

    char* szColon = strrchr(szPath, ':');
    if (szColon)
    {
      *szColon = 0x00;
      char* szDrive = szPath;
      char* szDirectory = &szColon[1];

      char szDevicePath[1024];
      char szXbePath[1024];

      CIoSupport::GetPartition(szDrive[0], szDevicePath);

      strcat(szDevicePath, szDirectory);
      wsprintf(szXbePath, "d:\\%s", szXbe);

#ifdef HAS_XBOX_HARDWARE
      g_application.Stop(false);

      CUtil::LaunchXbe(szDevicePath, szXbePath, szParameters, ForceVideo, ForceCountry, pData);
#endif
    }
  }

  CLog::Log(LOGERROR, "Unable to run xbe : %s", szPath);
}

void CUtil::LaunchXbe(const char* szPath, const char* szXbe, const char* szParameters, F_VIDEO ForceVideo, F_COUNTRY ForceCountry, CUSTOM_LAUNCH_DATA* pData)
{
  std::string strPath(CSpecialProtocol::TranslatePath(szPath));
  CLog::Log(LOGINFO, "launch xbe:%s %s", strPath.c_str(), szXbe);
  CLog::Log(LOGINFO, " mount %s as D:", strPath.c_str());

#ifdef HAS_XBOX_HARDWARE
  CIoSupport::RemapDriveLetter('D', const_cast<char*>(strPath.c_str()));

  CLog::Log(LOGINFO, "launch xbe:%s", szXbe);

  if (ForceVideo != VIDEO_NULL)
  {
    if (!ForceCountry)
    {
      if (ForceVideo == VIDEO_NTSCM)
        ForceCountry = COUNTRY_USA;
      if (ForceVideo == VIDEO_NTSCJ)
        ForceCountry = COUNTRY_JAP;
      if (ForceVideo == VIDEO_PAL50)
        ForceCountry = COUNTRY_EUR;
    }
    CLog::Log(LOGDEBUG,"forcing video mode: %i",ForceVideo);
    bool bSuccessful = PatchCountryVideo(ForceCountry, ForceVideo);
    if( !bSuccessful )
      CLog::Log(LOGINFO,"AutoSwitch: Failed to set mode");
  }
  if (pData)
  {
    DWORD dwTitleID = pData->magic;
    pData->magic = CUSTOM_LAUNCH_MAGIC;
    const char* xbe = szXbe+3;
    CLog::Log(LOGINFO, "launching game %s from path %s", pData->szFilename, strPath.c_str());
    CIoSupport::UnmapDriveLetter('D');
    XWriteTitleInfoAndRebootA( (char*)xbe, (char*)(std::string("\\Device\\")+strPath).c_str(), LDT_TITLE, dwTitleID, pData);
  }
  else
  {
    if (szParameters == NULL)
    {
      DWORD error = XLaunchNewImage(szXbe, NULL );
      CLog::Log(LOGERROR, "%s - XLaunchNewImage returned with error code %d", __FUNCTION__, error);
    }
    else
    {
      LAUNCH_DATA LaunchData;
      strcpy((char*)LaunchData.Data, szParameters);

      DWORD error = XLaunchNewImage(szXbe, &LaunchData );
      CLog::Log(LOGERROR, "%s - XLaunchNewImage returned with error code %d", __FUNCTION__, error);
    }
  }
#endif
}

int CUtil::LookupRomanDigit(char roman_digit)
{
  switch (roman_digit)
  {
    case 'i':
    case 'I':
      return 1;
    case 'v':
    case 'V':
      return 5;
    case 'x':
    case 'X':
      return 10;
    case 'l':
    case 'L':
      return 50;
    case 'c':
    case 'C':
      return 100;
    case 'd':
    case 'D':
      return 500;
    case 'm':
    case 'M':
      return 1000;
    default:
      return 0;
  }
}

int CUtil::TranslateRomanNumeral(const char* roman_numeral)
{

  int decimal = -1;

  if (roman_numeral && roman_numeral[0])
  {
    int temp_sum  = 0,
        last      = 0,
        repeat    = 0,
        trend     = 1;
    decimal = 0;
    while (*roman_numeral)
    {
      int digit = CUtil::LookupRomanDigit(*roman_numeral);
      int test  = last;

      // General sanity checks

      // numeral not in LUT
      if (!digit)
        return -1;

      while (test > 5)
        test /= 10;

      // N = 10^n may not precede (N+1) > 10^(N+1)
      if (test == 1 && digit > last * 10)
        return -1;

      // N = 5*10^n may not precede (N+1) >= N
      if (test == 5 && digit >= last)
        return -1;

      // End general sanity checks

      if (last < digit)
      {
        // smaller numerals may not repeat before a larger one
        if (repeat)
          return -1;

        temp_sum += digit;

        repeat  = 0;
        trend   = 0;
      }
      else if (last == digit)
      {
        temp_sum += digit;
        repeat++;
        trend = 1;
      }
      else
      {
        if (!repeat)
          decimal += 2 * last - temp_sum;
        else
          decimal += temp_sum;

        temp_sum = digit;

        trend   = 1;
        repeat  = 0;
      }
      // Post general sanity checks

      // numerals may not repeat more than thrice
      if (repeat == 3)
        return -1;

      last = digit;
      roman_numeral++;
    }

    if (trend)
      decimal += temp_sum;
    else
      decimal += 2 * last - temp_sum;
  }
  return decimal;
}

void CUtil::GetVideoBasePathAndFileName(const std::string& videoPath, std::string& basePath, std::string& videoFileName)
{
  CFileItem item(videoPath, false);
  videoFileName = URIUtils::ReplaceExtension(URIUtils::GetFileName(videoPath), "");

  if (item.HasVideoInfoTag())
    basePath = item.GetVideoInfoTag()->m_basePath;

  if (basePath.empty() && item.IsOpticalMediaFile())
  {
    videoFileName = item.GetMovieName();
    basePath = item.GetLocalMetadataPath();
  }

  if (basePath.empty())
    basePath = URIUtils::GetBasePath(videoPath);
}

void CUtil::GetItemsToScan(const std::string& videoPath,
                           const std::string& item_exts,
                           const std::vector<std::string>& sub_dirs,
                           CFileItemList& items)
{
  int flags = DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_NO_FILE_INFO;

  if (!videoPath.empty())
    CDirectory::GetDirectory(videoPath, items, item_exts, flags);

  std::vector<std::string> additionalPaths;
  for (int i = 0; i < items.Size(); ++i)
  {
    for (std::vector<std::string>::const_iterator subdir = sub_dirs.begin(); subdir != sub_dirs.end(); ++subdir)
    {
      if (StringUtils::EqualsNoCase(items[i]->GetLabel(), *subdir))
        additionalPaths.push_back(items[i]->GetPath());
    }
  }

  for (std::vector<std::string>::const_iterator it = additionalPaths.begin(); it != additionalPaths.end(); ++it)
  {
    CFileItemList moreItems;
    CDirectory::GetDirectory(*it, moreItems, item_exts, flags);
    items.Append(moreItems);
  }
}


void CUtil::ScanPathsForAssociatedItems(const std::string& videoName,
                                        const CFileItemList& items,
                                        const std::vector<std::string>& item_exts,
                                        std::vector<std::string>& associatedFiles)
{
  for (int i = 0; i < items.Size(); ++i)
  {
    const CFileItemPtr &pItem = items[i];
    if (pItem->m_bIsFolder)
      continue;

    std::string strCandidate = URIUtils::GetFileName(pItem->GetPath());

    // skip duplicates
    if (std::find(associatedFiles.begin(), associatedFiles.end(), pItem->GetPath()) != associatedFiles.end())
      continue;

    URIUtils::RemoveExtension(strCandidate);
    // NOTE: We don't know if one of videoName or strCandidate is URL-encoded and the other is not, so try both
    if (StringUtils::StartsWithNoCase(strCandidate, videoName) || (StringUtils::StartsWithNoCase(strCandidate, CURL::Decode(videoName))))
    {
      if (URIUtils::IsRAR(pItem->GetPath()) || URIUtils::IsZIP(pItem->GetPath()))
        CUtil::ScanArchiveForAssociatedItems(pItem->GetPath(), "", item_exts, associatedFiles);
      else
      {
        associatedFiles.push_back(pItem->GetPath());
        CLog::Log(LOGINFO, "%s: found associated file %s", __FUNCTION__,
                  CURL::GetRedacted(pItem->GetPath()).c_str());
      }
    }
    else
    {
      if (URIUtils::IsRAR(pItem->GetPath()) || URIUtils::IsZIP(pItem->GetPath()))
        CUtil::ScanArchiveForAssociatedItems(pItem->GetPath(), videoName, item_exts, associatedFiles);
    }
  }
}

int CUtil::ScanArchiveForAssociatedItems(const std::string& strArchivePath,
                                         const std::string& videoNameNoExt,
                                         const std::vector<std::string>& item_exts,
                                         std::vector<std::string>& associatedFiles)
{
  CLog::Log(LOGDEBUG, "Scanning archive %s", CURL::GetRedacted(strArchivePath).c_str());
  int nItemsAdded = 0;
  CFileItemList ItemList;

  // zip only gets the root dir
  if (URIUtils::HasExtension(strArchivePath, ".zip"))
  {
    CURL pathToUrl(strArchivePath);
    CURL zipURL = URIUtils::CreateArchivePath("zip", pathToUrl, "");
    if (!CDirectory::GetDirectory(zipURL, ItemList, "", DIR_FLAG_NO_FILE_DIRS))
      return false;
  }
  else if (URIUtils::HasExtension(strArchivePath, ".rar"))
  {
    CURL pathToUrl(strArchivePath);
    CURL rarURL = URIUtils::CreateArchivePath("rar", pathToUrl, "");
    if (!CDirectory::GetDirectory(rarURL, ItemList, "", DIR_FLAG_NO_FILE_DIRS))
      return false;
  }
  for (int i = ItemList.Size(); i < ItemList.Size(); ++i)
  {
    std::string strPathInRar = ItemList[i]->GetPath();
    std::string strExt = URIUtils::GetExtension(strPathInRar);

    // Check another archive in archive
    if (strExt == ".zip" || strExt == ".rar")
    {
      nItemsAdded +=
          ScanArchiveForAssociatedItems(strPathInRar, videoNameNoExt, item_exts, associatedFiles);
      continue;
    }

    // check that the found filename matches the movie filename
    size_t fnl = videoNameNoExt.size();
    // NOTE: We don't know if videoNameNoExt is URL-encoded, so try both
    if (fnl &&
      !(StringUtils::StartsWithNoCase(URIUtils::GetFileName(strPathInRar), videoNameNoExt) ||
        StringUtils::StartsWithNoCase(URIUtils::GetFileName(strPathInRar), CURL::Decode(videoNameNoExt))))
      continue;

    for (std::vector<std::string>::const_iterator ext = item_exts.begin(); ext != item_exts.end(); ++ext)
    {
      if (StringUtils::EqualsNoCase(strExt, *ext))
      {
        CLog::Log(LOGINFO, "%s: found associated file %s", __FUNCTION__,
                  CURL::GetRedacted(strPathInRar).c_str());
        associatedFiles.push_back(strPathInRar);
        nItemsAdded++;
        break;
      }
    }
  }

  return nItemsAdded;
}

bool CUtil::CanBindPrivileged()
{
  // we can bind to any port on non-Unix systems
  return true;
}

bool CUtil::ValidatePort(int port)
{
  // check that it's a valid port
  if (port <= 0 || port > 65535)
    return false;

  return true;
}

int CUtil::GetRandomNumber()
{
#ifdef TARGET_WINDOWS
  unsigned int number;
  if (rand_s(&number) == 0)
    return (int)number;
#elif defined (_XBOX)
  srand((uint32_t)XbmcThreads::SystemClockMillis());
#else
  return rand_r(&s_randomSeed);
#endif

  return rand();
}

void CUtil::CopyUserDataIfNeeded(const std::string& strPath,
                                 const std::string& file,
                                 const std::string& destname)
{
  std::string destPath;
  if (destname.empty())
    destPath = URIUtils::AddFileToFolder(strPath, file);
  else
    destPath = URIUtils::AddFileToFolder(strPath, destname);

  if (!CFile::Exists(destPath))
  {
    // need to copy it across
    std::string srcPath = URIUtils::AddFileToFolder("special://xbmc/home/userdata/", file);
    CFile::Copy(srcPath, destPath);
  }
}
