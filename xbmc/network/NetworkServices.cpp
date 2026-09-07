/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkServices.h"

#include "SectionLoader.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "network/EventServer.h"
#include "network/Network.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/RssManager.h"
#include "utils/Sntp.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <utility>

#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#endif // HAS_UPNP

#include "libFileZilla/XBFileZilla.h"

using namespace KODI::MESSAGING;
using namespace EVENTSERVER;
#ifdef HAS_UPNP
using namespace UPNP;
#endif // HAS_UPNP

CNetworkServices::CNetworkServices() :
    m_sntpclient(NULL),
    m_filezilla(NULL)
{
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_SERVICES_UPNP);
  settingSet.insert(CSettings::SETTING_SERVICES_UPNPSERVER);
  settingSet.insert(CSettings::SETTING_SERVICES_UPNPRENDERER);
  settingSet.insert(CSettings::SETTING_SERVICES_ESENABLED);
  settingSet.insert(CSettings::SETTING_SERVICES_ESPORT);
  settingSet.insert(CSettings::SETTING_SERVICES_ESALLINTERFACES);
  settingSet.insert(CSettings::SETTING_SERVICES_ESINITIALDELAY);
  settingSet.insert(CSettings::SETTING_SERVICES_ESCONTINUOUSDELAY);
  settingSet.insert(CSettings::SETTING_SERVICES_FTPSERVER);
  settingSet.insert(CSettings::SETTING_SERVICES_FTPSERVER_USER);
  settingSet.insert(CSettings::SETTING_SERVICES_FTPSERVER_PASSWORD);
  settingSet.insert(CSettings::SETTING_SERVICES_TIMESERVER);
  settingSet.insert(CSettings::SETTING_SERVICES_TIMESERVER_ADDRESS);
  settingSet.insert(CSettings::SETTING_SMB_WINSSERVER);
  settingSet.insert(CSettings::SETTING_SMB_WORKGROUP);
  m_settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_settings->GetSettingsManager()->RegisterCallback(this, settingSet);
}

CNetworkServices::~CNetworkServices()
{
  m_settings->GetSettingsManager()->UnregisterCallback(this);
  delete m_sntpclient;
  delete m_filezilla;
}

