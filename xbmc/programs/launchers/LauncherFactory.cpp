/*
 *  Copyright (C) 2023-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "LauncherFactory.h"

#include "URL.h"
#include "XBELauncher.h"
#include "settings/AdvancedSettings.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace LAUNCHERS;

/*!
 \brief Create a IProgramLauncher object of the executable specified in \e strPath .
 \param strPath Specifies the executable to access, can be a share or share with path.
 \return IProgramLauncher object to allow launching of executable.
 \sa IProgramLauncher
 */
IProgramLauncher* CLauncherFactory::Create(const CURL& url)
{
  // We currently only support executables from HDD
  if (!url.IsProtocol(""))
  {
    CLog::Log(LOGWARNING, "%s - unsupported protocol: %s", __FUNCTION__, url.GetProtocol().c_str());
    return false;
  }

  if (url.IsFileType("xbe"))
    return new CXBELauncher(url.Get());

  CLog::Log(LOGWARNING, "%s - unsupported executable: %s", __FUNCTION__, url.Get().c_str());
  return NULL;
}

