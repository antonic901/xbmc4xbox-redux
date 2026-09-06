/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"

class CSettings;
class CSNTPClient;
#ifdef HAS_WEB_SERVER
class CWebServer;
#endif // HAS_WEB_SERVER
class CXBFileZilla;

class CNetworkServices : public ISettingCallback
{
public:
  CNetworkServices();
  virtual ~CNetworkServices();

  virtual bool OnSettingChanging(const boost::shared_ptr<const CSetting>& setting);
  virtual void OnSettingChanged(const boost::shared_ptr<const CSetting>& setting);
  virtual bool OnSettingUpdate(const boost::shared_ptr<CSetting>& setting,
                       const char* oldSettingId,
                       const TiXmlNode* oldSettingNode);

  void Start();
  void Stop(bool bWait);

  enum ESERVERS
  {
    ES_WEBSERVER = 1,
    ES_UPNPRENDERER,
    ES_UPNPSERVER,
    ES_EVENTSERVER,
    ES_TIMESERVER,
    ES_FTPSERVER
  };

  bool StartServer(enum ESERVERS server, bool start);

  bool StartWebserver();
  bool IsWebserverRunning();
  bool StopWebserver();

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

  // Xbox services
  bool StartTimeServer();
  bool StopTimeServer();
  void UpdateTimeServer();

  bool StartFtpServer();
  bool StartFtpEmergencyRecoveryMode();
  bool StopFtpServer();
  bool SetFTPServerUserPass();
  bool FtpHasActiveConnections();

private:
  CNetworkServices(const CNetworkServices&);
  CNetworkServices const& operator=(CNetworkServices const&);

  bool ValidatePort(int port);

  // Construction parameters
  boost::shared_ptr<CSettings> m_settings;

  // Network services
#ifdef HAS_WEB_SERVER
  CWebServer* m_webserver;
#endif
  CSNTPClient* m_sntpclient;
  CXBFileZilla* m_filezilla;
};
