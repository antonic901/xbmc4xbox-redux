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

#include "XBDateTime.h"
#include "utils/ScraperUrl.h"
#include "utils/Fanart.h"
#include "utils/ISortable.h"
#include "utils/StreamDetails.h"
#include "video/Bookmark.h"

#include <vector>

class CArchive;
class TiXmlNode;
class TiXmlElement;
class CVariant;

struct SActorInfo
{
  CStdString strName;
  CStdString strRole;
  CScraperUrl thumbUrl;
};

class CVideoInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
  CVideoInfoTag() { Reset(); };
  void Reset();
  bool Load(const TiXmlElement *movie, bool chained = false);
  bool Save(TiXmlNode *node, const CStdString &tag, bool savePathInfo = true);
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value);
  virtual void ToSortable(SortItem& sortable);
  const CStdString GetCast(bool bIncludeRole = false) const;
  bool HasStreamDetails() const;
  bool IsEmpty() const;

  const CStdString& GetPath() const
  {
    if (m_strFileNameAndPath.IsEmpty())
      return m_strPath;
    return m_strFileNameAndPath;
  };

  /*! \brief retrieve the duration in seconds.
   Prefers the duration from stream details if available.
   */
  unsigned int GetDuration() const;

  /*! \brief get the duration in seconds from a minute string
   \param runtime the runtime string from a scraper or similar
   \return the time in seconds, if decipherable.
   */
  static unsigned int GetDurationFromMinuteString(const std::string &runtime);

  CStdString m_basePath; // the base path of the video, for folder-based lookups
  int m_parentPathID;      // the parent path id where the base path of the video lies
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
  std::vector<std::string> m_artist;
  std::vector< SActorInfo > m_cast;
  typedef std::vector< SActorInfo >::const_iterator iCast;
  CStdString m_strSet;
  int m_iSetId;
  std::vector<std::string> m_tags;
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
  CStdString m_strShowPath;
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
  int m_iIdSeason;
  CFanart m_fanart;
  CStreamDetails m_streamDetails;
  CBookmark m_resumePoint;
  CDateTime m_dateAdded;
  CStdString m_type;
  int m_duration; ///< duration in seconds

private:
  void ParseNative(const TiXmlElement* movie);
};

typedef std::vector<CVideoInfoTag> VECMOVIES;
