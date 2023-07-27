/*!
 \file Album.h
\brief
*/
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

#include <map>
#include <vector>

#include "music/Song.h"
#include "utils/ScraperUrl.h"

class CAlbum
{
public:
  CAlbum() { idAlbum = 0; iRating = 0; iYear = 0; };
  bool operator<(const CAlbum &a) const;

  void Reset()
  {
    idAlbum = -1;
    strAlbum.Empty();
    artist.clear();
    genre.clear();
    thumbURL.Clear();
    moods.clear();
    styles.clear();
    themes.clear();
    strReview.Empty();
    strLabel.Empty();
    strType.Empty();
    m_strDateOfRelease.Empty();
    iRating=-1;
    iYear=-1;
    bCompilation = false;
    songs.clear();
  }

  bool Load(const TiXmlElement *movie, bool chained=false);
  bool Save(TiXmlNode *node, const CStdString &tag, const CStdString& strPath);

  long idAlbum;
  CStdString strAlbum;
  std::vector<std::string> artist;
  std::vector<std::string> genre;
  CScraperUrl thumbURL;  
  std::vector<std::string> moods;
  std::vector<std::string> styles;
  std::vector<std::string> themes;
  CStdString strReview;
  CStdString strLabel;
  CStdString strType;
  CStdString m_strDateOfRelease;
  int iRating;
  int iYear;
  bool bCompilation;
  VECSONGS songs;
};

typedef std::vector<CAlbum> VECALBUMS;
