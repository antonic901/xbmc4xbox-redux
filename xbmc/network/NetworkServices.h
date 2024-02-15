#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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

#include "system.h"
#include "settings/ISettingCallback.h"

#ifdef HAS_TIME_SERVER
class CSNTPClient;
#endif
#ifdef HAS_WEB_SERVER
class CWebServer;
#endif // HAS_WEB_SERVER
#ifdef HAS_FTP_SERVER
class CXBFileZilla;
#endif

class CNetworkServices : public ISettingCallback
{
public:
  static CNetworkServices& Get();

  virtual bool OnSettingChanging(const CSetting *setting);
  virtual void OnSettingChanged(const CSetting *setting);

  void Start();
  void Stop(bool bWait);

  bool StartTimeServer();
  bool IsTimeServerRunning();
  bool StopTimeServer();
  bool IsTimeServerUpdateNeeded();
  void UpdateTimeServer();

  bool StartWebserver();
  bool IsWebserverRunning();
  bool StopWebserver();

  bool StartFtpServer();
  bool StartFtpEmergencyRecoveryMode();
  bool IsFtpServerRunning();
  bool StopFtpServer();
  bool SetFTPServerUserPass();
  bool FtpHasActiveConnections();
  int GetFtpServerPort();

  bool StartEventServer();
  bool IsEventServerRunning();
  bool StopEventServer(bool bWait, bool promptuser);
  bool RefreshEventServer();

  bool StartUPnP();
  bool StopUPnP(bool bWait);
  bool StartUPnPClient();
  bool IsUPnPClientRunning();
  bool StopUPnPClient();
  bool StartUPnPRenderer();
  bool IsUPnPRendererRunning();
  bool StopUPnPRenderer();
  bool StartUPnPServer();
  bool IsUPnPServerRunning();
  bool StopUPnPServer();

  bool StartRss();
  bool IsRssRunning();
  bool StopRss();

private:
  CNetworkServices();
  CNetworkServices(const CNetworkServices&);
  CNetworkServices const& operator=(CNetworkServices const&);
  virtual ~CNetworkServices();

  bool ValidatePort(int port);

#ifdef HAS_TIME_SERVER
 CSNTPClient* m_sntpclient;
#endif
#ifdef HAS_WEB_SERVER
  CWebServer* m_webserver;
#endif
#ifdef HAS_FTP_SERVER
  CXBFileZilla* m_filezilla;
#endif
};
