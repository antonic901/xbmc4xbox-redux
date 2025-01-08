/*
*      Copyright (C) 2005-2014 Team XBMC
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

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <limits.h>

#include "GUIDialogLibExportSettings.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "dialogs/GUIDialogOK.h"
#include "ServiceBroker.h"
#include "settings/SettingUtils.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "settings/windows/GUIControlSettings.h"
#include "storage/MediaManager.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "filesystem/Directory.h"

using namespace ADDON;
using namespace KODI::MESSAGING;

CGUIDialogLibExportSettings::CGUIDialogLibExportSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_LIBEXPORT_SETTINGS, "DialogSettings.xml"),
  m_destinationChecked(false)
{ }

bool CGUIDialogLibExportSettings::Show(CLibExportSettings& settings)
{
  CGUIDialogLibExportSettings *dialog = static_cast<CGUIDialogLibExportSettings*>(g_windowManager.GetWindow(WINDOW_DIALOG_LIBEXPORT_SETTINGS));
  if (!dialog)
    return false;

  // Get current export settings from service broker
  dialog->m_settings.SetExportType(CSettings::GetInstance().GetInt("musiclibrary.exportfiletype"));
  dialog->m_settings.m_strPath = CSettings::GetInstance().GetString("musiclibrary.exportfolder");
  dialog->m_settings.SetItemsToExport(CSettings::GetInstance().GetInt("musiclibrary.exportitems"));
  dialog->m_settings.m_unscraped = CSettings::GetInstance().GetBool("musiclibrary.exportunscraped");
  dialog->m_settings.m_artwork = CSettings::GetInstance().GetBool("musiclibrary.exportartwork");
  dialog->m_settings.m_skipnfo = CSettings::GetInstance().GetBool("musiclibrary.exportskipnfo");
  dialog->m_settings.m_overwrite = CSettings::GetInstance().GetBool("musiclibrary.exportoverwrite");

  dialog->m_destinationChecked = false;
  dialog->Open();

  bool confirmed = dialog->IsConfirmed();
  if (confirmed)
  {
    // Return the new settings (saved by service broker but avoids re-reading)
    settings = dialog->m_settings;
  }
  return confirmed;
}

void CGUIDialogLibExportSettings::OnInitWindow()
{
  CGUIDialogSettingsManualBase::OnInitWindow();
}

void CGUIDialogLibExportSettings::OnSettingChanged(const CSetting *setting)
{
  if (!setting)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == "musiclibrary.exportfiletype")
  {
    m_settings.SetExportType(((CSettingInt*)setting)->GetValue());
    SetupView();
    SetFocus("musiclibrary.exportfiletype");
  }
  else if (settingId == "musiclibrary.exportfolder")
  {
    m_settings.m_strPath = ((CSettingString*)setting)->GetValue();
    UpdateButtons();
  }
  else if (settingId == "musiclibrary.exportoverwrite")
    m_settings.m_overwrite = ((CSettingBool*)setting)->GetValue();
  else if (settingId == "musiclibrary.exportitems")
    m_settings.SetItemsToExport(GetExportItemsFromSetting(setting));
  else if (settingId == "musiclibrary.exportartwork")
  {
    m_settings.m_artwork = ((CSettingBool*)setting)->GetValue();
    ToggleState("musiclibrary.exportskipnfo", m_settings.m_artwork);
  }
  else if (settingId == "musiclibrary.exportunscraped")
    m_settings.m_unscraped = ((CSettingBool*)setting)->GetValue();
  else if (settingId == "musiclibrary.exportskipnfo")
    m_settings.m_skipnfo = ((CSettingBool*)setting)->GetValue();
}

void CGUIDialogLibExportSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == "musiclibrary.exportfolder")
  {
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    g_mediaManager.GetNetworkLocations(shares);
#ifndef _XBOX
    g_mediaManager.GetRemovableDrives(shares);
#endif
    std::string strDirectory = m_settings.m_strPath;
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

    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(661), strDirectory, true))
    {
      if (!strDirectory.empty())
      {
        m_destinationChecked = true;
        m_settings.m_strPath = strDirectory;
        SetLabel2("musiclibrary.exportfolder", strDirectory);
        SetFocus("musiclibrary.exportfolder");
      }
    }
    UpdateButtons();
  }
}

bool CGUIDialogLibExportSettings::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_SETTINGS_OKAY_BUTTON)
      {
        OnOK();
        return true;
      }
    }
    break;
  }
  return CGUIDialogSettingsManualBase::OnMessage(message);
}

void CGUIDialogLibExportSettings::OnOK()
{
  // Validate destination folder
  if (m_settings.IsToLibFolders())
  {
    // Check artist info folder setting
    std::string path = CSettings::GetInstance().GetString("musiclibrary.artistsfolder");
    if (path.empty())
    {
      //"Unable to export to library folders as the system artist information folder setting is empty"
      //Settings (YES) button takes user to enter the artist info folder setting
      if (HELPERS::ShowYesNoDialogText(20223, 38317, 186, 10004) == HELPERS::YES)
      {
        m_confirmed = false;
        Close();
        g_windowManager.ActivateWindow(WINDOW_SETTINGS_MEDIA, "musiclibrary.artistsfolder");
      }
      return;
    }
  }
  else if (!m_destinationChecked)
  {
    // ELIBEXPORT_SINGLEFILE or LIBEXPORT_SEPARATEFILES
    // Check that destination folder exists
    if (!XFILE::CDirectory::Exists(m_settings.m_strPath))
    {
      CGUIDialogOK::ShowAndGetInput( 38300, 38318 );
      return;
    }
  }
  m_confirmed = true;
  Save();
  Close();
}

void CGUIDialogLibExportSettings::Save()
{
  CLog::Log(LOGINFO, "CGUIDialogMusicExportSettings: Save() called");
  CSettings::GetInstance().SetInt("musiclibrary.exportfiletype", m_settings.GetExportType());
  CSettings::GetInstance().SetString("musiclibrary.exportfolder", m_settings.m_strPath);
  CSettings::GetInstance().SetInt("musiclibrary.exportitems", m_settings.GetItemsToExport());
  CSettings::GetInstance().SetBool("musiclibrary.exportunscraped", m_settings.m_unscraped);
  CSettings::GetInstance().SetBool("musiclibrary.exportoverwrite", m_settings.m_overwrite);
  CSettings::GetInstance().SetBool("musiclibrary.exportartwork", m_settings.m_artwork);
  CSettings::GetInstance().SetBool("musiclibrary.exportskipnfo", m_settings.m_skipnfo);
  CSettings::GetInstance().Save();
}

void CGUIDialogLibExportSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetHeading(38300);

  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 38319);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);

  SetLabel2("musiclibrary.exportfolder", m_settings.m_strPath);

  if (m_settings.IsSingleFile())
  {
    ToggleState("musiclibrary.exportfolder", true);
    ToggleState("musiclibrary.exportoverwrite", false);
    ToggleState("musiclibrary.exportartwork", false);
    ToggleState("musiclibrary.exportskipnfo", false);
  }
  else if (m_settings.IsSeparateFiles())
  {
    ToggleState("musiclibrary.exportfolder", true);
    ToggleState("musiclibrary.exportoverwrite", true);
    ToggleState("musiclibrary.exportartwork", true);
    ToggleState("musiclibrary.exportskipnfo", m_settings.m_artwork);
  }
  else // To library folders
  {
    ToggleState("musiclibrary.exportfolder", false);
    ToggleState("musiclibrary.exportoverwrite", true);
    ToggleState("musiclibrary.exportartwork", true);
    ToggleState("musiclibrary.exportskipnfo", m_settings.m_artwork);
  }
  UpdateButtons();
}

void CGUIDialogLibExportSettings::UpdateButtons()
{
  // Enable Export button when destination folder has a path (but may not exist)
  bool enableExport(true);
  if (m_settings.IsSingleFile() ||
      m_settings.IsSeparateFiles())
    enableExport = !m_settings.m_strPath.empty();

  CONTROL_ENABLE_ON_CONDITION(CONTROL_SETTINGS_OKAY_BUTTON, enableExport);
  if (!enableExport)
    SetFocus("musiclibrary.exportfolder");
}

void CGUIDialogLibExportSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("exportsettings", -1);
  if (!category)
  {
    CLog::Log(LOGERROR, "CGUIDialogLibExportSettings: unable to setup settings");
    return;
  }

  CSettingGroup *groupDetails = AddGroup(category);
  if (!groupDetails)
  {
    CLog::Log(LOGERROR, "CGUIDialogLibExportSettings: unable to setup settings");
    return;
  }

  std::vector<std::pair<int, int> > entries;

  entries.push_back(std::make_pair(38301, ELIBEXPORT_SINGLEFILE));
  entries.push_back(std::make_pair(38302, ELIBEXPORT_SEPARATEFILES));
  entries.push_back(std::make_pair(38303, ELIBEXPORT_TOLIBRARYFOLDER));
  AddList(groupDetails, "musiclibrary.exportfiletype", 38304, 0, m_settings.GetExportType(), entries, 38304); // "Choose kind of export output"
  AddButton(groupDetails, "musiclibrary.exportfolder", 38305, 0);

  entries.clear();
  entries.push_back(std::make_pair(132, ELIBEXPORT_ALBUMS));  //ablums
  entries.push_back(std::make_pair(38043, ELIBEXPORT_ALBUMARTISTS)); //album artists
  entries.push_back(std::make_pair(38312, ELIBEXPORT_SONGARTISTS)); //song artists
  entries.push_back(std::make_pair(38313, ELIBEXPORT_OTHERARTISTS)); //other artists
  AddList(groupDetails, "musiclibrary.exportitems", 38306, 0, m_settings.GetExportItems(), entries, 133, 1);

  AddToggle(groupDetails, "musiclibrary.exportunscraped", 38308, 0, m_settings.m_unscraped);
  AddToggle(groupDetails, "musiclibrary.exportartwork", 38307, 0, m_settings.m_artwork);
  AddToggle(groupDetails, "musiclibrary.exportskipnfo", 38309, 0, m_settings.m_skipnfo);
  AddToggle(groupDetails, "musiclibrary.exportoverwrite", 38310, 0, m_settings.m_overwrite);
}

void CGUIDialogLibExportSettings::SetLabel2(const std::string &settingid, const std::string &label)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_LABEL2(settingControl->GetID(), label);
}


void CGUIDialogLibExportSettings::ToggleState(const std::string & settingid, bool enabled)
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

void CGUIDialogLibExportSettings::SetFocus(const std::string &settingid)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_FOCUS(settingControl->GetID(), 0);
}

int CGUIDialogLibExportSettings::GetExportItemsFromSetting(const CSetting *setting)
{
  const CSettingList *settingList = static_cast<const CSettingList*>(setting);
  if (settingList->GetElementType() != SettingTypeList)
  {
    CLog::Log(LOGERROR, "CGUIDialogLibExportSettings::%s - wrong items element type", __FUNCTION__);
    return 0;
  }
  int exportitems = 0;
  std::vector<CVariant> list = CSettingUtils::GetList(settingList);
  for (std::vector<CVariant>::const_iterator it = list.begin(); it != list.end(); ++it)
  {
    const CVariant &value = *it;
    if (!value.isInteger())
    {
      CLog::Log(LOGERROR, "CGUIDialogLibExportSettings::%s - wrong items value type", __FUNCTION__);
      return 0;
    }
    exportitems += value.asInteger();
  }
  return exportitems;
}
