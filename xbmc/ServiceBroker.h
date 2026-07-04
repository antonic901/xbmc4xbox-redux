/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/GlobalsHandling.h"

namespace ADDON
{
class CAddonMgr;
class CBinaryAddonManager;
class CServiceAddonManager;
class CRepositoryUpdater;
} // namespace ADDON

namespace ANNOUNCEMENT
{
class CAnnouncementManager;
}

namespace PLAYLIST
{
class CPlayListPlayer;
}

namespace KODI
{
namespace MESSAGING
{
class CApplicationMessenger;
}
} // namespace KODI

template<class T>
class CComponentContainer;
class CContextMenuManager;
class XBPython;
class IApplicationComponent;
class CNetwork;
class CWinSystemBase;
class CRenderSystemBase;
class CWeather;
class CPlayerCoreFactory;
class CDatabaseManager;
class CGUIComponent;
class CSettingsComponent;
class CMediaManager;
class CTextureCache;
class CJobManager;

namespace KODI
{
namespace KEYBOARD
{
class CKeyboardLayoutManager;
} // namespace KEYBOARD
} // namespace KODI

class CServiceBroker
{
public:
  CServiceBroker();
  ~CServiceBroker();

  static boost::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> GetAnnouncementManager();
  static void RegisterAnnouncementManager(
      boost::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> announcementManager);
  static void UnregisterAnnouncementManager();

  static ADDON::CAddonMgr& GetAddonMgr();
  static ADDON::CBinaryAddonManager& GetBinaryAddonManager();
  static XBPython& GetXBPython();
  static CContextMenuManager& GetContextMenuManager();
  static PLAYLIST::CPlayListPlayer& GetPlaylistPlayer();
  static ADDON::CServiceAddonManager& GetServiceAddons();
  static ADDON::CRepositoryUpdater& GetRepositoryUpdater();
  static bool IsAddonInterfaceUp();
  static bool IsServiceManagerUp();
  static CNetwork& GetNetwork();
  static CWeather& GetWeatherManager();
  static CPlayerCoreFactory& GetPlayerCoreFactory();
  static CDatabaseManager& GetDatabaseManager();
  static CMediaManager& GetMediaManager();
  static CComponentContainer<IApplicationComponent>& GetAppComponents();

  static CGUIComponent* GetGUI();
  static void RegisterGUI(CGUIComponent* gui);
  static void UnregisterGUI();

  static void RegisterSettingsComponent(const boost::shared_ptr<CSettingsComponent>& settings);
  static void UnregisterSettingsComponent();
  static boost::shared_ptr<CSettingsComponent> GetSettingsComponent();

  static void RegisterWinSystem(CWinSystemBase* winsystem);
  static void UnregisterWinSystem();
  static CWinSystemBase* GetWinSystem();
  static CRenderSystemBase* GetRenderSystem();

  static void RegisterTextureCache(const boost::shared_ptr<CTextureCache>& cache);
  static void UnregisterTextureCache();
  static boost::shared_ptr<CTextureCache> GetTextureCache();

  static void RegisterJobManager(const boost::shared_ptr<CJobManager>& jobManager);
  static void UnregisterJobManager();
  static boost::shared_ptr<CJobManager> GetJobManager();

  static void RegisterAppMessenger(
      const boost::shared_ptr<KODI::MESSAGING::CApplicationMessenger>& appMessenger);
  static void UnregisterAppMessenger();
  static boost::shared_ptr<KODI::MESSAGING::CApplicationMessenger> GetAppMessenger();

  static void RegisterKeyboardLayoutManager(
      const boost::shared_ptr<KODI::KEYBOARD::CKeyboardLayoutManager>& keyboardLayoutManager);
  static void UnregisterKeyboardLayoutManager();
  static boost::shared_ptr<KODI::KEYBOARD::CKeyboardLayoutManager> GetKeyboardLayoutManager();

private:
  boost::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_pAnnouncementManager;
  CGUIComponent* m_pGUI;
  CWinSystemBase* m_pWinSystem;
  boost::shared_ptr<CSettingsComponent> m_pSettingsComponent;
  boost::shared_ptr<CTextureCache> m_textureCache;
  boost::shared_ptr<CJobManager> m_jobManager;
  boost::shared_ptr<KODI::MESSAGING::CApplicationMessenger> m_appMessenger;
  boost::shared_ptr<KODI::KEYBOARD::CKeyboardLayoutManager> m_keyboardLayoutManager;
};

XBMC_GLOBAL_REF(CServiceBroker, g_serviceBroker);
#define g_serviceBroker XBMC_GLOBAL_USE(CServiceBroker)
