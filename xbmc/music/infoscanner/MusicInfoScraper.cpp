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

#include "XMLUtils.h"
#include "music/infoscanner/MusicInfoScraper.h"
#include "settings/AdvancedSettings.h"
#include "HTMLUtil.h"
#include "HTMLTable.h"
#include "Util.h"
#include "ScraperParser.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace MUSIC_GRABBER;
using namespace HTML;
CMusicInfoScraper::CMusicInfoScraper(const SScraperInfo& info)
{
  m_bSucceeded=false;
  m_bCanceled=false;
  m_iAlbum=-1;
  m_iArtist=-1;
  m_info = info;
}

CMusicInfoScraper::~CMusicInfoScraper(void)
{
  StopThread();
}

int CMusicInfoScraper::GetAlbumCount() const
{
  return (int)m_vecAlbums.size();
}

int CMusicInfoScraper::GetArtistCount() const
{
  return (int)m_vecArtists.size();
}

CMusicAlbumInfo& CMusicInfoScraper::GetAlbum(int iAlbum)
{
  return m_vecAlbums[iAlbum];
}

CMusicArtistInfo& CMusicInfoScraper::GetArtist(int iArtist)
{
  return m_vecArtists[iArtist];
}

void CMusicInfoScraper::FindAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist /* = "" */)
{
  m_strAlbum=strAlbum;
  m_strArtist=strArtist;
  m_bSucceeded=false;
  StopThread();
  Create();
}

void CMusicInfoScraper::FindArtistInfo(const CStdString& strArtist)
{
  m_strArtist=strArtist;
  m_bSucceeded=false;
  StopThread();
  Create();
}


void CMusicInfoScraper::FindAlbumInfo()
{
  CStdString strAlbum=m_strAlbum;
  CStdString strHTML;
  m_vecAlbums.erase(m_vecAlbums.begin(), m_vecAlbums.end());

  CScraperParser parser;
  parser.ClearCache();

  if (!parser.Load("special://xbmc/system/scrapers/music/" + m_info.strPath) || !parser.HasFunction("CreateAlbumSearchUrl"))
    return;

  if (!m_info.settings.GetPluginRoot() && m_info.settings.GetSettings().IsEmpty() && parser.HasFunction("GetSettings"))
  {
    m_info.settings.LoadSettingsXML("special://xbmc/system/scrapers/music/" + m_info.strPath);
    m_info.settings.SaveFromDefault();
  }

  parser.m_param[0] = strAlbum;
  parser.m_param[1] = m_strArtist;
  CURL::Encode(parser.m_param[0]);
  CURL::Encode(parser.m_param[1]);

  CLog::Log(LOGDEBUG, "%s: Searching for '%s - %s' using %s scraper (file: '%s', content: '%s', language: '%s', date: '%s', framework: '%s')",
    __FUNCTION__, m_strArtist.c_str(), strAlbum.c_str(), m_info.strTitle.c_str(), m_info.strPath.c_str(), m_info.strContent.c_str(), m_info.strLanguage.c_str(), m_info.strDate.c_str(), m_info.strFramework.c_str());


  CScraperUrl scrURL;
  scrURL.ParseString(parser.Parse("CreateAlbumSearchUrl",&m_info.settings));
  if (!scrURL.m_url.size() || !CScraperUrl::Get(scrURL.m_url[0], strHTML, m_http, parser.GetFilename()) || strHTML.size() == 0)
  {
    CLog::Log(LOGERROR, "%s: Unable to retrieve web site",__FUNCTION__);
    return;
  }

  parser.m_param[0] = strHTML;
  CStdString strXML = parser.Parse("GetAlbumSearchResults",&m_info.settings);
  CLog::Log(LOGDEBUG,"scraper: GetAlbumSearchResults returns %s",strXML.c_str());
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse web site",__FUNCTION__);
    return;
  }

  if (!XMLUtils::HasUTF8Declaration(strXML))
    g_charsetConverter.unknownToUTF8(strXML);

  // ok, now parse the xml file
  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
    return;
  }
  TiXmlHandle docHandle( &doc );
  TiXmlElement* album = docHandle.FirstChild( "results" ).FirstChild( "entity" ).Element();
  if (!album)
    return;

  while (album)
  {
    TiXmlNode* title = album->FirstChild("title");
    TiXmlElement* link = album->FirstChildElement("url");
    TiXmlNode* artist = album->FirstChild("artist");
    TiXmlNode* year = album->FirstChild("year");
    TiXmlElement* relevance = album->FirstChildElement("relevance");
    if (title && title->FirstChild())
    {
      CStdString strTitle = title->FirstChild()->Value();
      CStdString strArtist;
      CStdString strAlbumName;

      if (artist && artist->FirstChild())
      {
        strArtist = artist->FirstChild()->Value();
        strAlbumName.Format("%s - %s",strArtist.c_str(),strTitle.c_str());
      }
      else
        strAlbumName = strTitle;

      if (year && year->FirstChild())
        strAlbumName.Format("%s (%s)",strAlbumName.c_str(),year->FirstChild()->Value());

      CScraperUrl url;
      if (!link)
        url.ParseString(scrURL.m_xml);

      while (link && link->FirstChild())
      {
        url.ParseElement(link);
        link = link->NextSiblingElement("url");
      }
      CMusicAlbumInfo newAlbum(strTitle, strArtist, strAlbumName, url);
      if (relevance && relevance->FirstChild())
      {
        float scale=1;
        const char* newscale = relevance->Attribute("scale");
        if (newscale)
          scale = (float)atof(newscale);
        newAlbum.SetRelevance((float)atof(relevance->FirstChild()->Value())/scale);
      }
      m_vecAlbums.push_back(newAlbum);
    }
    album = album->NextSiblingElement();
  }
  
  if (m_vecAlbums.size()>0)
    m_bSucceeded=true;

  return;
}

