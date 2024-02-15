/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "NetworkServices.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIInfoManager.h"
#include "Util.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/LocalizeStrings.h"
#include "xbox/Network.h"
#include "xbox/IoSupport.h"

#ifdef HAS_EVENT_SERVER
#include "network/EventServer.h"
#endif // HAS_EVENT_SERVER

#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#endif // HAS_UPNP

#ifdef HAS_WEB_SERVER
#include "lib/libGoAhead/WebServer.h"
#include "lib/libGoAhead/XBMChttp.h"
#endif // HAS_WEB_SERVER

#ifdef HAS_FTP_SERVER
#include "lib/libFileZilla/XBFileZilla.h"
#endif

#ifdef HAS_TIME_SERVER
#include "utils/Sntp.h"
#endif

#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Setting.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/RssManager.h"
#include "SectionLoader.h"

using namespace std;
#ifdef HAS_EVENT_SERVER
using namespace EVENTSERVER;
#endif // HAS_EVENT_SERVER

CNetworkServices::CNetworkServices()
  :
#ifdef HAS_WEB_SERVER
  m_webserver(NULL),
#endif // HAS_WEB_SERVER
  m_sntpclient(NULL),
  m_filezilla(NULL)
{
}

CNetworkServices::~CNetworkServices()
{
#ifdef HAS_TIME_SERVER
  delete m_sntpclient;
#endif
#ifdef HAS_WEB_SERVER
  delete m_webserver;
#endif // HAS_WEB_SERVER
#ifdef HAS_FTP_SERVER
  delete m_filezilla;
#endif
}

CNetworkServices& CNetworkServices::Get()
{
  static CNetworkServices sNetworkServices;
  return sNetworkServices;
}

bool CNetworkServices::OnSettingChanging(const CSetting *setting)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();
#ifdef HAS_WEB_SERVER
  if (settingId == "services.webserver" ||
      settingId == "services.webserverport")
  {
    if (IsWebserverRunning() && !StopWebserver())
      return false;

    if (CSettings::Get().GetBool("services.webserver"))
    {
      if (!StartWebserver())
      {
        CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(33101), "", g_localizeStrings.Get(33100), "");
        return false;
      }
    }
  }
  else if (settingId == "services.esport" ||
           settingId == "services.webserverport")
    return ValidatePort(((CSettingInt*)setting)->GetValue());
  else
#endif // HAS_WEB_SERVER

#ifdef HAS_FTP_SERVER
  if (settingId == "services.ftpserveruser" || settingId == "services.ftpserverpassword")
    return SetFTPServerUserPass();
  else
#endif // HAS_FTP_SERVER

#ifdef HAS_UPNP
  if (settingId == "services.upnpserver")
  {
    if (((CSettingBool*)setting)->GetValue())
      return StartUPnPServer();
    else
      return StopUPnPServer();
  }
  else if (settingId == "services.upnprenderer")
  {
    if (((CSettingBool*)setting)->GetValue())
      return StartUPnPRenderer();
    else
      return StopUPnPRenderer();
  }
  else if (settingId == "services.upnpcontroller")
  {
    // always stop and restart
    StopUPnPClient();
    if (((CSettingBool*)setting)->GetValue())
      return StartUPnPClient();
  }
  // else
#endif // HAS_UPNP

  if (settingId == "services.esenabled")
  {
#ifdef HAS_EVENT_SERVER
    if (((CSettingBool*)setting)->GetValue())
    {
      if (!StartEventServer())
      {
        CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(33102), "", g_localizeStrings.Get(33100), "");
        return false;
      }
    }
    else
      return StopEventServer(true, true);
#endif // HAS_EVENT_SERVER
  }
  else if (settingId == "services.esport")
  {
#ifdef HAS_EVENT_SERVER
    // restart eventserver without asking user
    if (!StopEventServer(true, false))
      return false;

    if (!StartEventServer())
    {
      CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(33102), "", g_localizeStrings.Get(33100), "");
      return false;
    }
#endif // HAS_EVENT_SERVER
  }
  else if (settingId == "services.esallinterfaces")
  {
#ifdef HAS_EVENT_SERVER
    if (CSettings::Get().GetBool("services.esenabled"))
    {
      if (!StopEventServer(true, true))
        return false;

      if (!StartEventServer())
      {
        CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(33102), "", g_localizeStrings.Get(33100), "");
        return false;
      }
    }
