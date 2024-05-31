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

#include "SndtrkDirectory.h"
#include "xbox/IoSupport.h"
#include "FileItem.h"
#include "URL.h"

SOUNDTRACK datastorage; //created a vector of the XSOUNDTRACK_DATA class to keep track of each album

using namespace XFILE;

CSndtrkDirectory::CSndtrkDirectory(void)
{}

CSndtrkDirectory::~CSndtrkDirectory(void)
{}


bool CSndtrkDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string strRoot = url.Get();
  if (IsAlone(strRoot))
  {
    // Add each user provided soundtrack to the soundtrack vector
    // Took code from samples in XDK
    XSOUNDTRACK_DATA stData;
    HANDLE hSoundtrack = XFindFirstSoundtrack( &stData );
    if ( INVALID_HANDLE_VALUE != hSoundtrack )
    {
      do
      {
        // Ignore empty soundtracks && parent directories
        if ( stData.uSongCount > 0 && strRoot != "..")
        {
          CSoundtrack stInfo;
          stInfo.uSoundtrackId = stData.uSoundtrackId;
          stInfo.uSongCount = stData.uSongCount;
          wcscpy(stInfo.strName, stData.szName );

          ISOUNDTRACK it=datastorage.find(stData.uSoundtrackId);
          if (it==datastorage.end())
            datastorage.insert(SOUNDTRACK_PAIR(stInfo.uSoundtrackId, stInfo));
          else
            it->second=stInfo;

          // convert from WCHAR to std::string
          std::string strName;
          size_t bufferSize = 32 * sizeof(char);
          char narrowStr[32];
          size_t numConverted = wcstombs(narrowStr, stData.szName, bufferSize);
          if (numConverted == static_cast<size_t>(-1)) 
            strName = "";
          else
            strName = narrowStr;

          CFileItemPtr pItem(new CFileItem(strName));
          pItem->SetLabelPreformated(true);
          char tmpvar[4];
          sprintf(tmpvar,"%i",stData.uSoundtrackId);
          pItem->SetPath(strRoot + tmpvar);
          pItem->m_bIsFolder = true;
          items.Add(pItem);
        }
      }
      while ( XFindNextSoundtrack( hSoundtrack, &stData ) );

      XFindClose(hSoundtrack);
    }
  }
  else
  {
    char *ptr = strstr(strRoot.c_str(), "//");
    ptr += 2;
    int m_iconvert = atoi(ptr); //convert from char back to int to compare to data

    ISOUNDTRACK it=datastorage.find(m_iconvert);
    if (it == datastorage.end())
      return false;

    CSoundtrack stInfo = it->second;
    for ( UINT i = 0; i < stInfo.uSongCount; ++i )
    {
      DWORD dwSongId;
      DWORD dwSongLength;
      WCHAR wcSong[64];
      if ( XGetSoundtrackSongInfo( stInfo.uSoundtrackId, i, &dwSongId,
                                   &dwSongLength, wcSong, MAX_SONG_NAME ) )
      {
        // convert from WCHAR to std::string
        std::string strSong;
        size_t bufferSize = 64 * sizeof(char);
        char narrowStr[64];
        size_t numConverted = wcstombs(narrowStr, wcSong, bufferSize);
        if (numConverted == static_cast<size_t>(-1)) 
          strSong = "";
        else
          strSong = narrowStr;

        // Add it to the list
        CFileItemPtr pItem(new CFileItem(strSong));
        std::string strPath("E:\\TDATA\\fffe0000\\music\\");
        char tmpvar[16];
        *tmpvar = NULL;
        sprintf(tmpvar, "%04x", stInfo.uSoundtrackId);
        strPath += tmpvar;
        strPath += "\\";
        *tmpvar = NULL;
        sprintf(tmpvar, "%08x", dwSongId);
        strPath += tmpvar;
        strPath += ".wma";
        pItem->SetPath(strPath);
        pItem->m_bIsFolder = false;
        items.Add(pItem);
      }
    }
  }
  return true;
}

bool CSndtrkDirectory::IsAlone(const std::string& strPath)
{
  return (strcmp("soundtrack://", strPath.c_str()) == 0);
}

bool CSndtrkDirectory::FindTrackName(const std::string& strPath, char *NameOfSong)
{
  char* ptr = strstr(strPath.c_str(), "E:\\TDATA\\fffe0000\\music\\");
  if (ptr == NULL) return false;
  ptr += strlen("E:\\TDATA\\fffe0000\\music\\");
  char album[5];
  int x = 0;
  for (x = 0;x < 4;x++)
  {
    album[x] = *ptr;
    ptr += 1;
  }
  album[4] = '\0';
  ptr += 1;
  char trackno[9];
  for (x = 0;x < 8;x++)
  {
    trackno[x] = *ptr;
    ptr += 1;
  }
  trackno[8] = '\0';
  int AlbumID, SongID;
  sscanf(album, "%x", &AlbumID);
  sscanf(trackno, "%x", &SongID);
  x = 0;
  bool test = true;
  while (test == true)
  {
    DWORD dwSongId;
    DWORD dwSongLength;
    WCHAR Songname[64];
    if ( XGetSoundtrackSongInfo( AlbumID, x, &dwSongId, &dwSongLength, Songname, MAX_SONG_NAME ) == false)
      test = false;
    if (dwSongId == SongID)
    {
      wcstombs(NameOfSong, Songname, 64);
      return true;
    }
    x++;
  }
  return false;
}