bool CNetworkServices::OnSettingChanging(const boost::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return false;

  const std::string &settingId = setting->GetId();

#ifdef HAS_UPNP
  if (settingId == CSettings::SETTING_SERVICES_UPNP)
  {
    if (boost::static_pointer_cast<const CSettingBool>(setting)->GetValue())
    {
      StartUPnPClient();
      StartUPnPServer();
      StartUPnPRenderer();
    }
    else
    {
      StopUPnPRenderer();
      StopUPnPServer();
      StopUPnPClient();
    }
  }
  else if (settingId == CSettings::SETTING_SERVICES_UPNPSERVER)
  {
    if (boost::static_pointer_cast<const CSettingBool>(setting)->GetValue())
    {
      if (!StartUPnPServer())
        return false;

      // always stop and restart the client and controller if necessary
      StopUPnPClient();
      StartUPnPClient();
    }
    else
      return StopUPnPServer();
  }
  else if (settingId == CSettings::SETTING_SERVICES_UPNPRENDERER)
  {
    if (boost::static_pointer_cast<const CSettingBool>(setting)->GetValue())
      return StartUPnPRenderer();
    else
      return StopUPnPRenderer();
  }
  else
#endif // HAS_UPNP

  if (settingId == CSettings::SETTING_SERVICES_ESENABLED)
  {
    if (boost::static_pointer_cast<const CSettingBool>(setting)->GetValue())
    {
      bool result = true;
      if (!StartEventServer())
      {
        HELPERS::ShowOKDialogText(33102, 33100);
        result = false;
      }

      return result;
    }
    else
    {
      bool result = true;
      result = StopEventServer(true, true);
      return result;
    }
  }
  else if (settingId == CSettings::SETTING_SERVICES_ESPORT)
  {
    // restart eventserver without asking user
    if (!StopEventServer(true, false))
      return false;

    if (!StartEventServer())
    {
      HELPERS::ShowOKDialogText(33102, 33100);
      return false;
    }
  }
  else if (settingId == CSettings::SETTING_SERVICES_ESALLINTERFACES)
  {
    if (m_settings->GetBool(CSettings::SETTING_SERVICES_ESALLINTERFACES) &&
        HELPERS::ShowYesNoDialogText(19098, 36633) != HELPERS::CHOICE_YES)
    {
      // Revert change, do not start server
      return false;
    }

    if (m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED))
    {
      if (!StopEventServer(true, true))
        return false;

      if (!StartEventServer())
      {
        HELPERS::ShowOKDialogText(33102, 33100);
        return false;
      }
    }
  }

  else if (settingId == CSettings::SETTING_SERVICES_ESINITIALDELAY ||
           settingId == CSettings::SETTING_SERVICES_ESCONTINUOUSDELAY)
  {
    if (m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED))
      return RefreshEventServer();
  }
  else if (settingId == CSettings::SETTING_SERVICES_TIMESERVER)
  {
    if (m_settings->GetBool(CSettings::SETTING_SERVICES_TIMESERVER))
      StartTimeServer();
    else
      StopTimeServer();
  }
  else if (settingId == CSettings::SETTING_SERVICES_TIMESERVER_ADDRESS)
  {
    StopTimeServer();
    StartTimeServer();
  }
  else if (settingId == CSettings::SETTING_SERVICES_FTPSERVER_USER ||
           settingId == CSettings::SETTING_SERVICES_FTPSERVER_PASSWORD)
  {
    return SetFTPServerUserPass();
  }
  else if (settingId == CSettings::SETTING_SERVICES_FTPSERVER)
  {
    if (m_settings->GetBool(CSettings::SETTING_SERVICES_FTPSERVER))
      StartFtpServer();
    else
      StopFtpServer();
  }

  return true;
}

void CNetworkServices::OnSettingChanged(const boost::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_SMB_WINSSERVER ||
      settingId == CSettings::SETTING_SMB_WORKGROUP)
  {
    // okey we really don't need to restart, only deinit samba, but that could be damn hard if something is playing
    //! @todo - General way of handling setting changes that require restart
    if (HELPERS::ShowYesNoDialogText(14038, 14039) ==
        HELPERS::CHOICE_YES)
    {
      m_settings->Save();
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_RESTARTAPP);
    }
  }
}

bool CNetworkServices::OnSettingUpdate(const boost::shared_ptr<CSetting>& setting,
                                       const char* oldSettingId,
                                       const TiXmlNode* oldSettingNode)
{
  if (setting == NULL)
    return false;

  return false;
}

void CNetworkServices::Start()
{
  if (m_settings->GetBool(CSettings::SETTING_SERVICES_UPNP))
    StartUPnP();
  if (m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED) && !StartEventServer())
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(33102), g_localizeStrings.Get(33100));

  StartRss();
  StartTimeServer();
  StartFtpServer();
}

void CNetworkServices::Stop(bool bWait)
{
  if (bWait)
  {
    StopUPnP(bWait);
    StopRss();
    StopTimeServer();
    StopFtpServer();
  }

  StopEventServer(bWait, false);
}

bool CNetworkServices::StartServer(enum ESERVERS server, bool start)
{
  boost::shared_ptr<CSettingsComponent> settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return false;

  boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (!settings)
    return false;

  bool ret = false;
  switch (server)
  {
    case ES_UPNPSERVER:
      // the callback will take care of starting/stopping upnp server
      ret = settings->SetBool(CSettings::SETTING_SERVICES_UPNPSERVER, start);
      break;

    case ES_UPNPRENDERER:
      // the callback will take care of starting/stopping upnp renderer
      ret = settings->SetBool(CSettings::SETTING_SERVICES_UPNPRENDERER, start);
      break;

    case ES_EVENTSERVER:
      // the callback will take care of starting/stopping event server
      ret = settings->SetBool(CSettings::SETTING_SERVICES_ESENABLED, start);
      break;

    case ES_TIMESERVER:
      // the callback will take care of starting/stopping time server
      ret = settings->SetBool(CSettings::SETTING_SERVICES_TIMESERVER, start);
      break;

    case ES_FTPSERVER:
      // the callback will take care of starting/stopping ftp server
      ret = settings->SetBool(CSettings::SETTING_SERVICES_FTPSERVER, start);
      break;

    default:
      ret = false;
      break;
  }
  settings->Save();

  return ret;
}

