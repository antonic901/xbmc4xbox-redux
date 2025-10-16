/*
 *  Copyright (C) 2023-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ProgramLauncher.h"

#include "LauncherFactory.h"
#include "URL.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace LAUNCHERS;

CProgramLauncher::CProgramLauncher()
{}

CProgramLauncher::~CProgramLauncher()
{}

bool CProgramLauncher::LaunchProgram(const std::string& strExecutable)
{
  const CURL url(strExecutable);
  return LaunchProgram(url);
}

bool CProgramLauncher::LaunchProgram(const CURL& url)
{
  try
  {
    CURL realURL = URIUtils::SubstitutePath(url);
    boost::shared_ptr<IProgramLauncher> pProgramLauncher(CLauncherFactory::Create(realURL));
    if (!pProgramLauncher.get())
      return false;

    if (pProgramLauncher->Launch())
      return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error launching %s", __FUNCTION__, url.GetRedacted().c_str());
  return false;
}