void CMusicInfoScraper::FindArtistInfo()
{
  CStdString strArtist=m_strArtist;
  CStdString strHTML;
  m_vecArtists.erase(m_vecArtists.begin(), m_vecArtists.end());

  CScraperParser parser;
  parser.ClearCache();

  if (!parser.Load("special://xbmc/system/scrapers/music/" + m_info.strPath) || !parser.HasFunction("CreateArtistSearchUrl"))
    return;

  if (!m_info.settings.GetPluginRoot() && m_info.settings.GetSettings().IsEmpty() && parser.HasFunction("GetSettings"))
  {
    m_info.settings.LoadSettingsXML("special://xbmc/system/scrapers/music/" + m_info.strPath);
    m_info.settings.SaveFromDefault();
  }
  
  parser.m_param[0] = m_strArtist;
  CURL::Encode(parser.m_param[0]);

  CLog::Log(LOGDEBUG, "%s: Searching for '%s' using %s scraper (file: '%s', content: '%s', language: '%s', date: '%s', framework: '%s')",
    __FUNCTION__, m_strArtist.c_str(), m_info.strTitle.c_str(), m_info.strPath.c_str(), m_info.strContent.c_str(), m_info.strLanguage.c_str(), m_info.strDate.c_str(), m_info.strFramework.c_str());

  CScraperUrl scrURL;
  scrURL.ParseString(parser.Parse("CreateArtistSearchUrl",&m_info.settings));
  if (!CScraperUrl::Get(scrURL.m_url[0], strHTML, m_http, parser.GetFilename()) || strHTML.size() == 0)
  {
    CLog::Log(LOGERROR, "%s: Unable to retrieve web site",__FUNCTION__);
    return;
  }

  parser.m_param[0] = strHTML;
  CStdString strXML = parser.Parse("GetArtistSearchResults",&m_info.settings);
  CLog::Log(LOGDEBUG,"scraper: GetArtistSearchResults returns %s",strXML.c_str());
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse web site",__FUNCTION__);
    return;
  }

  if (!XMLUtils::HasUTF8Declaration(strXML))
    g_charsetConverter.unknownToUTF8(strXML);

  // ok, now parse the xml file
  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
    return;
  }

  TiXmlHandle docHandle( &doc );
  TiXmlElement* artist = docHandle.FirstChild( "results" ).FirstChild( "entity" ).Element();
  if (!artist)
    return;

  while (artist)
  {
    TiXmlNode* title = artist->FirstChild("title");
    TiXmlNode* year = artist->FirstChild("year");
    TiXmlNode* genre = artist->FirstChild("genre");
    TiXmlElement* link = artist->FirstChildElement("url");
    if (title && title->FirstChild())
    {
      CStdString strTitle = title->FirstChild()->Value();
      CScraperUrl url;
      if (!link)
        url.ParseString(scrURL.m_xml);
      while (link && link->FirstChild())
      {
        url.ParseElement(link);
        link = link->NextSiblingElement("url");
      }
      CMusicArtistInfo newArtist(strTitle, url);
      if (genre && genre->FirstChild())
        newArtist.GetArtist().genre = StringUtils::Split(genre->FirstChild()->Value(), g_advancedSettings.m_musicItemSeparator);
      if (year && year->FirstChild())
        newArtist.GetArtist().strBorn = year->FirstChild()->Value();
      m_vecArtists.push_back(newArtist);
    }
    artist = artist->NextSiblingElement();
  }
  
  if (m_vecArtists.size()>0)
    m_bSucceeded=true;

  return;
}

