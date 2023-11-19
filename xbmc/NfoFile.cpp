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
// NfoFile.cpp: implementation of the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#include "NfoFile.h"
#include "music/MusicDatabase.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoDownloader.h"
#include "addons/AddonManager.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "settings/GUISettings.h"
#include "LangInfo.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "settings/Settings.h"
#include <vector>
#include "utils/log.h"

using namespace std;
using namespace XFILE;
using namespace ADDON;

CNfoFile::NFOResult CNfoFile::Create(const CStdString& strPath, const ScraperPtr& info, int episode, const CStdString& strPath2)
{
  m_info = info; // assume we can use these settings
  m_type = ScraperTypeFromContent(info->Content());
  if (FAILED(Load(strPath)))
    return NO_NFO;

  CFileItemList items;
  bool bNfo=false;

  AddonPtr addon;
  ScraperPtr defaultScraper;
  if (!CAddonMgr::Get().GetDefault(m_type, addon))
    return NO_NFO;
  else
    defaultScraper = boost::dynamic_pointer_cast<CScraper>(addon);

  if (m_type == ADDON_SCRAPER_ALBUMS)
  {
    CAlbum album;
    bNfo = GetDetails(album);
  }
  else if (m_type == ADDON_SCRAPER_ARTISTS)
  {
    CArtist artist;
    bNfo = GetDetails(artist);
  }
  else if (m_type == ADDON_SCRAPER_TVSHOWS || m_type == ADDON_SCRAPER_MOVIES || m_type == ADDON_SCRAPER_MUSICVIDEOS)
  {
    // first check if it's an XML file with the info we need
    CVideoInfoTag details;
    bNfo = GetDetails(details);
    if (episode > -1 && bNfo && m_type == ADDON_SCRAPER_TVSHOWS)
    {
      int infos=0;
      while (m_headofdoc && details.m_iEpisode != episode)
      {
        m_headofdoc = strstr(m_headofdoc+1,"<episodedetails>");
        bNfo  = GetDetails(details);
        infos++;
      }
      if (details.m_iEpisode != episode)
      {
        bNfo = false;
        details.Reset();
        m_headofdoc = m_doc;
        if (infos == 1) // still allow differing nfo/file numbers for single ep nfo's
          bNfo = GetDetails(details);
      }
    }
  }

  vector<ScraperPtr> vecScrapers;
  ADDON::ScraperPtr selected;
  // Get Selected Scraper
  if (m_type == ADDON_SCRAPER_MOVIES      ||
      m_type == ADDON_SCRAPER_MUSICVIDEOS ||
      m_type == ADDON_SCRAPER_TVSHOWS)
  {
    CVideoDatabase database;
    database.Open();
    selected = database.GetScraperForPath(strPath);
  }
  if (m_type == ADDON_SCRAPER_ALBUMS ||
      m_type == ADDON_SCRAPER_ARTISTS)
  {
    CMusicDatabase database;
    database.Open();
    database.GetScraperForPath(strPath2,selected,m_type);
  }

  // add selected scraper
  if (selected)
    vecScrapers.push_back(selected);

  VECADDONS addons;
  CAddonMgr::Get().GetAddons(m_type,addons);
  // first pass - add language based scrapers
  if (g_guiSettings.GetBool("scrapers.langfallback"))
    AddScrapers(false,addons,vecScrapers);

  // add default scraper
  if ((selected && selected->ID() != defaultScraper->ID()) || !selected)
    vecScrapers.push_back(defaultScraper);

  AddScrapers(true,addons,vecScrapers);

  // search ..
  int res = -1;
  for (unsigned int i=0;i<vecScrapers.size();++i)
    if ((res = Scrape(vecScrapers[i])) == 0 || res == 2)
      break;

  if (res == 2)
    return ERROR_NFO;
  if (bNfo)
    return m_scurl.m_url.empty() ? FULL_NFO : COMBINED_NFO;
  return m_scurl.m_url.empty() ? NO_NFO : URL_NFO;
}

// return value: 0 - success; 1 - no result; skip; 2 - error
int CNfoFile::Scrape(ScraperPtr& scraper)
{
  if (scraper->Type() != m_type)
    return 1;

  scraper->ClearCache();

  try
  {
    m_scurl = scraper->NfoUrl(m_doc);
  }
  catch (const CScraperError &sce)
  {
    CVideoInfoDownloader::ShowErrorDialog(sce);
    if (!sce.FAborted())
      return 2;
  }

  if (!m_scurl.m_url.empty())
    SetScraperInfo(scraper);
  return m_scurl.m_url.empty() ? 1 : 0;
}

int CNfoFile::Load(const CStdString& strFile)
{
  Close();
  XFILE::CFile file;
  if (file.Open(strFile))
  {
    int size = (int)file.GetLength();
    try
    {
      m_doc = new char[size+1];
      m_headofdoc = m_doc;
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s: Exception while creating file buffer",__FUNCTION__);
      return 1;
    }
    if (!m_doc)
    {
      file.Close();
      return 1;
    }
    file.Read(m_doc, size);
    m_doc[size] = 0;
    file.Close();
    return 0;
  }
  return 1;
}

void CNfoFile::Close()
{
  delete m_doc;
  m_doc = NULL;
  m_scurl.Clear();
}

void CNfoFile::AddScrapers(bool any, VECADDONS& addons,
                           vector<ScraperPtr>& vecScrapers)
{
  for (unsigned i=0;i<addons.size();++i)
  {
    ScraperPtr scraper = boost::dynamic_pointer_cast<CScraper>(addons[i]);

    // skip if scraper requires settings and there's nothing set yet
    if (scraper->RequiresSettings() && !scraper->HasUserSettings())
      continue;

    // add same language and multi-language
    if (any || 
       scraper->Language().Equals(g_langInfo.GetDVDSubtitleLanguage()) ||
       scraper->Language().Equals("multi"))
    {
      if (find(vecScrapers.begin(),vecScrapers.end(),scraper) == vecScrapers.end())
        vecScrapers.push_back(scraper);
    }
  }
}
