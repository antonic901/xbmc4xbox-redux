/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "settings/dialogs/GUIDialogSettingsManagerBase.h"

class CGUIDialogAddonSettings : public CGUIDialogSettingsManagerBase
{
public:
  CGUIDialogAddonSettings();
  virtual ~CGUIDialogAddonSettings() {}

  // specializations of CGUIControl
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  static bool ShowForAddon(const ADDON::AddonPtr& addon, bool saveToDisk = true);
  static void SaveAndClose();

  std::string GetCurrentAddonID() const;

protected:
  // implementation of CGUIDialogSettingsBase
  virtual void SetupView();
  virtual std::string GetLocalizedString(uint32_t labelId) const;
  virtual std::string GetSettingsLabel(const boost::shared_ptr<ISetting>& setting);
  virtual int GetSettingLevel() const;
  virtual boost::shared_ptr<CSettingSection> GetSection();

  // implementation of CGUIDialogSettingsManagerBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual bool Save() { return true; }
  virtual CSettingsManager* GetSettingsManager() const;

  // implementation of ISettingCallback
  virtual void OnSettingAction(const boost::shared_ptr<const CSetting>& setting);

private:
  static bool ShowForSingleInstance(const ADDON::AddonPtr& addon,
                                    bool saveToDisk,
                                    ADDON::AddonInstanceId instanceId = ADDON::ADDON_SETTINGS_ID);
  static bool ShowForMultipleInstances(const ADDON::AddonPtr& addon, bool saveToDisk);

  ADDON::AddonPtr m_addon;
  ADDON::AddonInstanceId m_instanceId{ADDON::ADDON_SETTINGS_ID};
  bool m_saveToDisk = false;
};
