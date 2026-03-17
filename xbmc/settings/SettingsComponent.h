/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system.h" // xtl.h

#include <boost/shared_ptr.hpp>

class CAdvancedSettings;
class CProfilesManager;
class CSettings;

class CSettingsComponent
{
public:
  CSettingsComponent();
  virtual ~CSettingsComponent();

  /*!
   * @brief Initialize all subcomponents with system default values (loaded from code, system settings files, ...).
   */
  void Initialize();

  /*!
   * @brief Initialize all subcomponents with user values (loaded from user settings files, according to active profile).
   * @return true on success, false otherwise.
   */
  bool Load();

  /*!
   * @brief Deinitialize all subcomponents.
   */
  void Deinitialize();

  /*!
   * @brief Get access to the settings subcomponent.
   * @return the settings subcomponent.
   */
  boost::shared_ptr<CSettings> GetSettings();

  /*!
   * @brief Get access to the advanced settings subcomponent.
   * @return the advanced settings subcomponent.
   */
  boost::shared_ptr<CAdvancedSettings> GetAdvancedSettings();

  /*!
   * @brief Get access to the profiles manager subcomponent.
   * @return the profiles manager subcomponent.
   */
  boost::shared_ptr<CProfilesManager> GetProfileManager();

private:
  bool InitDirectoriesXbox(bool bPlatformDirectories);
  void CreateUserDirs() const;

  enum State
  {
    DEINITED,
    INITED,
    LOADED
  };
  State m_state;

  boost::shared_ptr<CSettings> m_settings;
  boost::shared_ptr<CAdvancedSettings> m_advancedSettings;
  boost::shared_ptr<CProfilesManager> m_profileManager;
};
