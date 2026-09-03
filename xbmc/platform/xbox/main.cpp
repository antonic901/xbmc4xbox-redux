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
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

#include "platform/xbox/storage/IoSupport.h"

#include <boost/make_shared.hpp>

void SetupDriveLetters()
{
  // map Q to home drive of xbe to load the config file
  std::string strExecutablePath;
  CUtil::GetHomePath(strExecutablePath);
  char szDevicePath[MAX_PATH];
  CIoSupport::GetPartition(strExecutablePath.c_str()[0], szDevicePath);
  strcat(szDevicePath, &strExecutablePath.c_str()[2]);

  CIoSupport::Dismount("Cdrom0");
  CIoSupport::RemapDriveLetter('D', "Cdrom0");
  CIoSupport::RemapDriveLetter('Q', szDevicePath);
  CIoSupport::RemapDriveLetter('C', "Harddisk0\\Partition2");
  CIoSupport::RemapDriveLetter('E', "Harddisk0\\Partition1");
  CIoSupport::RemapDriveLetter('X', "Harddisk0\\Partition3");
  CIoSupport::RemapDriveLetter('Y', "Harddisk0\\Partition4");
  CIoSupport::RemapDriveLetter('Z', "Harddisk0\\Partition5");

  // Attempt to read the LBA48 v3 patch partition table, if kernel supports the command and it exists.
  CIoSupport::ReadPartitionTable();

  if (CIoSupport::HasPartitionTable())
  {
    // Mount up to Partition15 if they are available.
    for (int i = EXTEND_PARTITION_BEGIN; i <= EXTEND_PARTITION_BEGIN+EXTEND_PARTITIONS_LIMIT - 1; i++)
    {
      char szDevice[32];
      if (CIoSupport::PartitionExists(i))
      {
        char cDriveLetter = 'A' + i - 1;
        char extendDriveLetter = CIoSupport::GetExtendedPartitionDriveLetter(cDriveLetter - EXTEND_DRIVE_BEGIN);
        sprintf(szDevice, "Harddisk0\\Partition%u", i);
        CIoSupport::RemapDriveLetter(extendDriveLetter, szDevice);
      }
    }
  }
  else
  {
    if (CIoSupport::DriveExists('F'))
      CIoSupport::RemapDriveLetter('F', "Harddisk0\\Partition6");
    if (CIoSupport::DriveExists('G'))
      CIoSupport::RemapDriveLetter('G', "Harddisk0\\Partition7");
  }
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
}
