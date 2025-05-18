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
#include "PlayListPlayer.h"
#include "utils/log.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/python/XBPython.h"

CServiceManager::CServiceManager()
{
}

CServiceManager::~CServiceManager()
{
}

bool CServiceManager::Init1()
{
  m_announcementManager.reset(new ANNOUNCEMENT::CAnnouncementManager());
  m_announcementManager->Start();

  m_XBPython.reset(new XBPython());
  CScriptInvocationManager::GetInstance().RegisterLanguageInvocationHandler(m_XBPython.get(), ".py");

  m_playlistPlayer.reset(new PLAYLIST::CPlayListPlayer());

  return true;
}

bool CServiceManager::Init2()
{
  m_addonMgr.reset(new ADDON::CAddonMgr());
  if (!m_addonMgr->Init())
  {
    CLog::Log(LOGFATAL, "CServiceManager::Init: Unable to start CAddonMgr");
    return false;
  }

  m_contextMenuManager.reset(new CContextMenuManager(*m_addonMgr.get()));

  return true;
}

bool CServiceManager::Init3()
{
  m_contextMenuManager->Init();

  return true;
}

void CServiceManager::Deinit()
{
  m_contextMenuManager.reset();
  m_addonMgr.reset();
  CScriptInvocationManager::GetInstance().UnregisterLanguageInvocationHandler(m_XBPython.get());
  m_XBPython.reset();
  m_announcementManager.reset();
}

ADDON::CAddonMgr &CServiceManager::GetAddonMgr()
{
  return *m_addonMgr.get();
}

ANNOUNCEMENT::CAnnouncementManager& CServiceManager::GetAnnouncementManager()
{
  return *m_announcementManager;
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

void CServiceManager::delete_contextMenuManager::operator()(CContextMenuManager *p) const
{
  delete p;
}
