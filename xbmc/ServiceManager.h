/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <system.h> // <xtl.h>
#include <boost/shared_ptr.hpp>
#include <boost/move/unique_ptr.hpp>

namespace ADDON
{
class CAddonMgr;
class CBinaryAddonManager;
class CServiceAddonManager;
class CRepositoryUpdater;
} // namespace ADDON

namespace PLAYLIST
{
class CPlayListPlayer;
}

class CContextMenuManager;
#ifdef HAS_PYTHON
class XBPython;
#endif
class CNetwork;
class CWinSystemBase;
class CWeather;

class CPlayerCoreFactory;
class CDatabaseManager;
class CProfileManager;
class CMediaManager;

class CServiceManager
{
public:
  CServiceManager();
  ~CServiceManager();

  bool InitStageOne();
  bool InitStageTwo(const std::string& profilesUserDataFolder);
  bool InitStageThree(const boost::shared_ptr<CProfileManager>& profileManager);
  void DeinitTesting();
  void DeinitStageThree();
  void DeinitStageTwo();
  void DeinitStageOne();

  ADDON::CAddonMgr& GetAddonMgr();
  ADDON::CBinaryAddonManager& GetBinaryAddonManager();
  ADDON::CServiceAddonManager& GetServiceAddons();
  ADDON::CRepositoryUpdater& GetRepositoryUpdater();
  CNetwork& GetNetwork();
#ifdef HAS_PYTHON
  XBPython& GetXBPython();
#endif
  CContextMenuManager& GetContextMenuManager();

  PLAYLIST::CPlayListPlayer& GetPlaylistPlayer();
  int init_level;

  CWeather& GetWeatherManager();

  CPlayerCoreFactory& GetPlayerCoreFactory();

  CDatabaseManager& GetDatabaseManager();

  CMediaManager& GetMediaManager();

protected:
  boost::movelib::unique_ptr<ADDON::CAddonMgr> m_addonMgr;
  boost::movelib::unique_ptr<ADDON::CBinaryAddonManager> m_binaryAddonManager;
  boost::movelib::unique_ptr<ADDON::CServiceAddonManager> m_serviceAddons;
  boost::movelib::unique_ptr<ADDON::CRepositoryUpdater> m_repositoryUpdater;
#ifdef HAS_PYTHON
  boost::movelib::unique_ptr<XBPython> m_XBPython;
#endif
  boost::movelib::unique_ptr<CContextMenuManager> m_contextMenuManager;
  boost::movelib::unique_ptr<PLAYLIST::CPlayListPlayer> m_playlistPlayer;
  boost::movelib::unique_ptr<CNetwork> m_network;
  boost::movelib::unique_ptr<CWeather> m_weatherManager;
  boost::movelib::unique_ptr<CPlayerCoreFactory> m_playerCoreFactory;
  boost::movelib::unique_ptr<CDatabaseManager> m_databaseManager;
  boost::movelib::unique_ptr<CMediaManager> m_mediaManager;
};
