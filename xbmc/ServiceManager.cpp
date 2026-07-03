/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceManager.h"

#include "ContextMenuManager.h"
#include "DatabaseManager.h"
#include "PlayListPlayer.h"
#include "addons/RepositoryUpdater.h"
#include "addons/Service.h"
#include "addons/binary-addons/BinaryAddonManager.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/python/XBPython.h"
#include "xbox/Network.h"
#include "profiles/ProfileManager.h"
#include "storage/MediaManager.h"
#include "utils/log.h"
#include "utils/Weather.h"

#include <boost/move/make_unique.hpp>

using namespace KODI;

CServiceManager::CServiceManager() : init_level(0) {}

CServiceManager::~CServiceManager()
{
  if (init_level > 2)
    DeinitStageThree();
  if (init_level > 1)
    DeinitStageTwo();
  if (init_level > 0)
    DeinitStageOne();
}

bool CServiceManager::InitStageOne()
{
#ifdef HAS_PYTHON
  m_XBPython = boost::movelib::make_unique<XBPython>();
  CScriptInvocationManager::GetInstance().RegisterLanguageInvocationHandler(m_XBPython.get(),
                                                                            ".py");
#endif

  m_playlistPlayer = boost::movelib::make_unique<PLAYLIST::CPlayListPlayer>();

  m_network = boost::movelib::make_unique<CNetwork>();

  init_level = 1;
  return true;
}

bool CServiceManager::InitStageTwo(const std::string& profilesUserDataFolder)
{
  // Initialize the addon database (must be before the addon manager is init'd)
  m_databaseManager = boost::movelib::make_unique<CDatabaseManager>();

  m_binaryAddonManager = boost::movelib::make_unique<
      ADDON::
          CBinaryAddonManager>(); /* Need to constructed before, GetRunningInstance() of binary CAddonDll need to call them */
  m_addonMgr = boost::movelib::make_unique<ADDON::CAddonMgr>();
  if (!m_addonMgr->Init())
  {
    CLog::Log(LOGFATAL, "CServiceManager::%s: Unable to start CAddonMgr", __FUNCTION__);
    return false;
  }

  m_repositoryUpdater.reset(new ADDON::CRepositoryUpdater(*m_addonMgr));

  m_serviceAddons.reset(new ADDON::CServiceAddonManager(*m_addonMgr));

  m_contextMenuManager.reset(new CContextMenuManager(*m_addonMgr));

  m_weatherManager = boost::movelib::make_unique<CWeather>();

  m_mediaManager = boost::movelib::make_unique<CMediaManager>();

  init_level = 2;
  return true;
}

// stage 3 is called after successful initialization of WindowManager
bool CServiceManager::InitStageThree(const boost::shared_ptr<CProfileManager>& profileManager)
{
  m_contextMenuManager->Init();

  m_playerCoreFactory = boost::movelib::make_unique<CPlayerCoreFactory>(*profileManager);

  init_level = 3;
  return true;
}

void CServiceManager::DeinitStageThree()
{
  init_level = 2;
  m_playerCoreFactory.reset();
  m_contextMenuManager->Deinit();
}

void CServiceManager::DeinitStageTwo()
{
  init_level = 1;

  m_weatherManager.reset();
  m_contextMenuManager.reset();
  m_serviceAddons.reset();
  m_repositoryUpdater.reset();
  m_binaryAddonManager.reset();
  m_addonMgr.reset();
  m_databaseManager.reset();

  m_mediaManager.reset();
}

void CServiceManager::DeinitStageOne()
{
  init_level = 0;

  m_network.reset();
  m_playlistPlayer.reset();
#ifdef HAS_PYTHON
  CScriptInvocationManager::GetInstance().UnregisterLanguageInvocationHandler(m_XBPython.get());
  m_XBPython.reset();
#endif
}

ADDON::CAddonMgr& CServiceManager::GetAddonMgr()
{
  return *m_addonMgr;
}

ADDON::CBinaryAddonManager& CServiceManager::GetBinaryAddonManager()
{
  return *m_binaryAddonManager;
}

ADDON::CServiceAddonManager& CServiceManager::GetServiceAddons()
{
  return *m_serviceAddons;
}

ADDON::CRepositoryUpdater& CServiceManager::GetRepositoryUpdater()
{
  return *m_repositoryUpdater;
}

#ifdef HAS_PYTHON
XBPython& CServiceManager::GetXBPython()
{
  return *m_XBPython;
}
#endif

CContextMenuManager& CServiceManager::GetContextMenuManager()
{
  return *m_contextMenuManager;
}

PLAYLIST::CPlayListPlayer& CServiceManager::GetPlaylistPlayer()
{
  return *m_playlistPlayer;
}

CNetwork& CServiceManager::GetNetwork()
{
  return *m_network;
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