#endif // HAS_EVENT_SERVER
  }

#ifdef HAS_EVENT_SERVER
  else if (settingId == "services.esinitialdelay" ||
           settingId == "services.escontinuousdelay")
  {
    if (CSettings::Get().GetBool("services.esenabled"))
      return RefreshEventServer();
  }
#endif // HAS_EVENT_SERVER

  return true;
}

void CNetworkServices::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
#ifdef HAS_TIME_SERVER
  if (settingId == "services.timeserver")
  {
    if (((CSettingBool*)setting)->GetValue())
      StartTimeServer();
    else
      StopTimeServer();
  }
  else if (settingId == "services.timeserveraddress")
  {
    StopTimeServer();
    StartTimeServer();
  }
  else
#endif
#ifdef HAS_FTP_SERVER
  if (settingId == "services.ftpserver")
  {
    if (((CSettingBool*)setting)->GetValue())
      StartFtpServer();
    else
      StopFtpServer();
  }
  else
#endif
#ifdef HAS_WEB_SERVER
  if (settingId == "services.webserverusername" ||
      settingId == "services.webserverpassword")
  {
    if (settingId == "services.webserverusername")
      m_webserver->SetUserName(((CSettingString*)setting)->GetValue().c_str());
    else if(settingId == "services.webserverpassword")
      m_webserver->SetPassword(((CSettingString*)setting)->GetValue().c_str());
  }
  else
#endif // HAS_WEB_SERVER
  if (settingId == "smb.winsserver" ||
      settingId == "smb.workgroup")
  {
    // okey we really don't need to restart, only deinit samba, but that could be damn hard if something is playing
    // TODO - General way of handling setting changes that require restart
    if (CGUIDialogYesNo::ShowAndGetInput(14038, 14039, 14040, -1, -1))
    {
      CSettings::Get().Save();
      g_application.getApplicationMessenger().RestartApp();
    }
  }
}

void CNetworkServices::Start()
{
  StartTimeServer();
  if (CSettings::Get().GetBool("services.webserver") && !StartWebserver())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33101), g_localizeStrings.Get(33100));
  StartFtpServer();
  StartUPnP();
  if (CSettings::Get().GetBool("services.esenabled") && !StartEventServer())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33102), g_localizeStrings.Get(33100));
  StartRss();
}

void CNetworkServices::Stop(bool bWait)
{
  if (bWait)
  {
    StopTimeServer();
    StopWebserver();
    StopFtpServer();
    StopRss();
  }

  StopEventServer(bWait, false);
}

bool CNetworkServices::StartTimeServer()
{
#ifdef HAS_TIME_SERVER
  if (!g_application.getNetwork().IsAvailable())
    return false;

  if (!CSettings::Get().GetBool("services.timeserver"))
    return false;

  if(!IsTimeServerRunning())
  {
    CSectionLoader::Load("SNTP");
    CLog::Log(LOGNOTICE, "start timeserver client");
    m_sntpclient = new CSNTPClient();
    m_sntpclient->Update();
  }

  return true;
#endif
  return false;
}

bool CNetworkServices::IsTimeServerRunning()
{
#ifdef HAS_TIME_SERVER
  return m_sntpclient != NULL;
#endif
  return false;
}

bool CNetworkServices::StopTimeServer()
{
#ifdef HAS_TIME_SERVER
  if (m_sntpclient)
  {
    CLog::Log(LOGNOTICE, "stop time server client");
    SAFE_DELETE(m_sntpclient);
    CSectionLoader::Unload("SNTP");
  }
  return true;
#endif
  return false;
}

bool CNetworkServices::IsTimeServerUpdateNeeded()
{
  return m_sntpclient->UpdateNeeded();
}

void CNetworkServices::UpdateTimeServer()
{
  m_sntpclient->Update();
}

