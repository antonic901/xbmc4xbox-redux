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

#include "programs/ProgramInfoTag.h"
#include "XMLUtils.h"
#include "LocalizeStrings.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/CharsetConverter.h"
#include "pictures/Picture.h"

#include <sstream>

using namespace std;

void CProgramInfoTag::Reset()
{
  m_developer.clear();
  m_publisher.clear();
  m_genre.clear();
  m_descriptor.clear();
  m_generalFeature.clear();
  m_onlineFeature.clear();
  m_platform.clear();
  m_strTrailer = "";
  m_strPlot = "";
  m_strTitle = "";
  m_strPath = "";
  m_strXBENumber = "";
  m_strFileNameAndPath = "";
  m_strOriginalTitle = "";
  m_strESRB = "";
  m_strSystem = "";
  m_iYear = 0;
  m_iDbId = -1;
  m_iFileId = -1;
  m_bExclusive = false;
  m_fanart.m_xml = "";
  m_dateAdded.Reset();
  m_type = "";
}

bool CProgramInfoTag::Save(TiXmlNode *node, const CStdString &tag, bool savePathInfo)
{
  if (!node) return false;

  // we start with <tag> tag
  TiXmlElement programElement(tag.c_str());
  TiXmlNode *program = node->InsertEndChild(programElement);

  if (!program) return false;

  XMLUtils::SetString(program, "system", m_strSystem);
  XMLUtils::SetString(program, "title", m_strTitle);
  if (!m_strOriginalTitle.IsEmpty())
    XMLUtils::SetString(program, "originaltitle", m_strOriginalTitle);
  XMLUtils::SetFloat(program, "rating", m_fRating);
  XMLUtils::SetInt(program, "year", m_iYear);
  if (!m_strPictureURL.m_xml.empty())
  {
    TiXmlDocument doc;
    doc.Parse(m_strPictureURL.m_xml);
    const TiXmlNode* thumb = doc.FirstChild("thumb");
    while(thumb)
    {
      program->InsertEndChild(*thumb);
      thumb = thumb->NextSibling("thumb");
    }
  }
  if (m_fanart.m_xml.size())
  {
    TiXmlDocument doc;
    doc.Parse(m_fanart.m_xml);
    program->InsertEndChild(*doc.RootElement());
  }
  if (savePathInfo)
  {
    XMLUtils::SetString(program, "file", m_strFile);
    XMLUtils::SetString(program, "path", m_strPath);
    XMLUtils::SetString(program, "filenameandpath", m_strFileNameAndPath);
    XMLUtils::SetString(program, "basepath", m_basePath);
  }

  XMLUtils::SetString(program, "id", m_strXBENumber);
  XMLUtils::SetStringArray(program, "genre", m_genre);
  XMLUtils::SetString(program, "trailer", m_strTrailer);

  // XBMC4Gamers default.xml
  XMLUtils::SetStringArray(program, "developer", m_developer);
  XMLUtils::SetStringArray(program, "publisher", m_publisher);
  XMLUtils::SetStringArray(program, "esrb_descriptor", m_descriptor);
  XMLUtils::SetStringArray(program, "feature_general", m_generalFeature);
  XMLUtils::SetStringArray(program, "feature_online", m_onlineFeature);
  XMLUtils::SetStringArray(program, "platform", m_platform);
  XMLUtils::SetBoolean(program, "exclusive", m_bExclusive);
  XMLUtils::SetString(program, "esrb", m_strESRB);
  XMLUtils::SetString(program, "overview", m_strPlot);

  XMLUtils::SetString(program, "dateadded", m_dateAdded.GetAsDBDateTime());

  return true;
}

bool CProgramInfoTag::Load(const TiXmlElement *program, bool chained /* = false */)
{
  if (!program) return false;

  // reset our details if we aren't chained.
  if (!chained) Reset();

  ParseNative(program);

  return true;
}

void CProgramInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_developer;
    ar << m_publisher;
    ar << m_genre;
    ar << m_descriptor;
    ar << m_generalFeature;
    ar << m_onlineFeature;
    ar << m_platform;
    ar << m_strPlot;
    ar << m_strPictureURL.m_spoof;
    ar << m_strPictureURL.m_xml;
    ar << m_fanart.m_xml;
    ar << m_strTitle;
    ar << m_strTrailer;    
    ar << m_strFile;
    ar << m_strPath;
    ar << m_strXBENumber;
    ar << m_strESRB;
    ar << m_strSystem;
    ar << m_strFileNameAndPath;
    ar << m_strOriginalTitle;
    ar << m_iYear;
    ar << m_fRating;
    ar << m_bExclusive;
    ar << m_iDbId;
    ar << m_iFileId;
    ar << m_basePath;
    ar << m_parentPathID;
    ar << m_dateAdded.GetAsDBDateTime();
    ar << m_type;
  }
  else
  {
    ar >> m_developer;
    ar >> m_publisher;
    ar >> m_genre;
    ar >> m_descriptor;
    ar >> m_generalFeature;
    ar >> m_onlineFeature;
    ar >> m_platform;
    ar >> m_strPlot;
    ar >> m_strPictureURL.m_spoof;
    ar >> m_strPictureURL.m_xml;
    m_strPictureURL.Parse();
    ar >> m_fanart.m_xml;
    m_fanart.Unpack();
    ar >> m_strTitle;
    ar >> m_strTrailer;    
    ar >> m_strFile;
    ar >> m_strPath;
    ar >> m_strXBENumber;
    ar >> m_strESRB;
    ar >> m_strSystem;
    ar >> m_strFileNameAndPath;
    ar >> m_strOriginalTitle;
    ar >> m_iYear;
    ar >> m_bExclusive;
    ar >> m_fRating;
    ar >> m_iDbId;
    ar >> m_iFileId;
    ar >> m_basePath;
    ar >> m_parentPathID;

    CStdString dateAdded;
    ar >> dateAdded;
    m_dateAdded.SetFromDBDateTime(dateAdded);
    ar >> m_type;
  }
}

