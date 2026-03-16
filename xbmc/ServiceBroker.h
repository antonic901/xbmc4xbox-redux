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

#pragma once

#include "utils/GlobalsHandling.h"

namespace ADDON
{
class CAddonMgr;
class CBinaryAddonCache;
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

class CContextMenuManager;
class XBPython;
class CWinSystemBase;
class CWeather;
class CPlayerCoreFactory;
class CDatabaseManager;
class CGUIComponent;
class CMediaManager;
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
  static boost::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> GetAnnouncementManager();
  static void RegisterAnnouncementManager(
      boost::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> announcementManager);
  static void UnregisterAnnouncementManager();

  static ADDON::CAddonMgr &GetAddonMgr();
  static XBPython &GetXBPython();
  static CContextMenuManager& GetContextMenuManager();
  static PLAYLIST::CPlayListPlayer& GetPlaylistPlayer();
  static ADDON::CRepositoryUpdater& GetRepositoryUpdater();
  static CWeather& GetWeatherManager();
  static CPlayerCoreFactory& GetPlayerCoreFactory();
  static CDatabaseManager& GetDatabaseManager();
  static CMediaManager& GetMediaManager();

  static CGUIComponent* GetGUI();
  static void RegisterGUI(CGUIComponent* gui);
  static void UnregisterGUI();

  static void RegisterWinSystem(CWinSystemBase* winsystem);
  static void UnregisterWinSystem();
  static CWinSystemBase* GetWinSystem();

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
  boost::shared_ptr<CJobManager> m_jobManager;
  boost::shared_ptr<KODI::MESSAGING::CApplicationMessenger> m_appMessenger;
  boost::shared_ptr<KODI::KEYBOARD::CKeyboardLayoutManager> m_keyboardLayoutManager;
};

XBMC_GLOBAL_REF(CServiceBroker, g_serviceBroker);
#define g_serviceBroker XBMC_GLOBAL_USE(CServiceBroker)
