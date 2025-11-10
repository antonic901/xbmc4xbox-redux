/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ProgramInfoTag.h"

#include "Util.h"
#include "settings/AdvancedSettings.h"
#include "utils/Archive.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"

void CProgramInfoTag::Reset()
{
  m_developer.clear();
  m_publisher.clear();
  m_genre.clear();
  m_generalFeature.clear();
  m_onlineFeature.clear();
  m_platform.clear();
  m_tags.clear();
  m_strTrailer.clear();
  m_strPlot.clear();
  m_strTitle.clear();
  m_strFileNameAndPath.clear();
  m_strESRB.clear();
  m_strSystem.clear();
  m_strUniqueID.clear();
  m_playCount = 0;
  m_iDbId = -1;
  m_bExclusive = false;
  m_dateAdded.Reset();
  m_lastPlayed.Reset();
  m_releaseDate.Reset();
  m_rating = -1.0f;
  m_type.clear();
}

bool CProgramInfoTag::Save(TiXmlNode *node, const std::string &tag, bool savePathInfo, const TiXmlElement *additionalNode)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement programElement(tag.c_str());
  TiXmlNode *program = node->InsertEndChild(programElement);

  if (!program) return false;

  if (savePathInfo)
  {
    XMLUtils::SetString(program, "filenameandpath", m_strFileNameAndPath);
    XMLUtils::SetString(program, "basepath", m_basePath);
  }

  XMLUtils::SetStringArray(program, "developer", m_developer);
  XMLUtils::SetStringArray(program, "publisher", m_publisher);
  XMLUtils::SetStringArray(program, "genre", m_genre);
  XMLUtils::SetStringArray(program, "generalfeature", m_generalFeature);
  XMLUtils::SetStringArray(program, "onlinefeature", m_onlineFeature);
  XMLUtils::SetStringArray(program, "platform", m_platform);
  XMLUtils::SetStringArray(program, "tag", m_tags);

  if (!m_type.empty())
    XMLUtils::SetString(program, "type", m_type);
  if (!m_strTrailer.empty())
    XMLUtils::SetString(program, "trailer", m_strTrailer);
  if (!m_strPlot.empty())
    XMLUtils::SetString(program, "plot", m_strPlot);
  XMLUtils::SetString(program, "title", m_strTitle);
  if (!m_strESRB.empty())
    XMLUtils::SetString(program, "esrb", m_strESRB);
  if (!m_strSystem.empty())
    XMLUtils::SetString(program, "system", m_strSystem);
  if (!m_strUniqueID.empty())
    XMLUtils::SetString(program, "uniqueid", m_strUniqueID);

  XMLUtils::SetInt(program, "playcount", m_playCount);
  XMLUtils::SetBoolean(program, "exclusive", m_bExclusive);

  XMLUtils::SetDateTime(program, "dateadded", m_dateAdded);
  XMLUtils::SetDate(program, "lastplayed", m_lastPlayed);
  XMLUtils::SetDate(program, "releasedate", m_releaseDate);

  XMLUtils::SetFloat(program, "rating", m_rating);

  return true;
}

bool CProgramInfoTag::Load(const TiXmlElement *element, bool prioritise)
{
  if (!element)
    return false;

  ParseNative(element, prioritise);
  return true;
}

void CProgramInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_developer;
    ar << m_publisher;
    ar << m_genre;
    ar << m_generalFeature;
    ar << m_onlineFeature;
    ar << m_platform;
    ar << m_tags;
    ar << m_strTrailer;
    ar << m_strPlot;
    ar << m_strTitle;
    ar << m_strFileNameAndPath;
    ar << m_strESRB;
    ar << m_strSystem;
    ar << m_strUniqueID;
    ar << m_playCount;
    ar << m_iDbId;
    ar << m_bExclusive;
    ar << m_dateAdded.GetAsDBDateTime();
    ar << m_lastPlayed;
    ar << m_releaseDate;
    ar << m_rating;
    ar << m_type;
  }
  else
  {
    ar >> m_developer;
    ar >> m_publisher;
    ar >> m_genre;
    ar >> m_generalFeature;
    ar >> m_onlineFeature;
    ar >> m_platform;
    ar >> m_tags;
    ar >> m_strTrailer;
    ar >> m_strPlot;
    ar >> m_strTitle;
    ar >> m_strFileNameAndPath;
    ar >> m_strESRB;
    ar >> m_strSystem;
    ar >> m_strUniqueID;
    ar >> m_playCount;
    ar >> m_iDbId;
    ar >> m_bExclusive;

    std::string dbDateTime;
    ar >> dbDateTime;
    m_dateAdded.SetFromDBDateTime(dbDateTime);

    ar >> m_lastPlayed;
    ar >> m_releaseDate;

    ar >> m_rating;

    ar >> m_type;
  }
}

