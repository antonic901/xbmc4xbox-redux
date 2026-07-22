/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"

#include "ServiceManager.h"
#include "application/Application.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

#include <stdexcept>
#include <utility>

using namespace KODI;

CServiceBroker::CServiceBroker()
{
  m_pGUI = NULL;
  m_pWinSystem = NULL;
}

CServiceBroker::~CServiceBroker()
{
}

// announcement
boost::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> CServiceBroker::GetAnnouncementManager()
{
  return g_serviceBroker.m_pAnnouncementManager;
}
void CServiceBroker::RegisterAnnouncementManager(
    boost::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> port)
{
  g_serviceBroker.m_pAnnouncementManager = boost::move(port);
}

void CServiceBroker::UnregisterAnnouncementManager()
{
  g_serviceBroker.m_pAnnouncementManager.reset();
}

ADDON::CAddonMgr& CServiceBroker::GetAddonMgr()
{
  return g_application.m_ServiceManager->GetAddonMgr();
}

ADDON::CBinaryAddonManager& CServiceBroker::GetBinaryAddonManager()
{
  return g_application.m_ServiceManager->GetBinaryAddonManager();
}

#ifdef HAS_PYTHON
XBPython& CServiceBroker::GetXBPython()
{
  return g_application.m_ServiceManager->GetXBPython();
}
#endif

CContextMenuManager& CServiceBroker::GetContextMenuManager()
{
  return g_application.m_ServiceManager->GetContextMenuManager();
}

PLAYLIST::CPlayListPlayer& CServiceBroker::GetPlaylistPlayer()
{
  return g_application.m_ServiceManager->GetPlaylistPlayer();
}

void CServiceBroker::RegisterSettingsComponent(const boost::shared_ptr<CSettingsComponent>& settings)
{
  g_serviceBroker.m_pSettingsComponent = settings;
}

void CServiceBroker::UnregisterSettingsComponent()
{
  g_serviceBroker.m_pSettingsComponent.reset();
}

boost::shared_ptr<CSettingsComponent> CServiceBroker::GetSettingsComponent()
{
  return g_serviceBroker.m_pSettingsComponent;
}

ADDON::CServiceAddonManager& CServiceBroker::GetServiceAddons()
{
  return g_application.m_ServiceManager->GetServiceAddons();
}

ADDON::CRepositoryUpdater& CServiceBroker::GetRepositoryUpdater()
{
  return g_application.m_ServiceManager->GetRepositoryUpdater();
}

CInputManager& CServiceBroker::GetInputManager()
{
  return g_application.m_ServiceManager->GetInputManager();
}

CNetwork& CServiceBroker::GetNetwork()
{
  return g_application.m_ServiceManager->GetNetwork();
}

bool CServiceBroker::IsAddonInterfaceUp()
{
  return g_application.m_ServiceManager && g_application.m_ServiceManager->init_level > 1;
}

bool CServiceBroker::IsServiceManagerUp()
{
  return g_application.m_ServiceManager && g_application.m_ServiceManager->init_level == 3;
}

CWinSystemBase* CServiceBroker::GetWinSystem()
{
  return g_serviceBroker.m_pWinSystem;
}

void CServiceBroker::RegisterWinSystem(CWinSystemBase* winsystem)
{
  g_serviceBroker.m_pWinSystem = winsystem;
}

void CServiceBroker::UnregisterWinSystem()
{
  g_serviceBroker.m_pWinSystem = NULL;
}

CRenderSystemBase* CServiceBroker::GetRenderSystem()
{
  if (g_serviceBroker.m_pWinSystem)
    return g_serviceBroker.m_pWinSystem->GetRenderSystem();

  return NULL;
}

CWeather& CServiceBroker::GetWeatherManager()
{
  return g_application.m_ServiceManager->GetWeatherManager();
}

CPlayerCoreFactory& CServiceBroker::GetPlayerCoreFactory()
{
  return g_application.m_ServiceManager->GetPlayerCoreFactory();
}

CDatabaseManager& CServiceBroker::GetDatabaseManager()
{
  return g_application.m_ServiceManager->GetDatabaseManager();
}

CMediaManager& CServiceBroker::GetMediaManager()
{
  return g_application.m_ServiceManager->GetMediaManager();
}

CApplicationComponents& CServiceBroker::GetAppComponents()
{
  return g_application;
}

CGUIComponent* CServiceBroker::GetGUI()
{
  return g_serviceBroker.m_pGUI;
}

void CServiceBroker::RegisterGUI(CGUIComponent* gui)
{
  g_serviceBroker.m_pGUI = gui;
}

void CServiceBroker::UnregisterGUI()
{
  g_serviceBroker.m_pGUI = NULL;
}

void CServiceBroker::RegisterTextureCache(const boost::shared_ptr<CTextureCache>& cache)
{
  g_serviceBroker.m_textureCache = cache;
}

void CServiceBroker::UnregisterTextureCache()
{
  g_serviceBroker.m_textureCache.reset();
}

boost::shared_ptr<CTextureCache> CServiceBroker::GetTextureCache()
{
  return g_serviceBroker.m_textureCache;
}

void CServiceBroker::RegisterJobManager(const boost::shared_ptr<CJobManager>& jobManager)
{
  g_serviceBroker.m_jobManager = jobManager;
}

void CServiceBroker::UnregisterJobManager()
{
  g_serviceBroker.m_jobManager.reset();
}

boost::shared_ptr<CJobManager> CServiceBroker::GetJobManager()
{
  return g_serviceBroker.m_jobManager;
}

void CServiceBroker::RegisterAppMessenger(
    const boost::shared_ptr<KODI::MESSAGING::CApplicationMessenger>& appMessenger)
{
  g_serviceBroker.m_appMessenger = appMessenger;
}

void CServiceBroker::UnregisterAppMessenger()
{
  g_serviceBroker.m_appMessenger.reset();
}

boost::shared_ptr<KODI::MESSAGING::CApplicationMessenger> CServiceBroker::GetAppMessenger()
{
  return g_serviceBroker.m_appMessenger;
}

void CServiceBroker::RegisterKeyboardLayoutManager(
    const boost::shared_ptr<KEYBOARD::CKeyboardLayoutManager>& keyboardLayoutManager)
{
  g_serviceBroker.m_keyboardLayoutManager = keyboardLayoutManager;
}

void CServiceBroker::UnregisterKeyboardLayoutManager()
{
  g_serviceBroker.m_keyboardLayoutManager.reset();
}

boost::shared_ptr<KEYBOARD::CKeyboardLayoutManager> CServiceBroker::GetKeyboardLayoutManager()
{
  return g_serviceBroker.m_keyboardLayoutManager;
}