bool CNetworkServices::StartEventServer()
{
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED))
    return false;

  if (IsEventServerRunning())
    return true;

  CEventServer* server = CEventServer::GetInstance();
  if (!server)
  {
    CLog::Log(LOGERROR, "ES: Out of memory");
    return false;
  }

  server->StartServer();

  return true;
}

bool CNetworkServices::IsEventServerRunning()
{
  return CEventServer::GetInstance()->Running();
}

bool CNetworkServices::StopEventServer(bool bWait, bool promptuser)
{
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
      if (HELPERS::ShowYesNoDialogText(13140, 13141, "", "",
                                       10000) != HELPERS::CHOICE_YES)
      {
        CLog::Log(LOGINFO, "ES: Not stopping event server");
        return false;
      }
    }
    CLog::Log(LOGINFO, "ES: Stopping event server with confirmation");

    CEventServer::GetInstance()->StopServer(true);
  }
  else
  {
    if (!bWait)
      CLog::Log(LOGINFO, "ES: Stopping event server");

    CEventServer::GetInstance()->StopServer(bWait);
  }

  return true;
}

bool CNetworkServices::RefreshEventServer()
{
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_ESENABLED))
    return false;

  if (!IsEventServerRunning())
    return false;

  CEventServer::GetInstance()->RefreshSettings();
  return true;
}

bool CNetworkServices::StartUPnP()
{
  bool ret = false;
#ifdef HAS_UPNP
  ret |= StartUPnPClient();
  if (m_settings->GetBool(CSettings::SETTING_SERVICES_UPNPSERVER))
  {
   ret |= StartUPnPServer();
  }

  if (m_settings->GetBool(CSettings::SETTING_SERVICES_UPNPRENDERER))
  {
    ret |= StartUPnPRenderer();
  }
#endif // HAS_UPNP
  return ret;
}

bool CNetworkServices::StopUPnP(bool bWait)
{
#ifdef HAS_UPNP
  if (!CUPnP::IsInstantiated())
    return true;

  CLog::Log(LOGINFO, "stopping upnp");
  CUPnP::ReleaseInstance(bWait);

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPClient()
{
#ifdef HAS_UPNP
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_UPNP))
    return false;

  CLog::Log(LOGINFO, "starting upnp client");
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
  if (!IsUPnPClientRunning())
    return true;

  CLog::Log(LOGINFO, "stopping upnp client");
  CUPnP::GetInstance()->StopClient();

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPRenderer()
{
#ifdef HAS_UPNP
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_UPNPRENDERER) ||
      !m_settings->GetBool(CSettings::SETTING_SERVICES_UPNP))
    return false;

  CLog::Log(LOGINFO, "starting upnp renderer");
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

  CLog::Log(LOGINFO, "stopping upnp renderer");
  CUPnP::GetInstance()->StopRenderer();

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartUPnPServer()
{
#ifdef HAS_UPNP
  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_UPNPSERVER) ||
      !m_settings->GetBool(CSettings::SETTING_SERVICES_UPNP))
    return false;

  CLog::Log(LOGINFO, "starting upnp server");
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
  if (!IsUPnPServerRunning())
    return true;

  CLog::Log(LOGINFO, "stopping upnp server");
  CUPnP::GetInstance()->StopServer();

  return true;
#endif // HAS_UPNP
  return false;
}

bool CNetworkServices::StartRss()
{
  if (IsRssRunning())
    return true;

  CRssManager::GetInstance().Start();
  return true;
}

bool CNetworkServices::IsRssRunning()
{
  return CRssManager::GetInstance().IsActive();
}