void CProgramInfoTag::Serialize(CVariant& value) const
{
  value["developer"] = m_developer;
  value["publisher"] = m_publisher;
  value["genre"] = m_genre;
  value["generalfeature"] = m_generalFeature;
  value["onlinefeature"] = m_onlineFeature;
  value["platform"] = m_platform;
  value["tag"] = m_tags;
  value["trailer"] = m_strTrailer;
  value["plot"] = m_strPlot;
  value["title"] = m_strTitle;
  value["filenameandpath"] = m_strFileNameAndPath;
  value["esrb"] = m_strESRB;
  value["system"] = m_strSystem;
  value["uniqueid"] = m_strUniqueID;
  value["playcount"] = m_playCount;
  value["dbid"] = m_iDbId;
  value["exclusive"] = m_bExclusive;
  value["dateadded"] = m_dateAdded.IsValid() ? m_dateAdded.GetAsDBDateTime() : StringUtils::Empty;
  value["lastplayed"] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDate() : StringUtils::Empty;
  value["releasedate"] = m_releaseDate.IsValid() ? m_releaseDate.GetAsDBDate() : StringUtils::Empty;
  value["year"] = m_releaseDate.GetYear();
  value["rating"] = m_rating;
  value["type"] = m_type;
}

void CProgramInfoTag::ToSortable(SortItem& sortable, Field field) const
{
  switch (field)
  {
  case FieldTitle:
  {
    // make sure not to overwrite an existing title with an empty one
    std::string title = m_strTitle;
    if (!title.empty() || sortable.find(FieldTitle) == sortable.end())
      sortable[FieldTitle] = title;
    break;
  }
  case FieldPlaycount:                sortable[FieldPlaycount] = m_playCount; break;
  case FieldYear:                     sortable[FieldYear] = m_releaseDate.GetYear(); break;
  case FieldRating:                   sortable[FieldRating] = m_rating; break;
  case FieldDateAdded:                sortable[FieldDateAdded] = m_dateAdded.IsValid() ? m_dateAdded.GetAsDBDateTime() : StringUtils::Empty; break;
  default: break;
  }
}

const bool CProgramInfoTag::HasYear() const
{
  return m_releaseDate.IsValid();
}

const int CProgramInfoTag::GetYear() const
{
  if (HasYear())
    return m_releaseDate.GetYear();
  return 0;
}

const bool CProgramInfoTag::HasReleaseDate() const
{
  return m_releaseDate.IsValid();
}

const CDateTime& CProgramInfoTag::GetReleaseDate() const
{
  return m_releaseDate;
}

void CProgramInfoTag::ParseNative(const TiXmlElement* program, bool prioritise)
{
  std::string value;

  std::vector<std::string> developers(m_developer);
  if (XMLUtils::GetStringArray(program, "developer", developers, prioritise, g_advancedSettings.m_programItemSeparator))
    SetDeveloper(developers);

  std::vector<std::string> publishers(m_publisher);
  if (XMLUtils::GetStringArray(program, "publisher", publishers, prioritise, g_advancedSettings.m_programItemSeparator))
    SetPublisher(publishers);

  std::vector<std::string> genres(m_genre);
  if (XMLUtils::GetStringArray(program, "genre", genres, prioritise, g_advancedSettings.m_programItemSeparator))
    SetGenre(genres);

  std::vector<std::string> generalFeatures(m_generalFeature);
  if (XMLUtils::GetStringArray(program, "generalfeature", generalFeatures, prioritise, g_advancedSettings.m_programItemSeparator))
    SetGeneralFeature(generalFeatures);

  std::vector<std::string> onlineFeatures(m_onlineFeature);
  if (XMLUtils::GetStringArray(program, "onlinefeature", onlineFeatures, prioritise, g_advancedSettings.m_programItemSeparator))
    SetOnlineFeature(onlineFeatures);

  std::vector<std::string> platforms(m_platform);
  if (XMLUtils::GetStringArray(program, "platform", platforms, prioritise, g_advancedSettings.m_programItemSeparator))
    SetPlatform(platforms);

  std::vector<std::string> tags(m_tags);
  if (XMLUtils::GetStringArray(program, "tag", tags, prioritise, g_advancedSettings.m_programItemSeparator))
    SetTags(tags);

  if (XMLUtils::GetString(program, "type", value))
    m_type = value;

  if (XMLUtils::GetString(program, "trailer", value))
  {
    m_strTrailer = value;
    if (!StringUtils::StartsWithNoCase(value, "http"))
    {
      std::string strParentPath = URIUtils::GetParentPath(m_strFileNameAndPath);
      m_strTrailer = URIUtils::AddFileToFolder(strParentPath, value);
    }
  }

  if (XMLUtils::GetString(program, "overview", value))
    m_strPlot = value;

  if (XMLUtils::GetString(program, "title", value))
    m_strTitle = value;

  if (XMLUtils::GetString(program, "esrb", value))
    m_strESRB = value;

  if (XMLUtils::GetString(program, "system", value))
    m_strSystem = value;

  if (URIUtils::HasExtension(m_strFileNameAndPath, ".xbe"))
  {
    unsigned int xbeID = CUtil::GetXbeID(m_strFileNameAndPath);
    std::stringstream ss;
    ss << std::hex << std::uppercase << xbeID;
    m_strUniqueID = ss.str();
  }
  else if (false)
  {
    // TODO: How to get MD5 or similar hash for ROM files?
  }
  else if (XMLUtils::GetString(program, "uniqueid", value))
    m_strUniqueID = value;

  XMLUtils::GetInt(program, "playcount", m_playCount);

  XMLUtils::GetBoolean(program, "exclusive", m_bExclusive);

  XMLUtils::GetDateTime(program, "dateadded", m_dateAdded);
  XMLUtils::GetDate(program, "lastplayed", m_lastPlayed);
  if (!XMLUtils::GetDate(program, "releasedate", m_releaseDate))
  {
    int year;
    if (XMLUtils::GetInt(program, "year", year))
      SetYear(year);
  }

  XMLUtils::GetFloat(program, "rating", m_rating);
}

