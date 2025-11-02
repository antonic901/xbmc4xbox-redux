/*
 *  Copyright (C) 2023-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "IProgramLauncher.h"
#include "FileItem.h"

#include <string>

class CProgramDatabase;
struct SProgramSettings;

namespace LAUNCHERS
{
  typedef struct
  {
    const char* name;
    const char* shortname;
    const char* extension;
  } SystemMapping;

  class CROMLauncher : public IProgramLauncher
  {
  public:
    CROMLauncher(std::string strExecutable);
    virtual ~CROMLauncher(void);

    static bool FindEmulators(const std::string strRomFile, CFileItemList& emulators);

  protected:
    virtual bool LoadSettings();

  private:
    virtual bool Launch();
    virtual bool IsSupported();

    CFileItemPtr GetDefaultEmulator();

    std::string m_strExecutable;

    CProgramDatabase* m_database;
    SProgramSettings* m_settings;
  };
}
