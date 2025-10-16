/*
 *  Copyright (C) 2023-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

class IProgramLauncher
{
public:
  virtual ~IProgramLauncher() {}

  /*!
  \brief Launches the specified program.

  \param bLoadSettings specify if launcher should load custom settings
  \param bAllowRegionSwitching specify if launcher is allowed to force region
  \return Returns true if the program was successfully launched, false otherwise.
  */
  virtual bool Launch() = 0;

protected:
  /*!
  \brief Loads program settings stored in the database.

  Derived classes should implement this method if they support custom settings.
  */
  virtual bool LoadSettings() { return false; };

private:
  /*!
  \brief Checks if the given executable is supported.

  \return Returns true if the given executable is compatible with the launcher, false otherwise.
  */
  virtual bool IsSupported() = 0;
};
