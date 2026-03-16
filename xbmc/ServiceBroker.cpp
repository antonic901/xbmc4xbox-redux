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

#include "ServiceBroker.h"

#include "Application.h"

#include "windowing/WinSystem.h"

using namespace KODI;

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

ADDON::CAddonMgr &CServiceBroker::GetAddonMgr()
{
  return g_application.m_ServiceManager->GetAddonMgr();
}

XBPython& CServiceBroker::GetXBPython()
{
  return g_application.m_ServiceManager->GetXBPython();
}

CContextMenuManager& CServiceBroker::GetContextMenuManager()
{
  return g_application.m_ServiceManager->GetContextMenuManager();
}

PLAYLIST::CPlayListPlayer &CServiceBroker::GetPlaylistPlayer()
{
  return g_application.m_ServiceManager->GetPlaylistPlayer();
}

ADDON::CRepositoryUpdater& CServiceBroker::GetRepositoryUpdater()
{
  return g_application.m_ServiceManager->GetRepositoryUpdater();
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
  g_serviceBroker.m_pGUI = nullptr;
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
  g_serviceBroker.m_pWinSystem = nullptr;
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
