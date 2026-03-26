/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/SystemGUIInfo.h"

#include "Application.h"
#include "FileItem.h"
#include "GUIPassword.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoHelper.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/AlarmClock.h"
#include "utils/FanController.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "windowing/WinSystem.h"
#include "windows/GUIMediaWindow.h"
#include "xbox/PlatformDefs.h"

using namespace KODI::GUILIB;
using namespace KODI::GUILIB::GUIINFO;

CSystemGUIInfo::CSystemGUIInfo()
  : m_lastSysHeatInfoTime(-SYSTEM_HEAT_UPDATE_INTERVAL),
    m_fanSpeed(0), m_fps(0.0f), m_frameCounter(0), m_lastFPSTime(0)
{
}

std::string CSystemGUIInfo::GetSystemHeatInfo(int info) const
{
  if (CTimeUtils::GetFrameTime() - m_lastSysHeatInfoTime >= SYSTEM_HEAT_UPDATE_INTERVAL)
  {
    m_lastSysHeatInfoTime = CTimeUtils::GetFrameTime();
    m_fanSpeed = CFanController::Instance()->GetFanSpeed();
    m_gpuTemp = CFanController::Instance()->GetGPUTemp();
    m_cpuTemp = CFanController::Instance()->GetCPUTemp();
  }

  std::string text;
  switch(info)
  {
    case SYSTEM_CPU_TEMPERATURE:
      return m_cpuTemp.IsValid() ? g_langInfo.GetTemperatureAsString(m_cpuTemp) : g_localizeStrings.Get(10005); // Not available
    case SYSTEM_GPU_TEMPERATURE:
      return m_gpuTemp.IsValid() ? g_langInfo.GetTemperatureAsString(m_gpuTemp) : g_localizeStrings.Get(10005);
    case SYSTEM_FAN_SPEED:
      text = StringUtils::Format("%i%%", m_fanSpeed * 2);
      break;
    case SYSTEM_CPU_USAGE:
      text = StringUtils::Format("%2.0f%%", (1.0f - g_application.m_idleThread.GetRelativeUsage()) * 100.0f);
      break;
  }
  return text;
}

void CSystemGUIInfo::UpdateFPS()
{
  m_frameCounter++;
  unsigned int curTime = CTimeUtils::GetFrameTime();

  float fTimeSpan = static_cast<float>(curTime - m_lastFPSTime);
  if (fTimeSpan >= 1000.0f)
  {
    fTimeSpan /= 1000.0f;
    m_fps = m_frameCounter / fTimeSpan;
    m_lastFPSTime = curTime;
    m_frameCounter = 0;
  }
}

bool CSystemGUIInfo::InitCurrentItem(CFileItem *item)
{
  return false;
}

