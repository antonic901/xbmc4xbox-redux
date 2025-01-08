/*
 *      Copyright (C) 2017 Team XBMC
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

#include "GUIDialogInfoProviderSettings.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <limits.h>

#include "addons/AddonSystemSettings.h"
#include "addons/GUIDialogAddonSettings.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "filesystem/AddonsDirectory.h"
#include "filesystem/Directory.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/builtins/Builtins.h"
#include "ServiceBroker.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/windows/GUIControlSettings.h"
#include "storage/MediaManager.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace ADDON;

const std::string SETTING_ALBUMSCRAPER_SETTINGS = "albumscrapersettings";
const std::string SETTING_ARTISTSCRAPER_SETTINGS = "artistscrapersettings";
const std::string SETTING_APPLYTOITEMS = "applysettingstoitems";

CGUIDialogInfoProviderSettings::CGUIDialogInfoProviderSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_INFOPROVIDER_SETTINGS, "DialogSettings.xml"),
    m_showSingleScraper(false),
    m_singleScraperType(CONTENT_NONE),
    m_applyToItems(INFOPROVIDER_THISITEM)
{ }

bool CGUIDialogInfoProviderSettings::Show()
{
  CGUIDialogInfoProviderSettings *dialog = static_cast<CGUIDialogInfoProviderSettings*>(g_windowManager.GetWindow(WINDOW_DIALOG_INFOPROVIDER_SETTINGS));
  if (!dialog)
    return false;

  dialog->m_showSingleScraper = false;

  // Get current default info provider settings from service broker
  dialog->m_fetchInfo = CSettings::GetInstance().GetBool("musiclibrary.downloadinfo");

  ADDON::AddonPtr defaultScraper;
  // Get default album scraper (when enabled - can default scraper be disabled??)
  if (ADDON::CAddonSystemSettings::GetInstance().GetActive(ADDON::ADDON_SCRAPER_ALBUMS, defaultScraper))
  {
    ADDON::ScraperPtr scraper = boost::dynamic_pointer_cast<ADDON::CScraper>(defaultScraper);
    dialog->SetAlbumScraper(scraper);
  }

  // Get default artist scraper
  if (ADDON::CAddonSystemSettings::GetInstance().GetActive(ADDON::ADDON_SCRAPER_ARTISTS, defaultScraper))
  {
    ADDON::ScraperPtr scraper = boost::dynamic_pointer_cast<ADDON::CScraper>(defaultScraper);
    dialog->SetArtistScraper(scraper);
  }

  dialog->m_strArtistInfoPath = CSettings::GetInstance().GetString("musiclibrary.artistsfolder");

  dialog->Open();

  dialog->ResetDefaults();
  return dialog->IsConfirmed();
}

int CGUIDialogInfoProviderSettings::Show(ADDON::ScraperPtr& scraper)
{
  CGUIDialogInfoProviderSettings *dialog = static_cast<CGUIDialogInfoProviderSettings*>(g_windowManager.GetWindow(WINDOW_DIALOG_INFOPROVIDER_SETTINGS));
  if (!dialog || !scraper)
    return -1;
  if (scraper->Content() != CONTENT_ARTISTS && scraper->Content() != CONTENT_ALBUMS)
    return -1;

  dialog->m_showSingleScraper = true;
  dialog->m_singleScraperType = scraper->Content();

  if (dialog->m_singleScraperType == CONTENT_ALBUMS)
    dialog->SetAlbumScraper(scraper);
  else
    dialog->SetArtistScraper(scraper);
  // toast selected but disabled scrapers
  if (CServiceBroker::GetAddonMgr().IsAddonDisabled(scraper->ID()))
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(24024), scraper->Name(), 2000, true);

  dialog->Open();

  bool confirmed = dialog->IsConfirmed();
  unsigned int applyToItems = dialog->m_applyToItems;
  if (confirmed)
  {
    if (dialog->m_singleScraperType == CONTENT_ALBUMS)
      scraper = dialog->GetAlbumScraper();
    else
    {
      scraper = dialog->GetArtistScraper();
      // Save artist information folder (here not in the caller) when applying setting as default for all artists
      if (applyToItems == INFOPROVIDER_DEFAULT)
        CSettings::GetInstance().SetString("musiclibrary.artistsfolder", dialog->m_strArtistInfoPath);
    }
    if (scraper)
      scraper->SetPathSettings(dialog->m_singleScraperType, "");
  }

  dialog->ResetDefaults();

  if (confirmed)
    return applyToItems;
  else
    return -1;
}

void CGUIDialogInfoProviderSettings::OnInitWindow()
{
  CGUIDialogSettingsManualBase::OnInitWindow();
}

void CGUIDialogInfoProviderSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == nullptr)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == "musiclibrary.downloadinfo")
  {
    m_fetchInfo = ((const CSettingBool*)setting)->GetValue();
    SetupView();
    SetFocus(std::string("musiclibrary.downloadinfo"));
  }
  else if (settingId == "musiclibrary.artistsfolder")
    m_strArtistInfoPath = ((const CSettingString*)setting)->GetValue();
  else if (settingId == SETTING_APPLYTOITEMS)
  {
    m_applyToItems = ((const CSettingInt*)setting)->GetValue();
    SetupView();
    SetFocus(SETTING_APPLYTOITEMS);
  }
}

void CGUIDialogInfoProviderSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == nullptr)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == "musiclibrary.albumsscraper")
  {
    std::string currentScraperId;
    if (m_albumscraper)
      currentScraperId = m_albumscraper->ID();
    std::string selectedAddonId = currentScraperId;

    if (CGUIWindowAddonBrowser::SelectAddonID(ADDON_SCRAPER_ALBUMS, selectedAddonId, false) == 1
        && selectedAddonId != currentScraperId)
    {
      AddonPtr scraperAddon;
      CServiceBroker::GetAddonMgr().GetAddon(selectedAddonId, scraperAddon);
      m_albumscraper = boost::dynamic_pointer_cast<CScraper>(scraperAddon);
      SetupView();
      SetFocus(settingId);
    }
  }
  else if (settingId == "musiclibrary.artistsscraper")
  {
    std::string currentScraperId;
    if (m_artistscraper)
      currentScraperId = m_artistscraper->ID();
    std::string selectedAddonId = currentScraperId;

    if (CGUIWindowAddonBrowser::SelectAddonID(ADDON_SCRAPER_ARTISTS, selectedAddonId, false) == 1
        && selectedAddonId != currentScraperId)
    {
      AddonPtr scraperAddon;
      CServiceBroker::GetAddonMgr().GetAddon(selectedAddonId, scraperAddon);
      m_artistscraper = boost::dynamic_pointer_cast<CScraper>(scraperAddon);
      SetupView();
      SetFocus(settingId);
    }
  }
  else if (settingId == SETTING_ALBUMSCRAPER_SETTINGS)
    CGUIDialogAddonSettings::ShowAndGetInput(m_albumscraper, false);
  else if (settingId == SETTING_ARTISTSCRAPER_SETTINGS)
    CGUIDialogAddonSettings::ShowAndGetInput(m_artistscraper, false);
  else if (settingId == "musiclibrary.artistsfolder")
  {
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    g_mediaManager.GetNetworkLocations(shares);
#ifndef _XBOX
    g_mediaManager.GetRemovableDrives(shares);
#endif
    std::string strDirectory = m_strArtistInfoPath;
    if (!strDirectory.empty())
    {
      URIUtils::AddSlashAtEnd(strDirectory);
      bool bIsSource;
      if (CUtil::GetMatchingSource(strDirectory, shares, bIsSource) < 0) // path is outside shares - add it as a separate one
      {
        CMediaSource share;
        share.strName = g_localizeStrings.Get(13278);
        share.strPath = strDirectory;
        shares.push_back(share);
      }
    }
    else
      strDirectory = "default location";

    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(20223), strDirectory, true))
    {
      if (!strDirectory.empty())
      {
        m_strArtistInfoPath = strDirectory;
        SetLabel2("musiclibrary.artistsfolder", strDirectory);
        SetFocus(std::string("musiclibrary.artistsfolder"));
      }
    }
  }
}

void CGUIDialogInfoProviderSettings::Save()
{
  if (m_showSingleScraper)
    return;  //Save done by caller of ::Show

  // Save default settings for fetching additional information and art
  CLog::Log(LOGINFO, "%s called", __FUNCTION__);
  // Save Fetch addiitional info during update
  CSettings::GetInstance().SetBool("musiclibrary.downloadinfo", m_fetchInfo);
  // Save default scrapers and addon setting values
  CSettings::GetInstance().SetString("musiclibrary.albumsscraper", m_albumscraper->ID());
  m_albumscraper->SaveSettings();
  CSettings::GetInstance().SetString("musiclibrary.artistsscraper", m_artistscraper->ID());
  m_artistscraper->SaveSettings();
  // Save artist information folder
  CSettings::GetInstance().SetString("musiclibrary.artistsfolder", m_strArtistInfoPath);
  CSettings::GetInstance().Save();
}

void CGUIDialogInfoProviderSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);

  SetLabel2("musiclibrary.artistsfolder", m_strArtistInfoPath);

  if (!m_showSingleScraper)
  {
    SetHeading(38330);
    if (!m_fetchInfo)
    {
      ToggleState("musiclibrary.albumsscraper", false);
      ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, false);
      ToggleState("musiclibrary.artistsscraper", false);
      ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, false);
      ToggleState("musiclibrary.artistsfolder", false);
    }
    else
    {  // Album scraper
      ToggleState("musiclibrary.albumsscraper", true);
      if (m_albumscraper && !CServiceBroker::GetAddonMgr().IsAddonDisabled(m_albumscraper->ID()))
      {
        SetLabel2("musiclibrary.albumsscraper", m_albumscraper->Name());
        if (m_albumscraper && m_albumscraper->HasSettings())
          ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, true);
        else
          ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, false);
      }
      else
      {
        SetLabel2("musiclibrary.albumsscraper", g_localizeStrings.Get(231)); //Set label2 to "None"
        ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, false);
      }
      // Artist scraper
      ToggleState("musiclibrary.artistsscraper", true);
      if (m_artistscraper && !CServiceBroker::GetAddonMgr().IsAddonDisabled(m_artistscraper->ID()))
      {
        SetLabel2("musiclibrary.artistsscraper", m_artistscraper->Name());
        if (m_artistscraper && m_artistscraper->HasSettings())
          ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, true);
        else
          ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, false);
      }
      else
      {
        SetLabel2("musiclibrary.artistsscraper", g_localizeStrings.Get(231)); //Set label2 to "None"
        ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, false);
      }
      // Artist Information Folder
      ToggleState("musiclibrary.artistsfolder", true);
    }
  }
  else if (m_singleScraperType == CONTENT_ALBUMS)
  {
    SetHeading(38331);
    // Album scraper
    ToggleState("musiclibrary.albumsscraper", true);
    if (m_albumscraper && !CServiceBroker::GetAddonMgr().IsAddonDisabled(m_albumscraper->ID()))
    {
      SetLabel2("musiclibrary.albumsscraper", m_albumscraper->Name());
      if (m_albumscraper && m_albumscraper->HasSettings())
        ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, true);
      else
        ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, false);
    }
    else
    {
      SetLabel2("musiclibrary.albumsscraper", g_localizeStrings.Get(231)); //Set label2 to "None"
      ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, false);
    }
  }
  else
  {
    SetHeading(38332);
    // Artist scraper
    ToggleState("musiclibrary.artistsscraper", true);
    if (m_artistscraper && !CServiceBroker::GetAddonMgr().IsAddonDisabled(m_artistscraper->ID()))
    {
      SetLabel2("musiclibrary.artistsscraper", m_artistscraper->Name());
      if (m_artistscraper && m_artistscraper->HasSettings())
        ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, true);
      else
        ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, false);
    }
    else
    {
      SetLabel2("musiclibrary.artistsscraper", g_localizeStrings.Get(231)); //Set label2 to "None"
      ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, false);
    }
    // Artist Information Folder when default settings
    ToggleState("musiclibrary.artistsfolder", m_applyToItems == INFOPROVIDER_DEFAULT);
  }
}

void CGUIDialogInfoProviderSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("infoprovidersettings", -1);
  if (category == nullptr)
  {
    CLog::Log(LOGERROR, "%s: unable to setup settings", __FUNCTION__);
    return;
  }
  CSettingGroup *group1 = AddGroup(category);
  if (group1 == nullptr)
  {
    CLog::Log(LOGERROR, "%s: unable to setup settings", __FUNCTION__);
    return;
  }

  if (!m_showSingleScraper)
  {
    AddToggle(group1, "musiclibrary.downloadinfo", 38333, 0, m_fetchInfo); // "Fetch additional information during scan"
  }
  else
  {
    std::vector<std::pair<int, int> > entries;
    entries.clear();
    if (m_singleScraperType == CONTENT_ALBUMS)
    {
      entries.push_back(std::make_pair(38066, INFOPROVIDER_THISITEM));
      entries.push_back(std::make_pair(38067, INFOPROVIDER_ALLVIEW));
    }
    else
    {
      entries.push_back(std::make_pair(38064, INFOPROVIDER_THISITEM));
      entries.push_back(std::make_pair(38065, INFOPROVIDER_ALLVIEW));
    }
    entries.push_back(std::make_pair(38063, INFOPROVIDER_DEFAULT));
    AddList(group1, SETTING_APPLYTOITEMS, 38338, 0, m_applyToItems, entries, 38339); // "Apply settings to"
  }

  CSettingGroup *group = AddGroup(category, 38337);
  if (group == nullptr)
  {
    CLog::Log(LOGERROR, "%s: unable to setup settings", __FUNCTION__);
    return;
  }
  CSettingAction *subsetting;
  if (!m_showSingleScraper || m_singleScraperType == CONTENT_ALBUMS)
  {
    AddButton(group, "musiclibrary.albumsscraper", 38334, 0); //Provider for album information
    subsetting = AddButton(group, SETTING_ALBUMSCRAPER_SETTINGS, 10004, 0); //"settings"
    if (subsetting)
      subsetting->SetParent("musiclibrary.albumsscraper");
  }
  if (!m_showSingleScraper || m_singleScraperType == CONTENT_ARTISTS)
  {
    AddButton(group, "musiclibrary.artistsscraper", 38335, 0); //Provider for artist information
    subsetting = AddButton(group, SETTING_ARTISTSCRAPER_SETTINGS, 10004, 0); //"settings"
    if (subsetting)
      subsetting->SetParent("musiclibrary.artistsscraper");

    AddButton(group, "musiclibrary.artistsfolder", 38336, 0);
  }
}

void CGUIDialogInfoProviderSettings::SetLabel2(const std::string &settingid, const std::string &label)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_LABEL2(settingControl->GetID(), label);
}

void CGUIDialogInfoProviderSettings::ToggleState(const std::string &settingid, bool enabled)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
  {
    if (enabled)
      CONTROL_ENABLE(settingControl->GetID());
    else
      CONTROL_DISABLE(settingControl->GetID());
  }
}

void CGUIDialogInfoProviderSettings::SetFocus(const std::string &settingid)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_FOCUS(settingControl->GetID(), 0);
}

void CGUIDialogInfoProviderSettings::ResetDefaults()
{
  m_showSingleScraper = false;
  m_singleScraperType = CONTENT_NONE;
  m_applyToItems = INFOPROVIDER_THISITEM;
}
