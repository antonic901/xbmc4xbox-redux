/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonSystemSettings.h"

#include "ServiceBroker.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonInfo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

namespace ADDON
{

CAddonSystemSettings::CAddonSystemSettings()
{
  m_activeSettings[AddonType::RESOURCE_LANGUAGE] = CSettings::SETTING_LOCALE_LANGUAGE;
  m_activeSettings[AddonType::RESOURCE_UISOUNDS] = CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN;
  m_activeSettings[AddonType::SCRAPER_ALBUMS] = CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER;
  m_activeSettings[AddonType::SCRAPER_ARTISTS] = CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER;
  m_activeSettings[AddonType::SCRAPER_MOVIES] = CSettings::SETTING_SCRAPERS_MOVIESDEFAULT;
  m_activeSettings[AddonType::SCRAPER_MUSICVIDEOS] = CSettings::SETTING_SCRAPERS_MUSICVIDEOSDEFAULT;
  m_activeSettings[AddonType::SCRAPER_TVSHOWS] = CSettings::SETTING_SCRAPERS_TVSHOWSDEFAULT;
  m_activeSettings[AddonType::SCREENSAVER] = CSettings::SETTING_SCREENSAVER_MODE;
  m_activeSettings[AddonType::SCRIPT_WEATHER] = CSettings::SETTING_WEATHER_ADDON;
  m_activeSettings[AddonType::SKIN] = CSettings::SETTING_LOOKANDFEEL_SKIN;
  m_activeSettings[AddonType::WEB_INTERFACE] = CSettings::SETTING_SERVICES_WEBSKIN;
  m_activeSettings[AddonType::VISUALIZATION] = CSettings::SETTING_MUSICPLAYER_VISUALISATION;
}

CAddonSystemSettings& CAddonSystemSettings::GetInstance()
{
  static CAddonSystemSettings inst;
  return inst;
}

void CAddonSystemSettings::OnSettingAction(const boost::shared_ptr<const CSetting>& setting)
{
  if (setting->GetId() == CSettings::SETTING_ADDONS_MANAGE_DEPENDENCIES)
  {
    std::vector<std::string> params;
    params.push_back("addons://dependencies/");
    params.push_back("return");
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_ADDON_BROWSER, params);
  }
  else if (setting->GetId() == CSettings::SETTING_ADDONS_SHOW_RUNNING)
  {
    std::vector<std::string> params;
    params.push_back("addons://running/");
    params.push_back("return");
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_ADDON_BROWSER, params);
  }
  else if (setting->GetId() == CSettings::SETTING_ADDONS_REMOVE_ORPHANED_DEPENDENCIES)
  {
    using namespace KODI::MESSAGING::HELPERS;

    const std::vector<std::string> removedItems = CAddonInstaller::GetInstance().RemoveOrphanedDepsRecursively();
    if (removedItems.size() > 0)
    {
      const std::string message =
          StringUtils::Format(g_localizeStrings.Get(36641).c_str(), StringUtils::Join(removedItems, ", ").c_str());

      ShowOKDialogText(36640, message); // "following orphaned were removed..."
    }
    else
    {
      ShowOKDialogText(36640, 36642); // "no orphaned found / removed"
    }
  }
}

void CAddonSystemSettings::OnSettingChanged(const boost::shared_ptr<const CSetting>& setting)
{
  using namespace KODI::MESSAGING::HELPERS;

  if (setting->GetId() == CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES &&
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES) &&
      ShowYesNoDialogText(19098, 36618) != KODI::MESSAGING::HELPERS::YES)
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES, false);
  }
}

bool CAddonSystemSettings::GetActive(AddonType::Type type, AddonPtr& addon)
{
  std::map<ADDON::AddonType::Type, std::string>::const_iterator it = m_activeSettings.find(type);
  if (it != m_activeSettings.end())
  {
    std::string settingValue = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(it->second);
    return CServiceBroker::GetAddonMgr().GetAddon(settingValue, addon, type,
                                                  OnlyEnabled::CHOICE_YES);
  }
  return false;
}

bool CAddonSystemSettings::SetActive(AddonType::Type type, const std::string& addonID)
{
  std::map<ADDON::AddonType::Type, std::string>::const_iterator it = m_activeSettings.find(type);
  if (it != m_activeSettings.end())
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(it->second, addonID);
    return true;
  }
  return false;
}

bool CAddonSystemSettings::IsActive(const IAddon& addon)
{
  AddonPtr active;
  return GetActive(addon.Type(), active) && active->ID() == addon.ID();
}

bool CAddonSystemSettings::UnsetActive(const AddonInfoPtr& addon)
{
  std::map<ADDON::AddonType::Type, std::string>::const_iterator it = m_activeSettings.find(addon->MainType());
  if (it == m_activeSettings.end())
    return true;

  boost::shared_ptr<CSettingString> setting = boost::static_pointer_cast<CSettingString>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(it->second));
  if (setting->GetValue() != addon->ID())
    return true;

  if (setting->GetDefault() == addon->ID())
    return false; // Cant unset defaults

  setting->Reset();
  return true;
}

int CAddonSystemSettings::GetAddonAutoUpdateMode() const
{
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_ADDONS_AUTOUPDATES);
}

AddonRepoUpdateMode::Type CAddonSystemSettings::GetAddonRepoUpdateMode() const
{
  const int updateMode = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_ADDONS_UPDATEMODE);
  return static_cast<AddonRepoUpdateMode::Type>(updateMode);
}
}
