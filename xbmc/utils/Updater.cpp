/*
 *  Copyright (C) 2025-2025
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Updater.h"

#include "GUIInfoManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/CurlFile.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/log.h"


bool CUpdaterJob::DoWork()
{
  std::string temp(VERSION_STRING);
  size_t iPos = temp.find("-");
  std::string revision = temp.substr(iPos + 1);

  if (revision.empty() || revision == "dev")
    return true;

  CLog::Log(LOGINFO, "Checking for new updates...");

  XFILE::CCurlFile httpUtil;
  std::string bodyResponse;
  if (!httpUtil.Get("https://github.com/antonic901/xbmc4xbox-redux/releases/download/nightly/version.txt", bodyResponse))
  {
    CLog::Log(LOGWARNING, "Failed to fetch version file");
    return false;
  }

  std::string lastRevision;
  std::istringstream iss(bodyResponse);
  std::getline(iss, lastRevision);

  if (!lastRevision.empty() && lastRevision != revision)
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "XBMC", g_localizeStrings.Get(24068));
    KODI::MESSAGING::CApplicationMessenger::Get().PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, "Skin.SetBool(updateavailable)");
  }

  return true;
}

bool CUpdaterJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  return true;
}
