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

//------------------------------
// CApeTag in 2005 by JMarshall
//------------------------------
#include "cores/paplayer/ReplayGain.h"
#include "DllLibapetag.h"

namespace MUSIC_INFO
{

#pragma once

class CAPEv2Tag
{
public:
  CAPEv2Tag(void);
  virtual ~CAPEv2Tag(void);
  bool ReadTag(const char* filename);
  std::string GetTitle() { return m_strTitle; }
  std::string GetArtist() { return m_strArtist; }
  std::string GetYear() { return m_strYear; }
  std::string GetAlbum() { return m_strAlbum; }
  std::string GetAlbumArtist() { return m_strAlbumArtist; };
  std::string GetGenre() { return m_strGenre; }
  int GetTrackNum() { return m_nTrackNum; }
  int GetDiscNum() { return m_nDiscNum; }
  std::string GetMusicBrainzTrackID() { return m_strMusicBrainzTrackID; }
  std::string GetMusicBrainzArtistID() { return m_strMusicBrainzArtistID; }
  std::string GetMusicBrainzAlbumID() { return m_strMusicBrainzAlbumID; }
  std::string GetMusicBrainzAlbumArtistID() { return m_strMusicBrainzAlbumArtistID; }
  std::string GetMusicBrainzTRMID() { return m_strMusicBrainzTRMID; }
  std::string GetComment() { return m_strComment; };
  std::string GetLyrics() { return m_strLyrics; };
  bool GetCompilation() { return m_bCompilation; };
  char GetRating() { return m_rating; };
  void GetReplayGainFromTag(apetag *tag);
  const CReplayGain &GetReplayGain() { return m_replayGain; };

  static size_t fread_callback(void *ptr, size_t size, size_t nmemb, void *fp);
  static int fseek_callback(void *fp, long int offset, int whence);
  static long ftell_callback(void *fp);

protected:
  std::string m_strTitle;
  std::string m_strArtist;
  std::string m_strYear;
  std::string m_strAlbum;
  std::string m_strAlbumArtist;
  std::string m_strGenre;
  int m_nTrackNum;
  int m_nDiscNum;
  std::string m_strMusicBrainzTrackID;
  std::string m_strMusicBrainzArtistID;
  std::string m_strMusicBrainzAlbumID;
  std::string m_strMusicBrainzAlbumArtistID;
  std::string m_strMusicBrainzTRMID;
  std::string m_strComment;
  std::string m_strLyrics;
  CReplayGain m_replayGain;
  __int64 m_nDuration;
  char m_rating;
  bool m_bCompilation;

  DllLibApeTag m_dll;
};
}