void CProgramInfoTag::Serialize(CVariant& value)
{
  value["developer"] = m_developer;
  value["publisher"] = m_publisher;
  value["genre"] = m_genre;
  value["descriptor"] = m_descriptor;
  value["generalfeature"] = m_generalFeature;
  value["onlinefeature"] = m_onlineFeature;
  value["platform"] = m_platform;
  value["plot"] = m_strPlot;
  value["title"] = m_strTitle;
  value["trailer"] = m_strTrailer;
  value["file"] = m_strFile;
  value["path"] = m_strPath;
  value["imdbnumber"] = m_strXBENumber;
  value["filenameandpath"] = m_strFileNameAndPath;
  value["esrb"] = m_strESRB;
  value["system"] = m_strSystem;
  value["originaltitle"] = m_strOriginalTitle;
  value["year"] = m_iYear;
  value["rating"] = m_fRating;
  value["exclusive"] = m_bExclusive;
  value["dbid"] = m_iDbId;
  value["fileid"] = m_iFileId;
  value["dateadded"] = m_dateAdded.IsValid() ? m_dateAdded.GetAsDBDateTime() : "";
  value["type"] = m_type;
}

void CProgramInfoTag::ToSortable(SortItem& sortable)
{
  // TODO: implement this
}

void CProgramInfoTag::ParseNative(const TiXmlElement* program)
{
  XMLUtils::GetString(program, "type", m_type);
  XMLUtils::GetString(program, "system", m_strSystem);
  XMLUtils::GetString(program, "title", m_strTitle);
  XMLUtils::GetString(program, "originaltitle", m_strOriginalTitle);
  XMLUtils::GetFloat(program, "rating", m_fRating);
  int max_value = 10;
  const TiXmlElement* rElement = program->FirstChildElement("rating");
  if (rElement && (rElement->QueryIntAttribute("max", &max_value) == TIXML_SUCCESS) && max_value>=1)
  {    
    m_fRating = m_fRating / max_value * 10; // Normalise the Program Rating to between 1 and 10
  }
  XMLUtils::GetInt(program, "year", m_iYear);
  XMLUtils::GetString(program, "plot", m_strPlot);
  XMLUtils::GetString(program, "file", m_strFile);
  XMLUtils::GetString(program, "path", m_strPath);
  XMLUtils::GetString(program, "id", m_strXBENumber);
  XMLUtils::GetString(program, "filenameandpath", m_strFileNameAndPath);
  XMLUtils::GetString(program, "trailer", m_strTrailer);
  XMLUtils::GetString(program, "basepath", m_basePath);

  const TiXmlElement* thumb = program->FirstChildElement("thumb");
  while (thumb)
  {
    m_strPictureURL.ParseElement(thumb);
    thumb = thumb->NextSiblingElement("thumb");
  }

  XMLUtils::GetStringArray(program, "genre", m_genre);

  // fanart
  const TiXmlElement *fanart = program->FirstChildElement("fanart");
  if (fanart)
  {
    m_fanart.m_xml << *fanart;
    m_fanart.Unpack();
  }

  // XBMC4Gamers default.xml
  XMLUtils::GetStringArray(program, "developer", m_developer);
  XMLUtils::GetStringArray(program, "publisher", m_publisher);
  XMLUtils::GetStringArray(program, "esrb_descriptor", m_descriptor);
  XMLUtils::GetStringArray(program, "feature_general", m_generalFeature);
  XMLUtils::GetStringArray(program, "feature_online", m_onlineFeature);
  XMLUtils::GetStringArray(program, "platform", m_platform);
  XMLUtils::GetBoolean(program, "exclusive", m_bExclusive);
  XMLUtils::GetString(program, "esrb", m_strESRB);
  XMLUtils::GetString(program, "overview", m_strPlot);

  // dateAdded
  CStdString dateAdded;
  XMLUtils::GetString(program, "dateadded", dateAdded);
  m_dateAdded.SetFromDBDateTime(dateAdded);
}

bool CProgramInfoTag::IsEmpty() const
{
  return (m_strTitle.IsEmpty() &&
          m_strFile.IsEmpty() &&
          m_strPath.IsEmpty());
}
