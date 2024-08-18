/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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

#include "addons/AddonManager.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonSystemSettings.h"
#include "ServiceBroker.h"
#include "addons/RepositoryUpdater.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"
#include "utils/log.h"


namespace ADDON
{

std::map<ADDON::TYPE, std::string> CreateActiveSettings() {
  std::map<ADDON::TYPE, std::string> settings;
  settings[ADDON::ADDON_VIZ] = "musicplayer.visualisation";
  settings[ADDON::ADDON_SCREENSAVER] = "screensaver.mode";
  settings[ADDON::ADDON_SCRAPER_ALBUMS] = "musiclibrary.albumsscraper";
  settings[ADDON::ADDON_SCRAPER_ARTISTS] = "musiclibrary.artistsscraper";
  settings[ADDON::ADDON_SCRAPER_MOVIES] = "scrapers.moviesdefault";
  settings[ADDON::ADDON_SCRAPER_MUSICVIDEOS] = "scrapers.musicvideosdefault";
  settings[ADDON::ADDON_SCRAPER_TVSHOWS] = "scrapers.tvshowsdefault";
  settings[ADDON::ADDON_WEB_INTERFACE] = "services.webskin";
  settings[ADDON::ADDON_RESOURCE_LANGUAGE] = "locale.language";
  settings[ADDON::ADDON_SCRIPT_WEATHER] = "weather.addon";
  settings[ADDON::ADDON_SKIN] = "lookandfeel.skin";
  settings[ADDON::ADDON_RESOURCE_UISOUNDS] = "lookandfeel.soundskin";
  return settings;
}

CAddonSystemSettings::CAddonSystemSettings() :
  m_activeSettings(CreateActiveSettings())
{}

CAddonSystemSettings& CAddonSystemSettings::GetInstance()
{
  static CAddonSystemSettings inst;
  return inst;
}

void CAddonSystemSettings::OnSettingAction(const CSetting* setting)
{
  if (setting->GetId() == "addons.managedependencies")
  {
    std::vector<std::string> params;
    params.push_back("addons://dependencies/");
    params.push_back("return");
    g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
  }
  else if (setting->GetId() == "addons.showrunning")
  {
    std::vector<std::string> params;
    params.push_back("addons://running/");
    params.push_back("return");
    g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
  }
}

void CAddonSystemSettings::OnSettingChanged(const CSetting* setting)
{
  using namespace KODI::MESSAGING::HELPERS;

  if (setting->GetId() == "addons.unknownsources"
    && CSettings::GetInstance().GetBool("addons.unknownsources")
    && ShowYesNoDialogText(19098, 36618) != YES)
  {
    CSettings::GetInstance().SetBool("addons.unknownsources", false);
  }
}

bool CAddonSystemSettings::GetActive(const TYPE& type, AddonPtr& addon)
{
  std::map<ADDON::TYPE, std::string>::const_iterator it = m_activeSettings.find(type);
  if (it != m_activeSettings.end())
  {
    std::string settingValue = CSettings::GetInstance().GetString(it->second);
    return CServiceBroker::GetAddonMgr().GetAddon(settingValue, addon, type);
  }
  return false;
}

bool CAddonSystemSettings::SetActive(const TYPE& type, const std::string& addonID)
{
  std::map<ADDON::TYPE, std::string>::const_iterator it = m_activeSettings.find(type);
  if (it != m_activeSettings.end())
  {
    CSettings::GetInstance().SetString(it->second, addonID);
    return true;
  }
  return false;
}

bool CAddonSystemSettings::IsActive(const IAddon& addon)
{
  AddonPtr active;
  return GetActive(addon.Type(), active) && active->ID() == addon.ID();
}

bool CAddonSystemSettings::UnsetActive(const AddonPtr& addon)
{
  std::map<ADDON::TYPE, std::string>::const_iterator it = m_activeSettings.find(addon->Type());
  if (it == m_activeSettings.end())
    return true;

  CSettingString *setting = static_cast<CSettingString*>(CSettings::GetInstance().GetSetting(it->second));
  if (setting->GetValue() != addon->ID())
    return true;

  if (setting->GetDefault() == addon->ID())
    return false; // Cant unset defaults

  setting->Reset();
  return true;
}

bool IsCompatible(const AddonPtr a)
{
  return CServiceBroker::GetAddonMgr().IsCompatible(*a);
}

ADDON::VECADDONS getIncompatible()
{
  VECADDONS incompatible;
  CServiceBroker::GetAddonMgr().GetAddons(incompatible);
  std::vector<AddonPtr>::iterator end_it = boost::range::remove_if(incompatible, IsCompatible);
  incompatible.erase(end_it, incompatible.end());
  return incompatible;
}

std::vector<std::string> CAddonSystemSettings::MigrateAddons(boost::function<void(void)> onMigrate)
{
  if (getIncompatible().empty())
    return std::vector<std::string>();

  if (CSettings::GetInstance().GetInt("general.addonupdates") == AUTO_UPDATES_ON)
  {
    onMigrate();

    if (CRepositoryUpdater::GetInstance().CheckForUpdates())
      CRepositoryUpdater::GetInstance().Await();

    CLog::Log(LOGINFO, "ADDON: waiting for add-ons to update...");
    CAddonInstaller::GetInstance().InstallUpdatesAndWait();
  }

  VECADDONS incompatible = getIncompatible();
  for (VECADDONS::const_iterator it = incompatible.begin(); it != incompatible.end(); ++it)
    CLog::Log(LOGNOTICE, "ADDON: %s version %s is incompatible", (*it)->ID().c_str(), (*it)->Version().asString().c_str());

  std::vector<std::string> changed;
  for (VECADDONS::const_iterator it = incompatible.begin(); it != incompatible.end(); ++it)
  {
    const ADDON::AddonPtr &addon = *it;
    if (!UnsetActive(addon))
    {
      CLog::Log(LOGWARNING, "ADDON: failed to unset %s", addon->ID().c_str());
      continue;
    }
    if (!CServiceBroker::GetAddonMgr().DisableAddon(addon->ID()))
    {
      CLog::Log(LOGWARNING, "ADDON: failed to disable %s", addon->ID().c_str());
    }
    changed.push_back(addon->Name());
  }

  return changed;
}
}
