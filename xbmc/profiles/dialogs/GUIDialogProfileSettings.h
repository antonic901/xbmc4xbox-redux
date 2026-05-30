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

#include <string>

class CGUIDialogProfileSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogProfileSettings();
  virtual ~CGUIDialogProfileSettings();

  static bool ShowForProfile(unsigned int iProfile, bool firstLogin = false);

protected:
  // specializations of CGUIWindow
  virtual void OnWindowLoaded();

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

  /*! \brief Prompt for a change in profile path
   \param directory Current directory for the profile, new profile directory will be returned here
   \param isDefault whether this is the default profile or not
   \return true if the profile path has been changed, false otherwise.
   */
  static bool GetProfilePath(std::string &directory, bool isDefault);

  void UpdateProfileImage();
  void updateProfileDirectory();

  bool m_needsSaving;
  std::string m_name;
  std::string m_thumb;
  std::string m_directory;
  int m_sourcesMode;
  int m_dbMode;
  bool m_isDefault;
  bool m_isNewUser;
  bool m_showDetails;

  CProfile::CLock m_locks;
};
