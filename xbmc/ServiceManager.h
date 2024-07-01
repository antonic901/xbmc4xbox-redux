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

#include <boost/move/unique_ptr.hpp>

namespace ADDON {
class CAddonMgr;
}

namespace ANNOUNCEMENT
{
class CAnnouncementManager;
}

namespace PLAYLIST
{
  class CPlayListPlayer;
}

class CContextMenuManager;
class XBPython;

class CServiceManager
{
public:
  CServiceManager();
  ~CServiceManager();

  bool Init1();
  bool Init2();
  bool Init3();
  void Deinit();
  ADDON::CAddonMgr& GetAddonMgr();
  ANNOUNCEMENT::CAnnouncementManager& GetAnnouncementManager();
  XBPython& GetXBPython();
  CContextMenuManager& GetContextMenuManager();

  PLAYLIST::CPlayListPlayer& GetPlaylistPlayer();

protected:
  struct delete_contextMenuManager
  {
    void operator()(CContextMenuManager *p) const;
  };

  boost::movelib::unique_ptr<ADDON::CAddonMgr> m_addonMgr;
  boost::movelib::unique_ptr<ANNOUNCEMENT::CAnnouncementManager> m_announcementManager;
  boost::movelib::unique_ptr<XBPython> m_XBPython;
  boost::movelib::unique_ptr<CContextMenuManager, delete_contextMenuManager> m_contextMenuManager;
  boost::movelib::unique_ptr<PLAYLIST::CPlayListPlayer> m_playlistPlayer;
};
