/*
 *      Copyright (C) 2005-2016 Team XBMC
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

#include "ServiceManager.h"

#include "ContextMenuManager.h"
#include "DatabaseManager.h"
#include "PlayListPlayer.h"
#include "addons/RepositoryUpdater.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/python/XBPython.h"
#include "storage/MediaManager.h"
#include "utils/log.h"
#include "utils/Weather.h"

#include <boost/move/make_unique.hpp>

CServiceManager::CServiceManager()
{
}

CServiceManager::~CServiceManager()
{
}

bool CServiceManager::Init1()
{
  m_XBPython.reset(new XBPython());
  CScriptInvocationManager::GetInstance().RegisterLanguageInvocationHandler(m_XBPython.get(), ".py");

  m_playlistPlayer.reset(new PLAYLIST::CPlayListPlayer());

  return true;
}

bool CServiceManager::Init2()
{
  // Initialize the addon database (must be before the addon manager is init'd)
  m_databaseManager = boost::movelib::make_unique<CDatabaseManager>();

  m_addonMgr.reset(new ADDON::CAddonMgr());
  if (!m_addonMgr->Init())
  {
    CLog::Log(LOGFATAL, "CServiceManager::Init: Unable to start CAddonMgr");
    return false;
  }

  m_repositoryUpdater = boost::movelib::make_unique<ADDON::CRepositoryUpdater>();

  m_contextMenuManager.reset(new CContextMenuManager(*m_addonMgr.get()));

  m_weatherManager = boost::movelib::make_unique<CWeather>();

  m_mediaManager = boost::movelib::make_unique<CMediaManager>();

  return true;
}

bool CServiceManager::Init3()
{
  m_contextMenuManager->Init();

  m_playerCoreFactory = boost::movelib::make_unique<CPlayerCoreFactory>();

  return true;
}

void CServiceManager::Deinit()
{
  m_weatherManager.reset();
  m_playerCoreFactory.reset();
  m_contextMenuManager.reset();
  m_repositoryUpdater.reset();
  m_addonMgr.reset();
  m_databaseManager.reset();
  CScriptInvocationManager::GetInstance().UnregisterLanguageInvocationHandler(m_XBPython.get());
  m_XBPython.reset();

  m_mediaManager.reset();
}

ADDON::CAddonMgr &CServiceManager::GetAddonMgr()
{
  return *m_addonMgr.get();
}

ADDON::CRepositoryUpdater& CServiceManager::GetRepositoryUpdater()
{
  return *m_repositoryUpdater;
}

XBPython& CServiceManager::GetXBPython()
{
  return *m_XBPython;
}

CContextMenuManager& CServiceManager::GetContextMenuManager()
{
  return *m_contextMenuManager;
}

PLAYLIST::CPlayListPlayer& CServiceManager::GetPlaylistPlayer()
{
  return *m_playlistPlayer;
}

CWeather& CServiceManager::GetWeatherManager()
{
  return *m_weatherManager;
}

CPlayerCoreFactory& CServiceManager::GetPlayerCoreFactory()
{
  return *m_playerCoreFactory;
}

CDatabaseManager& CServiceManager::GetDatabaseManager()
{
  return *m_databaseManager;
}

CMediaManager& CServiceManager::GetMediaManager()
{
  return *m_mediaManager;
}

void CServiceManager::delete_contextMenuManager::operator()(CContextMenuManager *p) const
{
  delete p;
}