bool CProgramInfoTag::IsEmpty() const
{
  return m_strTitle.empty() && m_strFileNameAndPath.empty();
}

void CProgramInfoTag::SetBasePath(std::string basePath)
{
  m_basePath = basePath;
}

void CProgramInfoTag::SetDeveloper(std::vector<std::string> developer)
{
  m_developer = developer;
}

void CProgramInfoTag::SetPublisher(std::vector<std::string> publisher)
{
  m_publisher = publisher;
}

void CProgramInfoTag::SetGenre(std::vector<std::string> genre)
{
  m_genre = genre;
}

void CProgramInfoTag::SetGeneralFeature(std::vector<std::string> generalFeature)
{
  m_generalFeature = generalFeature;
}

void CProgramInfoTag::SetOnlineFeature(std::vector<std::string> onlineFeature)
{
  m_onlineFeature = onlineFeature;
}

void CProgramInfoTag::SetPlatform(std::vector<std::string> platform)
{
  m_platform = platform;
}

void CProgramInfoTag::SetTags(std::vector<std::string> tags)
{
  m_tags = tags;
}

void CProgramInfoTag::SetTrailer(std::string trailer)
{
  m_strTrailer = trailer;
}

void CProgramInfoTag::SetPlot(std::string plot)
{
  m_strPlot = plot;
}

void CProgramInfoTag::SetTitle(std::string title)
{
  m_strTitle = title;
}

void CProgramInfoTag::SetFileNameAndPath(std::string fileNameAndPath)
{
  m_strFileNameAndPath = fileNameAndPath;
}

void CProgramInfoTag::SetESRB(std::string esrb)
{
  m_strESRB = esrb;
}

void CProgramInfoTag::SetSystem(std::string system)
{
  m_strSystem = system;
}

void CProgramInfoTag::SetUniqueID(std::string uniqueID)
{
  m_strSystem = uniqueID;
}

void CProgramInfoTag::SetLastPlayed(CDateTime lastPlayed)
{
  m_lastPlayed = lastPlayed;
}

void CProgramInfoTag::SetPlayCount(int playCount)
{
  m_playCount = playCount;
}

void CProgramInfoTag::SetReleaseDate(CDateTime releaseDate)
{
  m_releaseDate = releaseDate;
}

void CProgramInfoTag::SetReleaseDateFromDBDate(std::string releaseDateString)
{
  CDateTime releaseDate;
  releaseDate.SetFromDBDate(releaseDateString);
  SetReleaseDate(releaseDate);
}

void CProgramInfoTag::SetYear(int year)
{
  if (HasReleaseDate())
    m_releaseDate.SetDate(year, m_releaseDate.GetMonth(), m_releaseDate.GetDay());
  else
    m_releaseDate = CDateTime(year, 1, 1, 0, 0, 0);
}

void CProgramInfoTag::SetDbId(int dbId)
{
  m_iDbId = dbId;
}

void CProgramInfoTag::SetRating(float rating)
{
  m_rating = rating;
}

void CProgramInfoTag::SetExclusive(bool exclusive)
{
  m_bExclusive = exclusive;
}

void CProgramInfoTag::SetType(std::string type)
{
  m_type = type;
}

std::string CProgramInfoTag::Trim(std::string &value)
{
  return StringUtils::Trim(value);
}

void TrimStr(std::string& str) {
  str = StringUtils::Trim(str);
}

std::vector<std::string> CProgramInfoTag::Trim(std::vector<std::string>& items)
{
  std::for_each(items.begin(), items.end(), TrimStr);
  return items;
}