bool CSystemGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_TIME:
      value = CDateTime::GetCurrentDateTime().GetAsLocalizedTime(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case SYSTEM_DATE:
      if (info.GetData3().empty())
        value = CDateTime::GetCurrentDateTime().GetAsLocalizedDate(true);
      else
        value = CDateTime::GetCurrentDateTime().GetAsLocalizedDate(info.GetData3());
      return true;
    case SYSTEM_FREE_SPACE:
    case SYSTEM_USED_SPACE:
    case SYSTEM_TOTAL_SPACE:
    case SYSTEM_FREE_SPACE_PERCENT:
    case SYSTEM_USED_SPACE_PERCENT:
      value = g_sysinfo.GetHddSpaceInfo(info.m_info);
      return true;
    case SYSTEM_CPU_TEMPERATURE:
    case SYSTEM_GPU_TEMPERATURE:
    case SYSTEM_FAN_SPEED:
    case SYSTEM_CPU_USAGE:
      value = GetSystemHeatInfo(info.m_info);
      return true;
    case SYSTEM_VIDEO_ENCODER_INFO:
    case NETWORK_IP_ADDRESS:
    case NETWORK_MAC_ADDRESS:
    case NETWORK_SUBNET_MASK:
    case NETWORK_GATEWAY_ADDRESS:
    case NETWORK_DNS1_ADDRESS:
    case NETWORK_DNS2_ADDRESS:
    case NETWORK_LINK_STATE:
    case SYSTEM_OS_VERSION_INFO:
    case SYSTEM_CPUFREQUENCY:
    case SYSTEM_INTERNET_STATE:
    case SYSTEM_UPTIME:
    case SYSTEM_TOTALUPTIME:
    case SYSTEM_BATTERY_LEVEL:
      value = g_sysinfo.GetInfo(info.m_info);
      return true;
    case SYSTEM_PRIVACY_POLICY:
      value = g_localizeStrings.Get(416);
      return true;
    case SYSTEM_SCREEN_RESOLUTION:
    {
      value = StringUtils::Format("%ix%i %s %02.2f fps.",
        CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iWidth,
        CDisplaySettings::GetInstance().GetCurrentResolutionInfo().iHeight,
        CDisplaySettings::GetInstance().GetCurrentResolutionInfo().strMode.c_str(),
        GetFPS());
      return true;
    }
    case SYSTEM_BUILD_VERSION_SHORT:
      value = SVN_APP_VERSION_SHORT;
      return true;
    case SYSTEM_BUILD_VERSION:
      value = StringUtils::Format("Git: %s", SVN_REV);
      return true;
    case SYSTEM_BUILD_DATE:
      value = __DATE__;
      return true;
    case SYSTEM_BUILD_VERSION_CODE:
      value = SVN_APP_VERSION_CODE;
      return true;
    case SYSTEM_BUILD_VERSION_GIT:
      value = SVN_REV;
      return true;
    case SYSTEM_FREE_MEMORY:
    case SYSTEM_FREE_MEMORY_PERCENT:
    case SYSTEM_USED_MEMORY:
    case SYSTEM_USED_MEMORY_PERCENT:
    case SYSTEM_TOTAL_MEMORY:
    {
      MEMORYSTATUS stat;
      GlobalMemoryStatus(&stat);
      int iMemPercentFree = 100 - ((int)( 100.0f* (stat.dwTotalPhys - stat.dwAvailPhys) / stat.dwTotalPhys + 0.5f ));
      int iMemPercentUsed = 100 - iMemPercentFree;

      if (info.m_info == SYSTEM_FREE_MEMORY)
        value = StringUtils::Format("%iMB", stat.dwAvailPhys /MB);
      else if (info.m_info == SYSTEM_FREE_MEMORY_PERCENT)
        value = StringUtils::Format("%i%%", iMemPercentFree);
      else if (info.m_info == SYSTEM_USED_MEMORY)
        value = StringUtils::Format("%iMB", (stat.dwTotalPhys - stat.dwAvailPhys)/MB);
      else if (info.m_info == SYSTEM_USED_MEMORY_PERCENT)
        value = StringUtils::Format("%i%%", iMemPercentUsed);
      else if (info.m_info == SYSTEM_TOTAL_MEMORY)
        value = StringUtils::Format("%iMB", stat.dwTotalPhys/MB);
      return true;
    }
    case SYSTEM_SCREEN_MODE:
      value = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo().strMode;
      return true;
    case SYSTEM_SCREEN_WIDTH:
      value = StringUtils::Format(
          "%i", CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo().iWidth);
      return true;
    case SYSTEM_SCREEN_HEIGHT:
      value = StringUtils::Format(
          "%i", CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo().iHeight);
      return true;
    case SYSTEM_FPS:
      value = StringUtils::Format("%02.2f", m_fps);
      return true;
#ifdef HAS_OPTICAL_DRIVE
    case SYSTEM_DVD_LABEL:
      value = CServiceBroker::GetMediaManager().GetDiskLabel();
      return true;
#endif
    case SYSTEM_ALARM_POS:
      if (g_alarmClock.GetRemaining("shutdowntimer") == 0.0)
        value.clear();
      else
      {
        double fTime = g_alarmClock.GetRemaining("shutdowntimer");
        if (fTime > 60.0)
          value = StringUtils::Format(g_localizeStrings.Get(13213).c_str(),
                                      g_alarmClock.GetRemaining("shutdowntimer") / 60.0);
        else
          value = StringUtils::Format(g_localizeStrings.Get(13214).c_str(),
                                      g_alarmClock.GetRemaining("shutdowntimer"));
      }
      return true;
    case SYSTEM_PROFILENAME:
      value = CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetCurrentProfile().getName();
      return true;
    case SYSTEM_PROFILECOUNT:
      value = StringUtils::Format("%zu", CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetNumberOfProfiles());
      return true;
    case SYSTEM_PROFILEAUTOLOGIN:
    {
      const boost::shared_ptr<CProfilesManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();
      int iProfileId = profileManager->GetAutoLoginProfileId();
      if ((iProfileId < 0) || !profileManager->GetProfileName(iProfileId, value))
        value = g_localizeStrings.Get(37014); // Last used profile
      return true;
    }
    case SYSTEM_PROFILETHUMB:
    {
      const std::string& thumb = CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetCurrentProfile().getThumb();
      value = thumb.empty() ? "DefaultUser.png" : thumb;
      return true;
    }
    case SYSTEM_LANGUAGE:
      value = g_langInfo.GetEnglishLanguageName();
      return true;
    case SYSTEM_TEMPERATURE_UNITS:
      value = g_langInfo.GetTemperatureUnitString();
      return true;
    case SYSTEM_FRIENDLY_NAME:
      value = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString("services.devicename");
      return true;
    case SYSTEM_GET_CORE_USAGE:
      value = StringUtils::Format("%4.2f", 100 - ((int)(100.0f *g_application.m_idleThread.GetRelativeUsage())));
      return true;
    case SYSTEM_RENDER_VENDOR:
      value = "Microsoft";
      return true;
    case SYSTEM_RENDER_RENDERER:
      value = "DirectX";
      return true;
    case SYSTEM_RENDER_VERSION:
      value = "8.0";
      return true;
#if defined(TARGET_LINUX)
    case SYSTEM_PLATFORM_WINDOWING:
      value = CServiceBroker::GetWinSystem()->GetName();
      StringUtils::ToCapitalize(value);
      return true;
#endif
    case SYSTEM_SUPPORTED_HDR_TYPES:
    {
      value = g_localizeStrings.Get(416);
      return true;
    }

    case SYSTEM_LOCALE_TIMEZONE:
    {
      value = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
          "locale.timezone");
      return true;
    }

    case SYSTEM_LOCALE_REGION:
    {
      value = g_langInfo.GetCurrentRegion();
      return true;
    }

    case SYSTEM_LOCALE:
    {
      value = g_langInfo.GetRegionLocale();
      return true;
    }
  }
  return false;
}