bool CNetworkServices::StartWebserver()
{
#ifdef HAS_WEB_SERVER
  if (!g_application.getNetwork().IsAvailable())
    return false;

  if (!CSettings::Get().GetBool("services.webserver"))
    return false;

  int webPort = CSettings::Get().GetInt("services.webserverport");
  if (!ValidatePort(webPort))
  {
    CLog::Log(LOGERROR, "Cannot start Web Server on port %i", webPort);
    return false;
  }

  if (IsWebserverRunning())
    return true;

  CLog::Log(LOGNOTICE, "Webserver: Starting...");
  CSectionLoader::Load("LIBHTTP");
  m_webserver = new CWebServer();
  if(!m_webserver->Start(webPort, false))
  {
    delete m_webserver;
    m_webserver = NULL;
    return false;
  }

  if (m_webserver)
  {
    m_webserver->SetUserName(CSettings::Get().GetString("services.webserverusername").c_str());
    m_webserver->SetPassword(CSettings::Get().GetString("services.webserverpassword").c_str());
  }
  if (m_webserver && m_pXbmcHttp && CSettings::Get().GetInt("services.httpapibroadcastlevel")>=1)
    g_application.getApplicationMessenger().HttpApi("broadcastlevel; StartUp;1");
  return true;
#endif // HAS_WEB_SERVER
  return false;
}

bool CNetworkServices::IsWebserverRunning()
{
#ifdef HAS_WEB_SERVER
  return m_webserver != NULL;
#endif // HAS_WEB_SERVER
  return false;
}

bool CNetworkServices::StopWebserver()
{
#ifdef HAS_WEB_SERVER
  if (!IsWebserverRunning())
    return true;

  CLog::Log(LOGNOTICE, "Webserver: Stopping...");
  m_webserver->Stop();
  delete m_webserver;
  m_webserver = NULL;
  CSectionLoader::Unload("LIBHTTP");
  CLog::Log(LOGNOTICE, "Webserver: Stopped...");
  return true;
#endif // HAS_WEB_SERVER
  return false;
}

bool CNetworkServices::StartFtpServer()
{
#ifdef HAS_FTP_SERVER
  if (!g_application.getNetwork().IsAvailable())
    return false;

  if (!CSettings::Get().GetBool("services.ftpserver"))
    return false;

  CLog::Log(LOGNOTICE, "XBFileZilla: Starting...");
  if (!IsFtpServerRunning())
  {
    CStdString xmlpath = "special://xbmc/system/";
    // if user didn't upgrade properly,
    // check whether UserData/FileZilla Server.xml exists
    if (XFILE::CFile::Exists(CProfilesManager::Get().GetUserDataItem("FileZilla Server.xml")))
      xmlpath = CProfilesManager::Get().GetUserDataFolder();

    // check file size and presence
    XFILE::CFile xml;
    if (xml.Open(xmlpath+"FileZilla Server.xml") && xml.GetLength() > 0)
    {
      m_filezilla = new CXBFileZilla(CSpecialProtocol::TranslatePath(xmlpath));
      m_filezilla->Start(false);
    }
    else
    {
      // 'FileZilla Server.xml' does not exist or is corrupt, 
      // falling back to ftp emergency recovery mode
      CLog::Log(LOGNOTICE, "XBFileZilla: 'FileZilla Server.xml' is missing or is corrupt!");
      CLog::Log(LOGNOTICE, "XBFileZilla: Starting ftp emergency recovery mode");
      StartFtpEmergencyRecoveryMode();
    }
    xml.Close();
  }
  return true;
#endif
  return false;
}

