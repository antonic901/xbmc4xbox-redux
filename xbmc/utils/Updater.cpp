/*
 *  Copyright (C) 2025-2025
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Updater.h"

#include "GUIInfoManager.h"
#include "Util.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/CurlFile.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/log.h"
#include "xbox/custom_launch_params.h"


CUpdaterJob::CUpdaterJob(bool notify /* = false */, bool install /* = false */)
  : CJob(),
    m_notify(notify),
    m_install(install)
{ }

bool CUpdaterJob::DoWork()
{
  if (!g_infoManager.EvaluateBool("Skin.HasSetting(updateavailable)"))
  {
    std::string revision(VERSION_STRING);
    size_t iPos = revision.find("-");
    revision = revision.substr(iPos + 1);

    if (revision.empty() || StringUtils::StartsWithNoCase(revision, "dev"))
      return true;

    CLog::Log(LOGINFO, "Checking for new updates...");

    std::string strURL = "https://github.com/antonic901/xbmc4xbox-redux/releases/download/nightly/version.txt";
    if (StringUtils::EndsWithNoCase(VERSION_STRING, "py3"))
      strURL = "https://github.com/antonic901/xbmc4xbox-redux/releases/download/nightly-python3/version.txt";

    XFILE::CCurlFile httpUtil;
    std::string bodyResponse;
    if (!httpUtil.Get(strURL, bodyResponse))
    {
      CLog::Log(LOGWARNING, "Failed to fetch version file");
      return false;
    }

    std::string lastRevision;
    std::istringstream iss(bodyResponse);
    std::getline(iss, lastRevision);

    bool updateAvailable = !lastRevision.empty() && lastRevision != revision;
    if (updateAvailable)
      KODI::MESSAGING::CApplicationMessenger::Get().PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, "Skin.SetBool(updateavailable)");

    m_notify &= updateAvailable;
    m_install &= updateAvailable;
  }

  if (m_notify)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "XBMC", g_localizeStrings.Get(24068));

  if (m_install)
    DoInstall();

  return true;
}

bool CUpdaterJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  return true;
}

void CUpdaterJob::DoInstall()
{
  if (CGUIDialogYesNo::ShowAndGetInput(24068, 38790))
  {
    CUSTOM_LAUNCH_DATA data;
    memset(&data, 0, sizeof(CUSTOM_LAUNCH_DATA));

    std::vector<std::string> split = StringUtils::Split(VERSION_STRING, "-");
    std::string strVersion = split[0];
    std::string strRevision = split[1];
    std::string strUpdateChannel = split[2] == "py2" ? "nightly" : "nightly-python3";

    strcpy(data.reserved, StringUtils::Format("version=%s&revision=%s&channel=%s", strVersion.c_str(), strRevision.c_str(), strUpdateChannel.c_str()).c_str());
    data.executionType = 0;

    KODI::MESSAGING::CApplicationMessenger::Get().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, "Skin.ToggleSetting(updateavailable)");
    CUtil::RunXBE("Q:\\updater.xbe", NULL, VIDEO_NULL, COUNTRY_NULL, &data);
  }
}