bool CNetworkServices::StopRss()
{
  if (!IsRssRunning())
    return true;

  CRssManager::GetInstance().Stop();
  return true;
}

bool CNetworkServices::ValidatePort(int port)
{
  if (port <= 0 || port > 65535)
    return false;

  return true;
}

bool CNetworkServices::StartTimeServer()
{
  if (!CServiceBroker::GetNetwork().IsAvailable())
    return false;

  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_TIMESERVER))
    return false;

  if(m_sntpclient == NULL)
  {
    CSectionLoader::Load("SNTP");
    m_sntpclient = new CSNTPClient();
    m_sntpclient->Update();
  }

  return true;
}

bool CNetworkServices::StopTimeServer()
{
  if (m_sntpclient != NULL)
  {
    delete m_sntpclient;
    m_sntpclient = NULL;
    CSectionLoader::Unload("SNTP");
  }
  return true;
}

void CNetworkServices::UpdateTimeServer()
{
  if (m_sntpclient != NULL && m_sntpclient->UpdateNeeded())
  {
    m_sntpclient->Update();
  }
}

bool CNetworkServices::StartFtpServer()
{
  if (!CServiceBroker::GetNetwork().IsAvailable())
    return false;

  if (!m_settings->GetBool(CSettings::SETTING_SERVICES_FTPSERVER))
    return false;

  if (m_filezilla != NULL)
    return true;

  std::string xmlpath = "special://xbmc/system/";
  // if user didn't upgrade properly,
  // check whether UserData/FileZilla Server.xml exists
  if (XFILE::CFile::Exists(CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetUserDataItem("FileZilla Server.xml")))
    xmlpath = CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetUserDataFolder();

  XFILE::CFile file;
  if (file.Open(xmlpath + "FileZilla Server.xml") && file.GetLength() > 0)
  {
    m_filezilla = new CXBFileZilla(CSpecialProtocol::TranslatePath(xmlpath).c_str());
    m_filezilla->Start(false);
  }
  else
  {
    CLog::Log(LOGINFO, "XBFileZilla: 'FileZilla Server.xml' is missing or is corrupt!");
    CLog::Log(LOGINFO, "XBFileZilla: Starting ftp emergency recovery mode");
    StartFtpEmergencyRecoveryMode();
  }
  file.Close();
  return true;
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

bool CNetworkServices::StopFtpServer()
{
  if (m_filezilla != NULL)
  {
    m_filezilla->Stop();
    delete m_filezilla;
    m_filezilla = NULL;
  }
  return true;
}

bool CNetworkServices::SetFTPServerUserPass()
{
  if (m_filezilla == NULL)
    return false;

  // TODO: Read the FileZilla Server XML and Set it here!
  // Get GUI USER and pass and set pass to FTP Server
  std::string strFtpUserName = m_settings->GetString(CSettings::SETTING_SERVICES_FTPSERVER_USER);
  std::string strFtpUserPassword = m_settings->GetString(CSettings::SETTING_SERVICES_FTPSERVER_PASSWORD);
  if (strFtpUserPassword.empty())
  {
    CGUIDialogOK::ShowAndGetInput(728, 0, 12358, 0);
    return false;
  }

  std::vector<CXFUser*> v_ftpusers;
  m_filezilla->GetAllUsers(v_ftpusers);
  if (v_ftpusers.size() > 0)
  {
    for (size_t i = 0; i < v_ftpusers.size(); i++)
    {
      if (v_ftpusers[i]->GetName() == strFtpUserName)
      {
        if (v_ftpusers[i]->SetPassword(strFtpUserPassword.c_str()) != XFS_INVALID_PARAMETERS)
        {
          v_ftpusers[i]->CommitChanges();
          CGUIDialogOK::ShowAndGetInput(728, 0, 1247, 0);
          return true;
        }
        break;
      }
    }
  }
  return false;
}

bool CNetworkServices::FtpHasActiveConnections()
{
  if (m_filezilla == NULL)
    return false;

  return m_filezilla->GetNoConnections() != 0;
}
