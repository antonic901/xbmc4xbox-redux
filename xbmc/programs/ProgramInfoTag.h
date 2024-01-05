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
#include "DateTime.h"
#include "utils/ScraperUrl.h"
#include "utils/Fanart.h"
#include "utils/ISortable.h"
#include "utils/StreamDetails.h"

#include <vector>

class CProgramInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
  CProgramInfoTag() { Reset(); };
  void Reset();
  bool Load(const TiXmlElement *program, bool chained = false);
  bool Save(TiXmlNode *node, const CStdString &tag, bool savePathInfo = true);
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value);
  virtual void ToSortable(SortItem& sortable);
  bool IsEmpty() const;

  CStdString m_basePath; // the base path of the program, for folder-based lookups
  int m_parentPathID; // the parent path id where the base path of the program lies
  std::vector<std::string> m_developer;
  std::vector<std::string> m_publisher;
  std::vector<std::string> m_genre;
  std::vector<std::string> m_descriptor;
  std::vector<std::string> m_generalFeature;
  std::vector<std::string> m_onlineFeature;
  std::vector<std::string> m_platform;
  std::vector<std::string> m_tags;
  CStdString m_strTrailer;
  CStdString m_strPlot;
  CScraperUrl m_strPictureURL;
  CStdString m_strTitle;
  CStdString m_strFile;
  CStdString m_strPath;
  CStdString m_strXBENumber; // ID of XBE
  CStdString m_strFileNameAndPath;
  CStdString m_strOriginalTitle;
  CStdString m_strESRB;
  CStdString m_strSystem;
  CDateTime m_lastPlayed;
  int m_playCount;
  int m_iYear;
  int m_iDbId; 
  int m_iFileId;
  float m_fRating;
  bool m_bExclusive;
  CFanart m_fanart;
  CDateTime m_dateAdded;
  CStdString m_type;

private:
  void ParseNative(const TiXmlElement* program);
};