bool CSystemGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_FREE_MEMORY:
    case SYSTEM_USED_MEMORY:
    {
      MEMORYSTATUS stat;
      GlobalMemoryStatus(&stat);
      int memPercentUsed = static_cast<int>(100.0f * (stat.dwTotalPhys - stat.dwAvailPhys) / stat.dwTotalPhys + 0.5f);
      if (info.m_info == SYSTEM_FREE_MEMORY)
        value = 100 - memPercentUsed;
      else
        value = memPercentUsed;
      return true;
    }
#ifdef HAS_XBOX_HARDWARE
    case SYSTEM_FREE_SPACE_C:
    case SYSTEM_FREE_SPACE_E:
    case SYSTEM_FREE_SPACE_F:
    case SYSTEM_FREE_SPACE_G:
    case SYSTEM_FREE_SPACE_X:
    case SYSTEM_FREE_SPACE_Y:
    case SYSTEM_FREE_SPACE_Z:
    case SYSTEM_USED_SPACE_C:
    case SYSTEM_USED_SPACE_E:
    case SYSTEM_USED_SPACE_F:
    case SYSTEM_USED_SPACE_G:
    case SYSTEM_USED_SPACE_X:
    case SYSTEM_USED_SPACE_Y:
    case SYSTEM_USED_SPACE_Z:
