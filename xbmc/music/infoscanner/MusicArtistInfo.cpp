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

#include "music/infoscanner/MusicArtistInfo.h"
#include "ScraperParser.h"
#include "addons/Scraper.h"
#include "XMLUtils.h"
#include "settings/Settings.h"
#include "CharsetConverter.h"
#include "utils/log.h"

using namespace MUSIC_GRABBER;
using namespace XFILE;
using namespace std;

CMusicArtistInfo::CMusicArtistInfo(void)
{
  m_bLoaded = false;
}

CMusicArtistInfo::~CMusicArtistInfo(void)
{
}

CMusicArtistInfo::CMusicArtistInfo(const CStdString& strArtist, const CScraperUrl& strArtistURL)
{
  m_artist.strArtist = strArtist;
  m_artistURL = strArtistURL;
  m_bLoaded = false;
}

const CArtist& CMusicArtistInfo::GetArtist() const
{
  return m_artist;
}

CArtist& CMusicArtistInfo::GetArtist()
{
  return m_artist;
}

void CMusicArtistInfo::SetArtist(const CArtist& artist)
{
  m_artist = artist;
  m_bLoaded = true;
}

const CScraperUrl& CMusicArtistInfo::GetArtistURL() const
{
  return m_artistURL;
}

bool CMusicArtistInfo::Parse(const TiXmlElement* artist, bool bChained)
{
  if (!m_artist.Load(artist,bChained))
    return false;

  SetLoaded(true);

  return true;
}

bool CMusicArtistInfo::Load(CCurlFile& http, const ADDON::ScraperPtr& scraper)
{
  // load our scraper xml
  if (!scraper->Load())
    return false;

  vector<CStdString> extras;
  extras.push_back(m_strSearch);

  vector<CStdString> xml = scraper->Run("GetArtistDetails",GetArtistURL(),http,&extras);

  bool ret=true;
  for (vector<CStdString>::iterator it  = xml.begin();
                                    it != xml.end(); ++it)
  {
    // ok, now parse the xml file
    TiXmlDocument doc;
    doc.Parse(it->c_str(),0,TIXML_ENCODING_UTF8);
    if (!doc.RootElement())
    {
      CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
      return false;
    }

    ret = Parse(doc.RootElement(),it!=xml.begin());
  }

  return ret;
}

void CMusicArtistInfo::SetLoaded(bool bOnOff)
{
  m_bLoaded = bOnOff;
}

bool CMusicArtistInfo::Loaded() const
{
  return m_bLoaded;
}
