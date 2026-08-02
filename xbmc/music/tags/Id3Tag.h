#pragma once
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
#include "music/tags/Tag.h"
#include "music/tags/DllLibid3tag.h"

namespace MUSIC_INFO
{

class CID3Tag : public CTag
{
public:
  CID3Tag(void);
  virtual ~CID3Tag(void);
  virtual bool Read(const std::string& strFile);
  virtual bool Write(const std::string& strFile);

  std::string ParseMP3Genre(const std::string& str) const;

protected:
  bool Parse();
  void ParseReplayGainInfo();

  std::string GetArtist() const;
  std::string GetAlbum() const;
  std::string GetAlbumArtist() const;
  std::string GetTitle() const;
  int GetTrack() const;
  int GetPartOfSet() const;
  std::string GetYear() const;
  std::string GetGenre() const;
  std::string GetComment() const;
  char       GetRating() const;
  bool       GetCompilation() const;
  std::string GetEncodedBy() const;
  std::string GetLyrics() const;

  bool HasPicture(id3_picture_type pictype) const;
  std::string GetPictureMimeType(id3_picture_type pictype) const;
  const BYTE* GetPictureData(id3_picture_type pictype, id3_length_t* length) const;
  const BYTE* GetUniqueFileIdentifier(const std::string& strOwnerIdentifier, id3_length_t* length) const;
  std::string GetUserText(const std::string& strDescription) const;
  bool GetFirstNonStandardPictype(id3_picture_type* pictype) const;

  void SetArtist(const std::string& strValue);
  void SetAlbum(const std::string& strValue);
  void SetAlbumArtist(const std::string& strValue);
  void SetTitle(const std::string& strValue);
  void SetTrack(int n);
  void SetPartOfSet(int n);
  void SetYear(const std::string& strValue);
  void SetGenre(const std::string& strValue);
  void SetEncodedBy(const std::string& strValue);
  void SetComment(const std::string& strValue);
  void SetRating(char rating);
  void SetCompilation(bool compilation);

  std::string ToStringCharset(const id3_ucs4_t* ucs4, id3_field_textencoding encoding) const;
  id3_ucs4_t* StringCharsetToUcs4(const std::string& str) const;

  mutable DllLibID3Tag m_dll;

  id3_tag* m_tag;
};
}