bool CNetworkServices::StartFtpEmergencyRecoveryMode()
{
#ifdef HAS_FTP_SERVER
  m_filezilla = new CXBFileZilla(NULL);
  m_filezilla->Start();

  // Default settings
  m_filezilla->mSettings.SetMaxUsers(0);
  m_filezilla->mSettings.SetWelcomeMessage("XBMC emergency recovery console FTP.");

  // default user
  CXFUser* pUser;
  m_filezilla->AddUser("xbox", pUser);
  pUser->SetPassword("xbox");
  pUser->SetShortcutsEnabled(false);
  pUser->SetUseRelativePaths(false);
  pUser->SetBypassUserLimit(false);
  pUser->SetUserLimit(0);
  pUser->SetIPLimit(0);
  pUser->AddDirectory("/", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS | XBDIR_HOME);
  pUser->AddDirectory("C:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  pUser->AddDirectory("D:\\", XBFILE_READ | XBDIR_LIST | XBDIR_SUBDIRS);
  pUser->AddDirectory("E:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  pUser->AddDirectory("Q:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  //Add existing extended partitions
  if (CIoSupport::DriveExists('F')){
    pUser->AddDirectory("F:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  }
  if (CIoSupport::DriveExists('G')){
    pUser->AddDirectory("G:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  }
  if (CIoSupport::DriveExists('R')){
    pUser->AddDirectory("R:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  }
  if (CIoSupport::DriveExists('S')){
    pUser->AddDirectory("S:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  }
  if (CIoSupport::DriveExists('V')){
    pUser->AddDirectory("V:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  }
  if (CIoSupport::DriveExists('W')){
    pUser->AddDirectory("W:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  }
  if (CIoSupport::DriveExists('A')){
    pUser->AddDirectory("A:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  }
  if (CIoSupport::DriveExists('B')){
    pUser->AddDirectory("B:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  }
  pUser->CommitChanges();
  return true;
#endif
  return false;
}

bool CNetworkServices::IsFtpServerRunning()
{
#ifdef HAS_FTP_SERVER
  return m_filezilla != NULL;
#endif
  return false;
}

bool CNetworkServices::StopFtpServer()
{
#ifdef HAS_FTP_SERVER
  if (IsFtpServerRunning())
  {
    CLog::Log(LOGINFO, "XBFileZilla: Stopping...");
    m_filezilla->Stop();
    delete m_filezilla;
    m_filezilla = NULL;
    CLog::Log(LOGINFO, "XBFileZilla: Stopped");
  }
  return true;
#endif
  return false;
}

bool CNetworkServices::SetFTPServerUserPass()
{
#ifdef HAS_FTP_SERVER
  if(!m_filezilla)
    return false;

  // TODO: Read the FileZilla Server XML and Set it here!
  // Get GUI USER and pass and set pass to FTP Server
  CStdString strFtpUserName, strFtpUserPassword;
  strFtpUserName      = CSettings::Get().GetString("services.ftpserveruser");
  strFtpUserPassword  = CSettings::Get().GetString("services.ftpserverpassword");

  if(strFtpUserPassword.size() == 0)
  { // PopUp OK and Display: FTP Server Password is empty! Try Again!
    CGUIDialogOK::ShowAndGetInput(728, 0, 12358, 0);
    return false;
  }

  CStdString strTempUserName;
  class CXFUser* p_ftpUser;
  vector<CXFUser*> v_ftpusers;
  m_filezilla->GetAllUsers(v_ftpusers);
  int iUserSize = v_ftpusers.size();
  if (iUserSize > 0)
  {
    int i = 1 ;
    while( i <= iUserSize)
    {
      p_ftpUser = v_ftpusers[i-1];
      strTempUserName = p_ftpUser->GetName();
      if (strTempUserName.Equals(strFtpUserName.c_str()) )
      {
        if (p_ftpUser->SetPassword(strFtpUserPassword.c_str()) != XFS_INVALID_PARAMETERS)
        {
          p_ftpUser->CommitChanges();
          CSettings::Get().SetString("services.ftpserverpassword",strFtpUserPassword.c_str());
          CGUIDialogOK::ShowAndGetInput(728, 0, 1247, 0);
          return true;
        }
        break;
      }
      i++;
    }
  }
#endif
  return false;
}

bool CNetworkServices::FtpHasActiveConnections()
{
  if (IsFtpServerRunning())
    return m_filezilla->GetNoConnections() != 0;
  return false;
}

int CNetworkServices::GetFtpServerPort()
{
  if (IsFtpServerRunning())
    return m_filezilla->mSettings.GetServerPort();
  return 0;
}

bool CNetworkServices::StartEventServer()
{
#ifdef HAS_EVENT_SERVER
  if (!CSettings::Get().GetBool("services.esenabled"))
    return false;

  if (IsEventServerRunning())
    return true;

  CEventServer* server = CEventServer::GetInstance();
  if (!server)
  {
    CLog::Log(LOGERROR, "ES: Out of memory");
    return false;
  }

  CLog::Log(LOGNOTICE, "ES: Starting event server");
  server->StartServer();

  return true;
#endif // HAS_EVENT_SERVER
  return false;
}

bool CNetworkServices::IsEventServerRunning()
{
#ifdef HAS_EVENT_SERVER
  return CEventServer::GetInstance()->Running();
#endif // HAS_EVENT_SERVER
  return false;
}

bool CNetworkServices::StopEventServer(bool bWait, bool promptuser)
{
#ifdef HAS_EVENT_SERVER
  if (!IsEventServerRunning())
    return true;

  CEventServer* server = CEventServer::GetInstance();
  if (!server)
  {
    CLog::Log(LOGERROR, "ES: Out of memory");
    return false;
  }

  if (promptuser)
  {
    if (server->GetNumberOfClients() > 0)
    {
      bool cancelled = false;
      if (!CGUIDialogYesNo::ShowAndGetInput(13140, 13141, 13142, 20022,
                                            -1, -1, cancelled, 10000)
          || cancelled)
      {
        CLog::Log(LOGNOTICE, "ES: Not stopping event server");
        return false;
      }
    }
    CLog::Log(LOGNOTICE, "ES: Stopping event server with confirmation");

    CEventServer::GetInstance()->StopServer(true);
  }
  else
  {
    if (!bWait)
      CLog::Log(LOGNOTICE, "ES: Stopping event server");

    CEventServer::GetInstance()->StopServer(bWait);
  }

  return true;
#endif // HAS_EVENT_SERVER
  return false;
}

bool CNetworkServices::RefreshEventServer()
{
#ifdef HAS_EVENT_SERVER
  if (!CSettings::Get().GetBool("services.esenabled"))
    return false;

  if (!IsEventServerRunning())
    return false;

  CEventServer::GetInstance()->RefreshSettings();
  return true;
#endif // HAS_EVENT_SERVER
  return false;
}

bool CNetworkServices::StartUPnP()
{
  bool ret = false;
#ifdef HAS_UPNP
  ret |= StartUPnPClient();
  ret |= StartUPnPServer();
  ret |= StartUPnPRenderer();
#endif // HAS_UPNP
  return ret;
}

bool CNetworkServices::StopUPnP(bool bWait)
{
#ifdef HAS_UPNP
  if (!CUPnP::IsInstantiated())
    return true;

  CLog::Log(LOGNOTICE, "stopping upnp");
  CUPnP::ReleaseInstance(bWait);

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPClient()
{
#ifdef HAS_UPNP
  if (!CSettings::Get().GetBool("services.upnpcontroller"))
    return false;

  CLog::Log(LOGNOTICE, "starting upnp controller");
  CUPnP::GetInstance()->StartClient();
  return IsUPnPClientRunning();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::IsUPnPClientRunning()
{
#ifdef HAS_UPNP
  return CUPnP::GetInstance()->IsClientStarted();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StopUPnPClient()
{
#ifdef HAS_UPNP
  if (!IsUPnPRendererRunning())
    return true;

  CLog::Log(LOGNOTICE, "stopping upnp client");
  CUPnP::GetInstance()->StopClient();

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPRenderer()
{
#ifdef HAS_UPNP
  if (!CSettings::Get().GetBool("services.upnprenderer"))
    return false;

  CLog::Log(LOGNOTICE, "starting upnp renderer");
  return CUPnP::GetInstance()->StartRenderer();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::IsUPnPRendererRunning()
{
#ifdef HAS_UPNP
  return CUPnP::GetInstance()->IsInstantiated();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StopUPnPRenderer()
{
#ifdef HAS_UPNP
  if (!IsUPnPRendererRunning())
    return true;

  CLog::Log(LOGNOTICE, "stopping upnp renderer");
  CUPnP::GetInstance()->StopRenderer();

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPServer()
{
#ifdef HAS_UPNP
  if (!CSettings::Get().GetBool("services.upnpserver"))
    return false;

  CLog::Log(LOGNOTICE, "starting upnp server");
  return CUPnP::GetInstance()->StartServer();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::IsUPnPServerRunning()
{
#ifdef HAS_UPNP
  return CUPnP::GetInstance()->IsInstantiated();
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StopUPnPServer()
{
#ifdef HAS_UPNP
  if (!IsUPnPRendererRunning())
    return true;

  CLog::Log(LOGNOTICE, "stopping upnp server");
  CUPnP::GetInstance()->StopServer();

  return true;
#endif // HAS_UPNP
  return false;
}
  
bool CNetworkServices::StartRss()
{
  if (IsRssRunning())
    return true;

  CRssManager::Get().Start();
  return true;
}

bool CNetworkServices::IsRssRunning()
{
  return CRssManager::Get().IsActive();
}

bool CNetworkServices::StopRss()
{
  if (!IsRssRunning())
    return true;

  CRssManager::Get().Stop();
  return true;
}

bool CNetworkServices::ValidatePort(int port)
{
  if (port <= 0 || port > 65535)
    return false;

#ifdef TARGET_LINUX
  if (!CUtil::CanBindPrivileged() && (port < 1024 || port > 65535))
    return false;
#endif

  return true;
}