#endif
    case SYSTEM_FREE_SPACE:
    case SYSTEM_USED_SPACE:
    {
      g_sysinfo.GetHddSpaceInfo(value, info.m_info, true);
      return true;
    }
    case SYSTEM_CPU_USAGE:
      value = 100 - static_cast<int>(100.0f * g_application.m_idleThread.GetRelativeUsage());
      return true;
    case SYSTEM_BATTERY_LEVEL:
      value = 0;
      return true;
  }

  return false;
}

bool CSystemGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_ALWAYS_TRUE:
      value = true;
      return true;
    case SYSTEM_ALWAYS_FALSE:
      value = false;
      return true;
    case SYSTEM_ETHERNET_LINK_ACTIVE:
      // wtf: not implemented - always returns true?!
      value = true;
      return true;
    case SYSTEM_PLATFORM_LINUX:
#if defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_WINDOWS:
#ifdef TARGET_WINDOWS
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_UWP:
#ifdef TARGET_WINDOWS_STORE
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_DARWIN:
#ifdef TARGET_DARWIN
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_DARWIN_OSX:
#ifdef TARGET_DARWIN_OSX
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_DARWIN_IOS:
#ifdef TARGET_DARWIN_IOS
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_DARWIN_TVOS:
#ifdef TARGET_DARWIN_TVOS
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_ANDROID:
#if defined(TARGET_ANDROID)
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_WEBOS:
#if defined(TARGET_WEBOS)
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_MEDIA_DVD:
    {
      int iTrayState = CIoSupport::GetTrayState();
      if ( iTrayState == DRIVE_CLOSED_MEDIA_PRESENT || iTrayState == TRAY_CLOSED_MEDIA_PRESENT )
        value = MEDIA_DETECT::CDetectDVDMedia::IsDiscInDrive();
      else
        value = false;
      return true;
    }
    case SYSTEM_MEDIA_AUDIO_CD:
#ifdef HAS_OPTICAL_DRIVE
      if (CServiceBroker::GetMediaManager().IsDiscInDrive())
      {
        MEDIA_DETECT::CCdInfo* pCdInfo = CServiceBroker::GetMediaManager().GetCdInfo();
        value = pCdInfo && (pCdInfo->IsAudio(1) || pCdInfo->IsCDExtra(1) || pCdInfo->IsMixedMode(1));
      }
      else
    #endif
      {
        value = false;
      }
      return true;
#ifdef HAS_OPTICAL_DRIVE
    case SYSTEM_DVDREADY:
      value = CServiceBroker::GetMediaManager().GetDriveStatus() != DriveState::NOT_READY;
      return true;
    case SYSTEM_TRAYOPEN:
      value = CServiceBroker::GetMediaManager().GetDriveStatus() == DriveState::OPEN;
      return true;
#endif
    case SYSTEM_CAN_POWERDOWN:
      value = true;
      return true;
    case SYSTEM_CAN_SUSPEND:
      value = false;
      return true;
    case SYSTEM_CAN_HIBERNATE:
      value = false;
      return true;
    case SYSTEM_CAN_REBOOT:
      value = true;
      return true;
    case SYSTEM_SCREENSAVER_ACTIVE:
    case SYSTEM_IS_SCREENSAVER_INHIBITED:
    case SYSTEM_DPMS_ACTIVE:
    case SYSTEM_IDLE_SHUTDOWN_INHIBITED:
    case SYSTEM_IDLE_TIME:
    {
      switch (info.m_info)
      {
        case SYSTEM_SCREENSAVER_ACTIVE:
          value = g_application.IsInScreenSaver();
          return true;
        case SYSTEM_DPMS_ACTIVE:
          value = false;
          return true;
        case SYSTEM_IDLE_TIME:
          value = g_application.GlobalIdleTime() >= (int)info.GetData1();
          return true;
        default:
          return false;
      }
    }
    case SYSTEM_HASLOCKS:
      value = CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE;
      return true;
    case SYSTEM_HAS_PVR:
      value = false;
      return true;
    case SYSTEM_HAS_PVR_ADDON:
      value = false;
      return true;
    case SYSTEM_HAS_CMS:
