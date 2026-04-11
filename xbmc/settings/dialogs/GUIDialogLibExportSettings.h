/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/LibExportSettings.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

#include <map>

class CGUIDialogLibExportSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogLibExportSettings();

  // specialization of CGUIWindow
  virtual bool HasListItems() const { return true; }
  static bool Show(CLibExportSettings& settings);

protected:
  // specializations of CGUIWindow
  virtual void OnInitWindow();

  // implementations of ISettingCallback
  virtual void OnSettingChanged(const boost::shared_ptr<const CSetting>& setting);
  virtual void OnSettingAction(const boost::shared_ptr<const CSetting>& setting);

  // specialization of CGUIDialogSettingsBase
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool AllowResettingSettings() const { return false; }
  virtual bool Save();
  virtual void SetupView();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

  void OnOK();
  void UpdateButtons();

private:
  void SetLabel2(const std::string &settingid, const std::string &label);
  void SetLabel(const std::string &settingid, const std::string &label);
  void ToggleState(const std::string &settingid, bool enabled);

  using CGUIDialogSettingsManualBase::SetFocus;
  void SetFocus(const std::string &settingid);
  static int GetExportItemsFromSetting(const SettingConstPtr& setting);
  void UpdateToggles();
  void UpdateDescription();

  CLibExportSettings m_settings;
  bool m_destinationChecked = false;
  boost::shared_ptr<CSettingBool> m_settingNFO;
  boost::shared_ptr<CSettingBool> m_settingArt;
};