void CMusicInfoScraper::LoadAlbumInfo(int iAlbum)
{
  m_iAlbum=iAlbum;
  m_iArtist=-1;
  StopThread();
  Create();
}

void CMusicInfoScraper::LoadArtistInfo(int iArtist)
{
  m_iAlbum=-1;
  m_iArtist=iArtist;
  StopThread();
  Create();
}

void CMusicInfoScraper::LoadAlbumInfo()
{
  if (m_iAlbum<0 || m_iAlbum>=(int)m_vecAlbums.size())
    return;

  CMusicAlbumInfo& album=m_vecAlbums[m_iAlbum];
  album.GetAlbum().artist.clear();
  if (album.Load(m_http,m_info))
    m_bSucceeded=true;
}

void CMusicInfoScraper::LoadArtistInfo()
{
  if (m_iArtist<0 || m_iArtist>=(int)m_vecArtists.size())
    return;

  CMusicArtistInfo& artist=m_vecArtists[m_iArtist];
  artist.GetArtist().strArtist.Empty();
  if (artist.Load(m_http,m_info))
    m_bSucceeded=true;
}

bool CMusicInfoScraper::Completed()
{
  return WaitForThreadExit(10);
}

bool CMusicInfoScraper::Succeeded()
{
  return !m_bCanceled && m_bSucceeded;
}

void CMusicInfoScraper::Cancel()
{
  m_http.Cancel();
  m_bCanceled=true;
  m_http.Reset();
}

bool CMusicInfoScraper::IsCanceled()
{
  return m_bCanceled;
}

void CMusicInfoScraper::OnStartup()
{
  m_bSucceeded=false;
  m_bCanceled=false;
}

void CMusicInfoScraper::Process()
{
  try
  {
    if (m_strAlbum.size())
    {
      FindAlbumInfo();
      m_strAlbum.Empty();
      m_strArtist.Empty();
    }
    else if (m_strArtist.size())
    {
      FindArtistInfo();
      m_strArtist.Empty();
    }
    if (m_iAlbum>-1)
    {
      LoadAlbumInfo();
      m_iAlbum=-1;
    }
    if (m_iArtist>-1)
    {
      LoadArtistInfo();
      m_iArtist=-1;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicInfoScraper::Process()");
  }
}

bool CMusicInfoScraper::CheckValidOrFallback(const CStdString &fallbackScraper)
{
  CScraperParser parser;
  if (parser.Load("special://xbmc/system/scrapers/music/" + m_info.strPath))
    return true;
  if (m_info.strPath != fallbackScraper &&
      parser.Load("special://xbmc/system/scrapers/music/" + fallbackScraper))
  {
    CLog::Log(LOGWARNING, "%s - scraper %s fails to load, falling back to %s", __FUNCTION__, m_info.strPath.c_str(), fallbackScraper.c_str());
    m_info.strPath = fallbackScraper;
    m_info.strContent = "albums";
    m_info.strTitle = parser.GetName();
    m_info.strDate = parser.GetDate();
    m_info.strFramework = parser.GetFramework();
    m_info.strLanguage = parser.GetLanguage();
    m_info.settings.LoadSettingsXML("special://xbmc/system/scrapers/music/" + m_info.strPath);
    return true;
  }
  return false;
}
