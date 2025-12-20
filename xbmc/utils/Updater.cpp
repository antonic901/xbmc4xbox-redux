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
  std::vector<std::string> split = StringUtils::Split(VERSION_STRING, "-");
  std::string strVersion = split[0];
  std::string strRevision = split[1];
  std::string strUpdateChannel = split[2] == "py2" ? "nightly" : "nightly-python3";

  if (!g_infoManager.EvaluateBool("Skin.HasSetting(updateavailable)"))
  {
    if (strRevision.empty() || StringUtils::StartsWithNoCase(strRevision, "dev"))
      return true;

    CLog::Log(LOGINFO, "Checking for new updates...");

    std::string strURL = StringUtils::Format("https://github.com/antonic901/xbmc4xbox-redux/releases/download/%s/version.txt", strUpdateChannel.c_str());
    XFILE::CCurlFile httpUtil;
    std::string strLastRevision;
    if (!httpUtil.Get(strURL, strLastRevision))
    {
      CLog::Log(LOGWARNING, "Failed to fetch version file");
      return false;
    }
    StringUtils::Replace(strLastRevision, "\r", "");
    StringUtils::Replace(strLastRevision, "\n", "");
    StringUtils::Trim(strLastRevision);

    bool updateAvailable = !strLastRevision.empty() && strLastRevision != strRevision;
    if (updateAvailable)
      KODI::MESSAGING::CApplicationMessenger::Get().PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, "Skin.SetBool(updateavailable)");

    m_notify &= updateAvailable;
    m_install &= updateAvailable;
  }

  if (m_notify)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "XBMC", g_localizeStrings.Get(24068));

  if (m_install)
    DoInstall(strVersion, strRevision, strUpdateChannel);

  return true;
}

bool CUpdaterJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  return true;
}

void CUpdaterJob::DoInstall(const std::string& strCurrentVersion, const std::string& strCurrentRevision, const std::string& strUpdateChannel)
{
  if (CGUIDialogYesNo::ShowAndGetInput(24068, 38790))
  {
    CUSTOM_LAUNCH_DATA data;
    memset(&data, 0, sizeof(CUSTOM_LAUNCH_DATA));

    strcpy(data.reserved, StringUtils::Format("version=%s&revision=%s&channel=%s", strCurrentVersion.c_str(), strCurrentRevision.c_str(), strUpdateChannel.c_str()).c_str());
    data.executionType = 0;

    KODI::MESSAGING::CApplicationMessenger::Get().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, "Skin.ToggleSetting(updateavailable)");
    CUtil::RunXBE("Q:\\updater.xbe", NULL, VIDEO_NULL, COUNTRY_NULL, &data);
  }
}
