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
#include "addons/AddonVersion.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/CurlFile.h"
#include "guilib/GUIComponent.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/RegExp.h"
#include "utils/log.h"

#include "platform/xbox/custom_launch_params.h"

CUpdaterJob::CUpdaterJob(bool notify /* = false */, bool install /* = false */)
  : CJob(),
    m_notify(notify),
    m_install(install)
{ }

bool CUpdaterJob::DoWork()
{
  bool allowMajorUpgrade = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_UPDATER_PERFORM_MAJOR_UPGRADE);
  if (!CServiceBroker::GetGUI()->GetInfoManager().EvaluateBool("Skin.HasSetting(updateavailable)", INFO::DEFAULT_CONTEXT))
  {
    CLog::Log(LOGINFO, "Checking for new updates...");
    XFILE::CCurlFile httpUtil;
    std::string channel(APP_CHANNEL);

    CVariant data;
    bool updateAvailable = false;
    if (channel == "stable")
    {
      int page = 1;
      do
      {      
        std::string response;
        if (!httpUtil.Get(StringUtils::Format("https://api.github.com/repos/antonic901/xbmc4xbox-redux/releases?page=%i", page), response))
        {
          CLog::Log(LOGWARNING, "Check for update failed!");
          return false;
        }

        data = CJSONVariantParser::Parse(response);
        if (data.isNull() || !data.isArray())
        {
          CLog::Log(LOGWARNING, "Check for update failed! API returned invalid response.");
          return false;
        }

        ADDON::CAddonVersion currentVersion(APP_VERSION_CODE);
        for (CVariant::iterator_array it = data.begin_array(); it != data.end_array(); ++it)
        {
          CVariant release = *it;
          if (!release.isObject() || !release.isMember("tag_name"))
            continue;

          std::string strVersion = release["tag_name"].asString();

          CRegExp regex;
          if(!regex.RegComp("\\A[0-9]+\\.[0-9]+\\.[0-9]+\\z") || regex.RegFind(strVersion) != 0)
            continue;

          std::vector<std::string> vecVersion = StringUtils::Split(strVersion, ".");
          int major = atoi(vecVersion[0].c_str());

          // Ignore major upgrade if not allowed
          if (!allowMajorUpgrade && major > atoi(APP_VERSION_MAJOR))
            continue;

          ADDON::CAddonVersion version(strVersion);
          if (version > currentVersion)
          {
            updateAvailable = true;
            break;
          }
        }
        page++;
      } while (!updateAvailable && data.size() != 0);
    }
    else/* if (channel == "nightly") */
    {
      std::string strLastRevision;
      if (!httpUtil.Get("https://github.com/antonic901/xbmc4xbox-redux/releases/download/nightly/version.txt", strLastRevision))
      {
        CLog::Log(LOGWARNING, "Failed to fetch version file");
        return false;
      }
      StringUtils::Replace(strLastRevision, "\r", "");
      StringUtils::Replace(strLastRevision, "\n", "");
      StringUtils::Trim(strLastRevision);
      updateAvailable = !strLastRevision.empty() && strLastRevision != APP_SCMID;
    }

    if (updateAvailable)
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, NULL, "Skin.SetBool(updateavailable)");
    m_notify &= updateAvailable;
    m_install &= updateAvailable;
  }

  if (m_notify)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "Xodi", g_localizeStrings.Get(24068));

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

    bool allowMajorUpgrade = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_UPDATER_PERFORM_MAJOR_UPGRADE);
    strcpy(data.reserved, StringUtils::Format("version=%s&revision=%s&channel=%s&perform_major_upgrade=%s", APP_VERSION_CODE, APP_SCMID, APP_CHANNEL, allowMajorUpgrade ? "true" : "false").c_str());
    data.executionType = 0;

    CServiceBroker::GetAppMessenger()->SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, NULL, "Skin.ToggleSetting(updateavailable)");
    CUtil::RunXBE("Q:\\updater.xbe", NULL, VIDEO_NULL, COUNTRY_NULL, &data);
  }
}
