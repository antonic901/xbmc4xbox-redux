/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsComponent.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

CSettingsComponent::CSettingsComponent()
  : m_settings(new CSettings()),
    m_advancedSettings(new CAdvancedSettings()),
    m_profileManager(new CProfilesManager())
{
  m_state = State::DEINITED;
}

CSettingsComponent::~CSettingsComponent()
{
}

void CSettingsComponent::Initialize()
{
  if (m_state == State::DEINITED)
  {
    InitDirectoriesXbox(true);

    m_settings->Initialize();

    m_advancedSettings->Initialize(*m_settings->GetSettingsManager());

    m_profileManager->Initialize(m_settings);

    m_state = State::INITED;
  }
}

bool CSettingsComponent::Load()
{
  if (m_state == State::INITED)
  {
    if (!m_profileManager->Load())
    {
      CLog::Log(LOGFATAL, "unable to load profile");
      return false;
    }

    if (!m_settings->Load())
    {
      CLog::Log(LOGFATAL, "unable to load settings");
      return false;
    }

    m_settings->SetLoaded();

    m_state = State::LOADED;
    return true;
  }
  else if (m_state == State::LOADED)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void CSettingsComponent::Deinitialize()
{
  if (m_state >= State::INITED)
  {
    if (m_state == State::LOADED)
    {
      m_settings->Unload();
    }
    m_profileManager->Uninitialize();

    m_advancedSettings->Uninitialize(*m_settings->GetSettingsManager());

    m_settings->Uninitialize();
  }
  m_state = State::DEINITED;
}

boost::shared_ptr<CSettings> CSettingsComponent::GetSettings()
{
  return m_settings;
}

boost::shared_ptr<CAdvancedSettings> CSettingsComponent::GetAdvancedSettings()
{
  return m_advancedSettings;
}

boost::shared_ptr<CProfilesManager> CSettingsComponent::GetProfileManager()
{
  return m_profileManager;
}

bool CSettingsComponent::InitDirectoriesXbox(bool bPlatformDirectories)
{
  std::string xbmcPath = "Q:\\";
  CSpecialProtocol::SetXBMCBinPath(xbmcPath);
  CSpecialProtocol::SetXBMCPath(xbmcPath);
  CSpecialProtocol::SetXBMCBinAddonPath(xbmcPath + "addons");

  std::string strWin32UserFolder = "E:\\UDATA\\XBMC\\";
  CSpecialProtocol::SetLogPath(strWin32UserFolder);
  CSpecialProtocol::SetHomePath(strWin32UserFolder);
  CSpecialProtocol::SetMasterProfilePath(URIUtils::AddFileToFolder(strWin32UserFolder, "userdata"));
  CSpecialProtocol::SetTempPath("Z:\\");

  CreateUserDirs();

  return true;
}

void CSettingsComponent::CreateUserDirs() const
{
  // We need to clear temp directory on Xbox (Z:\)
  std::string strPath = CSpecialProtocol::TranslatePath("special://temp/");
  if (!XFILE::CDirectory::RemoveRecursive(strPath))
    CLog::Log(LOGWARNING, "Failed to remove the temp cache at %s", strPath.c_str());

  XFILE::CDirectory::Create("special://home/");
  XFILE::CDirectory::Create("special://home/addons");
  XFILE::CDirectory::Create("special://home/addons/packages");
  XFILE::CDirectory::Create("special://home/addons/temp");
  XFILE::CDirectory::Create("special://home/media");
  XFILE::CDirectory::Create("special://home/system");
  XFILE::CDirectory::Create("special://masterprofile/");
  XFILE::CDirectory::Create("special://temp/");
  XFILE::CDirectory::Create("special://logpath");
  XFILE::CDirectory::Create("special://temp/temp"); // temp directory for python and dllGetTempPathA
  XFILE::CDirectory::Create(CSpecialProtocol::TranslatePath("special://temp/archive_cache/"));

}
