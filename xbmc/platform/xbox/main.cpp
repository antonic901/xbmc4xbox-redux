/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/xbmc.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "application/ApplicationEnums.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/XTimeUtils.h"

#include "platform/xbox/XKHDD.h"
#include "platform/xbox/XKUtils.h"
#include "platform/xbox/storage/IoSupport.h"

#include <boost/make_shared.hpp>

void SetupDriveLetters()
{
  // map Q to home drive of xbe to load the config file
  std::string strExecutablePath;
  CUtil::GetHomePath(strExecutablePath);
  char szDevicePath[MAX_PATH];
  size_t colon = strExecutablePath.find(':');
  if (colon == std::string::npos)
    return;

  CIoSupport::GetPartition(strExecutablePath.substr(0, colon).c_str(), szDevicePath);
  strcat(szDevicePath, strExecutablePath.c_str() + colon + 1);

  CIoSupport::Dismount("Cdrom0");
  CIoSupport::RemapDriveLetter('D', "Cdrom0");
  CIoSupport::RemapDriveLetter('Q', szDevicePath);

  CIoSupport::MapHarddiskPartitions();

  // Remove aliases inherited from the dashboard/BIOS, including the former P mount.
  const char* legacyHddLetters = "ABCEFGPRSVWXYZ";
  for (const char* letter = legacyHddLetters; *letter; ++letter)
    CIoSupport::UnmapDriveLetter(*letter);
}

void main()
{
  //floating point precision to 24 bits (faster performance)
  _controlfp(_PC_24, _MCW_PC);

  SetupDriveLetters();

  const boost::shared_ptr<CSettingsComponent> settingsComponent = boost::make_shared<CSettingsComponent>();
  settingsComponent->Initialize();
  CServiceBroker::RegisterSettingsComponent(settingsComponent);

  // better 128mb ram support
  // set MTRRDefType memory type to write-back as done in other XBox apps - seems a bit of a hack as really the def type
  // should be uncachable and the mtrr/mask for ram instead set up for 128MB with writeback as is done in cromwell.
  MEMORYSTATUS stat;
  GlobalMemoryStatus(&stat);
  if (stat.dwTotalPhys > 67108864)
  {
    __asm
    {
      mov ecx, 0x2ff
      rdmsr
      mov al, 0x06
      wrmsr
    }
    settingsComponent->GetAdvancedSettings()->m_guiKeepInMemory = true;
  }

  // Initialize input devices
  XInitDevices(0, NULL);

  int status = XBMC_Run();
  if (status == -1)
  {
    // TODO: Initialize basic D3D and start FTP server
    // FatalErrorHandler and InitBasicD3D
  }

  CServiceBroker::GetSettingsComponent()->Deinitialize();
  CServiceBroker::UnregisterSettingsComponent();

#ifndef _DEBUG
  if (status == EXITCODE_POWERDOWN)
  {
    KODI::TIME::Sleep(200);
    XKHDD::SpindownHarddisk();
    XKUtils::XBOXPowerOff();
  }
  else if (status == EXITCODE_REBOOT)
  {
    KODI::TIME::Sleep(200);
    XKUtils::XBOXPowerCycle();
  }
  else if (status == EXITCODE_QUIT)
  {
    LD_LAUNCH_DASHBOARD ld;
    ZeroMemory(&ld, sizeof(LD_LAUNCH_DASHBOARD));
    ld.dwReason = XLD_LAUNCH_DASHBOARD_MAIN_MENU;
    XLaunchNewImage(0, (PLAUNCH_DATA)&ld);
  }
  else if (status == EXITCODE_RESTARTAPP)
  {
    char szXbePath[MAX_PATH];
    CIoSupport::GetXbePath(szXbePath);
    char szDevicePath[MAX_PATH];
    char* colon = strchr(szXbePath, ':');
    if (!colon)
      return;
    *colon = 0;
    CIoSupport::GetPartition(szXbePath, szDevicePath);
    std::string launchPath = std::string("D:") + (colon + 1);
    strcpy(szXbePath, launchPath.c_str());
    CUtil::LaunchXbe(szDevicePath, szXbePath, NULL, VIDEO_NULL, COUNTRY_NULL, NULL);
  }
#endif

  while(1) { KODI::TIME::Sleep(0); }
}
