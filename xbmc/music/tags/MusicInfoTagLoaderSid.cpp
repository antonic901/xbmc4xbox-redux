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

#include "MusicInfoTagLoaderSid.h"
#include "utils/RegExp.h"
#include "utils/log.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "filesystem/SpecialProtocol.h"

#include <cstring>

#include <fstream>

using namespace std;
using namespace MUSIC_INFO;

CMusicInfoTagLoaderSid::CMusicInfoTagLoaderSid(void)
{
}

CMusicInfoTagLoaderSid::~CMusicInfoTagLoaderSid()
{
}

bool CMusicInfoTagLoaderSid::Load(const CStdString& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
{
  CStdString strFileToLoad = strFileName;
  int iTrack = 0;
  if (URIUtils::HasExtension(strFileName, ".sidstream"))
  {
    //  Extract the track to play
    CStdString strFile=URIUtils::GetFileName(strFileName);
    int iStart=strFile.ReverseFind("-")+1;
    iTrack = atoi(strFile.substr(iStart, strFile.size()-iStart-10).c_str());
    //  The directory we are in, is the file
    //  that contains the bitstream to play,
    //  so extract it
    CStdString strPath=strFileName;
    strFileToLoad = URIUtils::GetDirectory(strPath);
    URIUtils::RemoveSlashAtEnd(strFileToLoad);   // we want the filename
  }
  CStdString strFileNameLower(strFileToLoad);
  strFileNameLower.MakeLower();
  int iHVSC = strFileNameLower.find("hvsc"); // need hvsc in path name since our lookupfile is based on hvsc paths
  if (iHVSC < 0)
  {
    iHVSC = strFileNameLower.find("c64music");
    if (iHVSC >= 0)
      iHVSC += 8;
  }
  else
    iHVSC += 4;
  if( iHVSC < 0 )
  {
    tag.SetLoaded(false);
    return( false );
  }

  CStdString strHVSCpath = strFileToLoad.substr(iHVSC,strFileToLoad.length()-1);

  strHVSCpath.Replace('\\','/'); // unix paths
  strHVSCpath.MakeLower();

  char temp[8192];
  CRegExp reg;
  if (!reg.RegComp("TITLE: ([^\r\n]*)\r?\n[^A]*ARTIST: ([^\r\n]*)\r?\n"))
  {
    CLog::Log(LOGINFO,"MusicInfoTagLoaderSid::Load(..): failed to compile regular expression");
    tag.SetLoaded(false);
    return( false );
  }

  ifstream f(CSpecialProtocol::TranslatePath(URIUtils::AddFileToFolder(CProfilesManager::Get().GetDatabaseFolder(), "stil.txt")).c_str()); // changeme?
  if( !f.good() ) {
    CLog::Log(LOGINFO,"MusicInfoTagLoaderSid::Load(..) unable to locate stil.txt");
    tag.SetLoaded(false);
    return( false );
  }

  const char* szStart = NULL;
  const char* szEnd = NULL;
  char temp2[8191];
  char* temp3 = temp2;
  while( !f.eof() && !szEnd )
  {
    f.read(temp,8191);
    CStdString strLower = temp;
    strLower.MakeLower();

    if (!szStart)
      szStart= (char *)strstr(strLower.c_str(),strHVSCpath.c_str());
    if (szStart)
    {
      szEnd = strstr(szStart+strHVSCpath.size(),".sid");
      if (szEnd)
      {
        memcpy(temp3,temp+(szStart-strLower.c_str()),szEnd-szStart);
        temp3 += szEnd-szStart;
      }
      else
      {
        memcpy(temp3,temp+(szStart-strLower.c_str()),strlen(szStart));
        szStart = NULL;
        temp3 += strlen(szStart);
      }
    }
  }
  f.close();

  if (!f.eof() && szEnd)
  {
    temp2[temp3-temp2] = '\0';
    temp3 = strstr(temp2,"TITLE:");
  }
  else
    temp3 = NULL;
  if (temp3)
  {
    for (int i=0;i<iTrack-1;++i) // skip tracks
    {
      int iStart = reg.RegFind(temp3);
      if (!iStart)
      {
        tag.SetLoaded(false);
        return false;
      }
      temp3 += iStart;
    }
    if(reg.RegFind(temp3) > -1)
    {
      std::string strTitle = reg.GetReplaceString("\\1");
      std::string strArtist = reg.GetReplaceString("\\2");
      std::string strMins;
      std::string strSecs;
      CRegExp reg2;
      reg2.RegComp("(.*) \\(([0-9]*):([0-9]*)\\)");
      if (reg2.RegFind(strTitle.c_str()) > -1)
      {
        strMins = reg2.GetReplaceString("\\2");
        strSecs = reg2.GetReplaceString("\\3");
        std::string strTemp = reg2.GetReplaceString("\\1");
        strTitle = strTemp;
      }
      tag.SetLoaded(true);
      tag.SetURL(strFileToLoad);
      tag.SetTrackNumber(iTrack);
      if (!strMins.empty() && !strSecs.empty())
        tag.SetDuration(atoi(strMins.c_str())*60+atoi(strSecs.c_str()));

      tag.SetTitle(strTitle);
      tag.SetArtist(strArtist);
    }
  }

  ifstream f2(CSpecialProtocol::TranslatePath(URIUtils::AddFileToFolder(CProfilesManager::Get().GetDatabaseFolder(),"sidlist.csv")).c_str()); // changeme?
  if( !f2.good() ) {
    CLog::Log(LOGINFO,"MusicInfoTagLoaderSid::Load(..) unable to locate sidlist.csv");
    tag.SetLoaded(false);
    return( false );
  }

  while( !f2.eof() ) {
    f2.getline(temp,8191);
    CStdString strTemp(temp);
    strTemp.MakeLower();
    unsigned int iFind = strTemp.find(strHVSCpath);
    if (iFind == string::npos)
      continue;

    char temp2[1024];
    char temp3[1024];
    strncpy(temp3,temp+iFind,strlen(strHVSCpath));
    temp3[strlen(strHVSCpath)] = '\0';
    sprintf(temp2,"\"%s\",\"[^\"]*\",\"[^\"]*\",\"([^\"]*)\",\"([^\"]*)\",\"([0-9]*)[^\"]*\",\"[0-9]*\",\"[0-9]*\",\"",temp3);
    for (int i=0;i<iTrack-1;++i)
      strcat(temp2,"[0-9]*:[0-9]* ");
    strcat(temp2,"([0-9]*):([0-9]*)");
    if( !reg.RegComp(temp2) )
    {
      CLog::Log(LOGINFO,"MusicInfoTagLoaderSid::Load(..): failed to compile regular expression");
      tag.SetLoaded(false);
      return( false );
    }
    if( reg.RegFind(temp) >= 0 ) {
      std::string strTitle = reg.GetReplaceString("\\1");
      std::string strArtist = reg.GetReplaceString("\\2");
      std::string strYear = reg.GetReplaceString("\\3");
      std::string strMins = reg.GetReplaceString("\\4");
      std::string strSecs = reg.GetReplaceString("\\5");
      tag.SetLoaded(true);
      tag.SetTrackNumber(iTrack);
      if (tag.GetDuration() == 0)
        tag.SetDuration(atoi(strMins.c_str())*60+atoi(strSecs.c_str()));
      if (tag.GetTitle() == "")
        tag.SetTitle(strTitle);
      if (tag.GetArtist().empty())
        tag.SetArtist(strArtist);
      SYSTEMTIME dateTime;
      dateTime.wYear = atoi(strYear.c_str());
      tag.SetReleaseDate(dateTime);
      f2.close();
      return( true );
    }
  }

  f2.close();
  tag.SetLoaded(false);
  return( false );
}

