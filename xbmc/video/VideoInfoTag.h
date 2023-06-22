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

#include "utils/Archive.h"
#include "utils/ScraperUrl.h"
#include "utils/Fanart.h"
#include "utils/StreamDetails.h"
#include "video/Bookmark.h"
#include "XBDateTime.h"

#include <vector>

struct SActorInfo
{
  CStdString strName;
  CStdString strRole;
  CScraperUrl thumbUrl;
};

class CVideoInfoTag : public IArchivable, public ISerializable
{
public:
  CVideoInfoTag() { Reset(); };
  void Reset();
  bool Load(const TiXmlElement *movie, bool chained = false);
  bool Save(TiXmlNode *node, const CStdString &tag, bool savePathInfo = true);
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value);
  const CStdString GetCast(bool bIncludeRole = false) const;
  bool HasStreamDetails() const;
  bool IsEmpty() const;

  std::vector<std::string> m_director;
  std::vector<std::string> m_writingCredits;
  std::vector<std::string> m_genre;
  std::vector<std::string> m_country;
  CStdString m_strTagLine;
  CStdString m_strPlotOutline;
  CStdString m_strTrailer;
  CStdString m_strPlot;
  CScraperUrl m_strPictureURL;
  CStdString m_strTitle;
  CStdString m_strSortTitle;
  CStdString m_strVotes;
  CStdString m_strArtist;
  std::vector< SActorInfo > m_cast;
  typedef std::vector< SActorInfo >::const_iterator iCast;
  std::vector<std::string> m_set;
  std::vector<int> m_setId;
  CStdString m_strRuntime;
  CStdString m_strFile;
  CStdString m_strPath;
  CStdString m_strIMDBNumber;
  CStdString m_strMPAARating;
  CStdString m_strFileNameAndPath;
  CStdString m_strOriginalTitle;
  CStdString m_strEpisodeGuide;
  CDateTime m_premiered;
  CStdString m_strStatus;
  CStdString m_strProductionCode;
  CDateTime m_firstAired;
  CStdString m_strShowTitle;
  std::vector<std::string> m_studio;
  CStdString m_strAlbum;
  CDateTime m_lastPlayed;
  std::vector<std::string> m_showLink;
  int m_playCount;
  int m_iTop250;
  int m_iYear;
  int m_iSeason;
  int m_iEpisode;
  int m_iDbId; 
  int m_iFileId; 
  int m_iSpecialSortSeason;
  int m_iSpecialSortEpisode;
  int m_iTrack;
  float m_fRating;
  float m_fEpBookmark;
  int m_iBookmarkId;
  int m_iIdShow;
  CFanart m_fanart;
  CStreamDetails m_streamDetails;
  CBookmark m_resumePoint;
  CStdString m_type;

private:
  void ParseNative(const TiXmlElement* movie);
};

typedef std::vector<CVideoInfoTag> VECMOVIES;
