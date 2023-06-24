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

#include "music/Artist.h"
#include "XMLUtils.h"
#include "settings/AdvancedSettings.h"

using namespace std;

bool CArtist::Load(const TiXmlElement *artist, bool chained)
{
  if (!artist) return false;
  if (!chained)
    Reset();

  XMLUtils::GetString(artist,"name",strArtist);
  XMLUtils::GetStringArray(artist, "genre", genre);
  XMLUtils::GetStringArray(artist, "style", styles);
  XMLUtils::GetStringArray(artist, "mood", moods);
  XMLUtils::GetStringArray(artist, "yearsactive", yearsActive);
  XMLUtils::GetStringArray(artist, "instruments", instruments);

  XMLUtils::GetString(artist, "born", strBorn);
  XMLUtils::GetString(artist, "formed", strFormed);
  XMLUtils::GetString(artist, "biography", strBiography);
  XMLUtils::GetString(artist, "died", strDied);
  XMLUtils::GetString(artist, "disbanded", strDisbanded);

  const TiXmlElement* thumb = artist->FirstChildElement("thumb");
  while (thumb)
  {
    thumbURL.ParseElement(thumb);
    thumb = thumb->NextSiblingElement("thumb");
  }
  const TiXmlElement* node = artist->FirstChildElement("album");
  while (node)
  {
    const TiXmlNode* title = node->FirstChild("title");
    if (title && title->FirstChild())
    {
      CStdString strTitle = title->FirstChild()->Value();
      CStdString strYear;
      const TiXmlNode* year = node->FirstChild("year");
      if (year && year->FirstChild())
        strYear = year->FirstChild()->Value();
      discography.push_back(make_pair(strTitle,strYear));
    }
    node = node->NextSiblingElement("album");
  }
  
  // fanart
  const TiXmlElement *fanart2 = artist->FirstChildElement("fanart");
  if (fanart2)
  {
    fanart.m_xml << *fanart2;
    fanart.Unpack();
  }

  return true;
}

bool CArtist::Save(TiXmlNode *node, const CStdString &tag, const CStdString& strPath)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement artistElement(tag.c_str());
  TiXmlNode *artist = node->InsertEndChild(artistElement);

  if (!artist) return false;
  
  XMLUtils::SetString(artist,       "name", strArtist);
  XMLUtils::SetStringArray(artist, "genre", genre);
  XMLUtils::SetStringArray(artist, "style", styles);
  XMLUtils::SetStringArray(artist,  "mood", moods);
  XMLUtils::SetStringArray(artist, "yearsactive", yearsActive);
  XMLUtils::SetStringArray(artist, "instruments", instruments);
  XMLUtils::SetString(artist,        "born", strBorn);
  XMLUtils::SetString(artist,      "formed", strFormed);
  XMLUtils::SetString(artist,   "biography", strBiography);
  XMLUtils::SetString(artist,        "died", strDied);
  XMLUtils::SetString(artist,   "disbanded", strDisbanded);
  if (!thumbURL.m_xml.empty())
  {
    TiXmlDocument doc;
    doc.Parse(thumbURL.m_xml); 
    const TiXmlNode* thumb = doc.FirstChild("thumb");
    while (thumb)
    {
      artist->InsertEndChild(*thumb);
      thumb = thumb->NextSibling("thumb");
    }
  }
  XMLUtils::SetString(artist,        "path", strPath);
  if (fanart.m_xml.size())
  {
    TiXmlDocument doc;
    doc.Parse(fanart.m_xml);
    artist->InsertEndChild(*doc.RootElement());
  }

  // albums
  for (vector< pair<CStdString,CStdString> >::const_iterator it = discography.begin(); it != discography.end(); ++it)
  {
    // add a <album> tag
    TiXmlElement cast("album");
    TiXmlNode *node = artist->InsertEndChild(cast);
    TiXmlElement title("title");
    TiXmlNode *titleNode = node->InsertEndChild(title);
    TiXmlText name(it->first);
    titleNode->InsertEndChild(name);
    TiXmlElement year("year");
    TiXmlNode *yearNode = node->InsertEndChild(year);
    TiXmlText name2(it->second);
    yearNode->InsertEndChild(name2);
  }

  return true;
}
