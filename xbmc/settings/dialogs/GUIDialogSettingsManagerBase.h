/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/dialogs/GUIDialogSettingsBase.h"

class CSettingsManager;

class CGUIDialogSettingsManagerBase : public CGUIDialogSettingsBase
{
public:
  CGUIDialogSettingsManagerBase(int windowId, const std::string &xmlFile);
  virtual ~CGUIDialogSettingsManagerBase();

protected:
  virtual bool Save() = 0;
  virtual CSettingsManager* GetSettingsManager() const = 0;

  // implementation of CGUIDialogSettingsBase
  virtual boost::shared_ptr<CSetting> GetSetting(const std::string &settingId);
  virtual bool OnOkay();

  virtual std::set<std::string> CreateSettings();
  virtual void FreeSettingsControls();

  // implementation of ISettingControlCreator
  virtual boost::shared_ptr<ISettingControl> CreateControl(const std::string &controlType) const;
};
