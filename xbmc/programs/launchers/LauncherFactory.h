/*
 *  Copyright (C) 2023-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "IProgramLauncher.h"

class CURL;

namespace LAUNCHERS
{
/*!
 \ingroup launchers
 \brief Get a launcher of given executable.
 \n
 Example:

 \verbatim
 std::string strExecutable="F:\Games\Halo: Combat Evolved\default.xbe";

 IProgramLauncher* pLauncher=CLauncherFactory::Create(strExecutable);
 \endverbatim
 The \e pLauncher pointer can be used to launch a program executable.
 \sa IProgramLauncher
 */
class CLauncherFactory
{
public:
  static IProgramLauncher* Create(const CURL& url);
};
}
