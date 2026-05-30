/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "profiles/Profile.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CGUIDialogLockSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogLockSettings();
  virtual ~CGUIDialogLockSettings();

  static bool ShowAndGetLock(LockType &lockMode, std::string &password, int header = 20091);
  static bool ShowAndGetLock(CProfile::CLock &locks, int buttonLabel = 20091, bool conditional = false, bool details = true);
  static bool ShowAndGetUserAndPassword(std::string &user, std::string &password, const std::string &url, bool *saveUserDetails);

protected:
  // implementations of ISettingCallback
  virtual void OnSettingChanged(const boost::shared_ptr<const CSetting>& setting);
  virtual void OnSettingAction(const boost::shared_ptr<const CSetting>& setting);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual bool Save() { return true; }
  virtual void OnCancel();
  virtual void SetupView();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

private:
  std::string GetLockModeLabel();
  void SetDetailSettingsEnabled(bool enabled);
  void SetSettingLockCodeLabel();

  bool m_changed;

  CProfile::CLock m_locks;
  std::string m_user;
  std::string m_url;
  bool m_details;
  bool m_conditionalDetails;
  bool m_getUser;
  bool* m_saveUserDetails;
  int m_buttonLabel;
};
