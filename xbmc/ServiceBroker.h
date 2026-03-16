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
class CWinSystemBase;
class CGUIComponent;
class XBPython;

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
  static ADDON::CAddonMgr &GetAddonMgr();
  static ANNOUNCEMENT::CAnnouncementManager &GetAnnouncementManager();
  static XBPython &GetXBPython();
  static CContextMenuManager& GetContextMenuManager();
  static PLAYLIST::CPlayListPlayer& GetPlaylistPlayer();

  static CGUIComponent* GetGUI();
  static void RegisterGUI(CGUIComponent* gui);
  static void UnregisterGUI();

  static void RegisterWinSystem(CWinSystemBase* winsystem);
  static void UnregisterWinSystem();
  static CWinSystemBase* GetWinSystem();

  static void RegisterAppMessenger(
      const boost::shared_ptr<KODI::MESSAGING::CApplicationMessenger>& appMessenger);
  static void UnregisterAppMessenger();
  static boost::shared_ptr<KODI::MESSAGING::CApplicationMessenger> GetAppMessenger();

  static void RegisterKeyboardLayoutManager(
      const boost::shared_ptr<KODI::KEYBOARD::CKeyboardLayoutManager>& keyboardLayoutManager);
  static void UnregisterKeyboardLayoutManager();
  static boost::shared_ptr<KODI::KEYBOARD::CKeyboardLayoutManager> GetKeyboardLayoutManager();

private:
  CGUIComponent* m_pGUI;
  CWinSystemBase* m_pWinSystem;
  boost::shared_ptr<KODI::MESSAGING::CApplicationMessenger> m_appMessenger;
  boost::shared_ptr<KODI::KEYBOARD::CKeyboardLayoutManager> m_keyboardLayoutManager;
};

XBMC_GLOBAL_REF(CServiceBroker, g_serviceBroker);
#define g_serviceBroker XBMC_GLOBAL_USE(CServiceBroker)