#if defined(HAS_GL)
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_ISMASTER:
      value = CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && g_passwordManager.bMasterUser;
      return true;
    case SYSTEM_ISFULLSCREEN:
      value = true;
      return true;
    case SYSTEM_ISSTANDALONE:
      value = true;
      return true;
    case SYSTEM_HAS_SHUTDOWN:
      value = (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("powermanagement.shutdowntime") > 0);
      return true;
    case SYSTEM_LOGGEDON:
      value = !(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_LOGIN_SCREEN);
      return true;
    case SYSTEM_SHOW_EXIT_BUTTON:
      value = true;
      return true;
    case SYSTEM_HAS_LOGINSCREEN:
      value = CServiceBroker::GetSettingsComponent()->GetProfileManager()->UsingLoginScreen();
      return true;
    case SYSTEM_INTERNET_STATE:
    {
      g_sysinfo.GetInfo(info.m_info);
      value = g_sysinfo.HasInternet();
      return true;
    }
    case SYSTEM_HAS_CORE_ID:
      value = false;
      return true;
    case SYSTEM_DATE:
    {
      if (info.GetData2() == -1) // info doesn't contain valid startDate
        return false;
      const CDateTime date = CDateTime::GetCurrentDateTime();
      int currentDate = date.GetMonth() * 100 + date.GetDay();
      int startDate = info.GetData1();
      int stopDate = info.GetData2();

      if (stopDate < startDate)
        value = currentDate >= startDate || currentDate < stopDate;
      else
        value = currentDate >= startDate && currentDate < stopDate;
      return true;
    }
    case SYSTEM_TIME:
    {
      int currentTime = CDateTime::GetCurrentDateTime().GetMinuteOfDay();
      int startTime = info.GetData1();
      int stopTime = info.GetData2();

      if (stopTime < startTime)
        value = currentTime >= startTime || currentTime < stopTime;
      else
        value = currentTime >= startTime && currentTime < stopTime;
      return true;
    }
    case SYSTEM_ALARM_LESS_OR_EQUAL:
    {
      int time = MathUtils::round_int(g_alarmClock.GetRemaining(info.GetData3()));
      int timeCompare = info.GetData2();
      if (time > 0)
        value = timeCompare >= time;
      else
        value = false;
      return true;
    }
    case SYSTEM_HAS_ALARM:
      value = g_alarmClock.HasAlarm(info.GetData3());
      return true;
    case SYSTEM_SUPPORTS_CPU_USAGE:
      value = true;
      return true;
    case SYSTEM_GET_BOOL:
      value = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(info.GetData3());
      return true;
    case SYSTEM_SETTING:
    {
      if (StringUtils::EqualsNoCase(info.GetData3(), "hidewatched"))
      {
        CGUIMediaWindow* window = GUIINFO::GetMediaWindow(contextWindow);
        if (window)
        {
          value = CMediaSettings::GetInstance().GetWatchedMode(window->CurrentDirectory().GetContent()) == WatchedModeUnwatched;
          return true;
        }
      }
      else if (StringUtils::EqualsNoCase(info.GetData3(), "hideunwatchedepisodethumbs"))
      {
        value = !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("videolibrary.showunwatchedplots");
        return true;
      }
      break;
    }
  }

  return false;
}
