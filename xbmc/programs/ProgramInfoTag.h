/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>
#include <vector>

#include "XBDateTime.h"
#include "utils/IArchivable.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"

class CArchive;
class TiXmlNode;
class TiXmlElement;
class CVariant;

class CProgramInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
  CProgramInfoTag() { Reset(); };
  void Reset();
  bool Load(const TiXmlElement *element, bool prioritise = false);
  bool Save(TiXmlNode *node, const std::string &tag, bool savePathInfo = true, const TiXmlElement *additionalNode = NULL);
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value) const;
  virtual void ToSortable(SortItem& sortable, Field field) const;
  const bool HasYear() const;
  const int GetYear() const;
  const bool HasReleaseDate() const;
  const CDateTime& GetReleaseDate() const;
  bool IsEmpty() const;

  const std::string& GetPath() const
  {
    return m_strFileNameAndPath;
  };

  void SetBasePath(std::string basePath);
  void SetDeveloper(std::vector<std::string> developer);
  void SetPublisher(std::vector<std::string> publisher);
  void SetGenre(std::vector<std::string> genre);
  void SetGeneralFeature(std::vector<std::string> generalFeature);
  void SetOnlineFeature(std::vector<std::string> onlineFeature);
  void SetPlatform(std::vector<std::string> platform);
  void SetTags(std::vector<std::string> tags);
  void SetTrailer(std::string trailer);
  void SetPlot(std::string plot);
  void SetTitle(std::string title);
  void SetFileNameAndPath(std::string fileNameAndPath);
  void SetESRB(std::string esrb);
  void SetSystem(std::string system);
  void SetUniqueID(std::string uniqueID);
  void SetLastPlayed(CDateTime lastPlayed);
  void SetPlayCount(int playCount);
  void SetReleaseDate(CDateTime year);
  void SetReleaseDateFromDBDate(std::string releaseDateString);
  void SetYear(int year);
  void SetDbId(int dbId);
  void SetRating(float rating);
  void SetExclusive(bool exclusive);
  void SetType(std::string type);

  std::string m_basePath;
  std::vector<std::string> m_developer;
  std::vector<std::string> m_publisher;
  std::vector<std::string> m_genre;
  std::vector<std::string> m_generalFeature;
  std::vector<std::string> m_onlineFeature;
  std::vector<std::string> m_platform;
  std::vector<std::string> m_tags;
  std::string m_strTrailer;
  std::string m_strPlot;
  std::string m_strTitle;
  std::string m_strFileNameAndPath;
  std::string m_strESRB;
  std::string m_strSystem;
  std::string m_strUniqueID;
  int m_playCount;
  int m_iDbId;
  bool m_bExclusive;
  CDateTime m_dateAdded;
  CDateTime m_lastPlayed;
  CDateTime m_releaseDate;
  float m_rating;
  std::string m_type;

private:
  /* \brief Parse our native XML format for program info.
   See Load for a description of the available tag types.

   \param element    the root XML element to parse.
   \param prioritise whether additive tags should be replaced (or prepended) by the content of the tags, or appended to.
   \sa Load
   */
  void ParseNative(const TiXmlElement* element, bool prioritise);

  std::string Trim(std::string &value);
  std::vector<std::string> Trim(std::vector<std::string> &items);
};
