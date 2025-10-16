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

class CURL;

namespace LAUNCHERS
{
/*!
 \ingroup launchers
 \brief Wrappers for \e IProgramLauncher
 */
class CProgramLauncher
{
public:
  CProgramLauncher(void);
  virtual ~CProgramLauncher(void);

  static bool LaunchProgram(const std::string& strExecutable);
  static bool LaunchProgram(const CURL& url);
};
}
