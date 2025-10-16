/*
 *  Copyright (C) 2023-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "IProgramLauncher.h"

#include <string>

class CTrainer;
class CProgramDatabase;
struct SProgramSettings;

namespace LAUNCHERS
{
  class CXBELauncher : public IProgramLauncher
  {
  public:
    CXBELauncher(std::string strExecutable);
    virtual ~CXBELauncher(void);

    static CTrainer* LoadTrainer(unsigned int iTitleID);

    /*! \brief Applies a flicker filter patch to an Xbox Executable (XBE) file.
     \param strExecutable path to original XBE file that needs patching.
     \param strPatchedExecutable path to patched XBE
     */
    static bool ApplyFFPatch(const std::string& strExecutable, std::string& strPatchedExecutable);

  protected:
    virtual bool LoadSettings();

  private:
    virtual bool Launch();
    virtual bool IsSupported();

    std::string m_strExecutable;

    CTrainer* m_trainer;
    CProgramDatabase* m_database;
    SProgramSettings* m_settings;
  };
}
