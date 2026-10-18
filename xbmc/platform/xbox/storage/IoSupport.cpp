/*
* XBMC Media Center
* Copyright (c) 2002 d7o3g4q and RUNTiME
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// IoSupport.cpp: implementation of the CIoSupport class.
//
//////////////////////////////////////////////////////////////////////

#include "system.h"
#include "IoSupport.h"
#include "utils/DrivePath.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#ifdef HAS_UNDOCUMENTED
#ifdef _XBOX
#include "Undocumented.h"
#include "XKExports.h"
#else
#include "ntddcdrm.h"
#endif
#endif

#define NT_STATUS_OBJECT_NAME_NOT_FOUND long(0xC0000000 | 0x0034)
#define NT_STATUS_VOLUME_DISMOUNTED     long(0xC0000000 | 0x026E)

struct HarddiskMapping
{
  const char* name;
  const char* device;
  int disk;
  int partition;
};

static const HarddiskMapping harddiskMapping[] =
{
  { "HDD0-E", "Harddisk0\\Partition1", 0, 1 },
  { "HDD0-C", "Harddisk0\\Partition2", 0, 2 },
  { "HDD0-X", "Harddisk0\\Partition3", 0, 3 },
  { "HDD0-Y", "Harddisk0\\Partition4", 0, 4 },
  { "HDD0-Z", "Harddisk0\\Partition5", 0, 5 },
  { "HDD0-F", "Harddisk0\\Partition6", 0, 6 },
  { "HDD0-G", "Harddisk0\\Partition7", 0, 7 },
  { "HDD0-R", "Harddisk0\\Partition8", 0, 8 },
  { "HDD0-S", "Harddisk0\\Partition9", 0, 9 },
  { "HDD0-V", "Harddisk0\\Partition10", 0, 10 },
  { "HDD0-W", "Harddisk0\\Partition11", 0, 11 },
  { "HDD0-A", "Harddisk0\\Partition12", 0, 12 },
  { "HDD0-B", "Harddisk0\\Partition13", 0, 13 },
  { "HDD0-N", "Harddisk0\\Partition14", 0, 14 },
  { "HDD1-E", "Harddisk1\\Partition1", 1, 1 },
  { "HDD1-C", "Harddisk1\\Partition2", 1, 2 },
  { "HDD1-X", "Harddisk1\\Partition3", 1, 3 },
  { "HDD1-Y", "Harddisk1\\Partition4", 1, 4 },
  { "HDD1-Z", "Harddisk1\\Partition5", 1, 5 },
  { "HDD1-F", "Harddisk1\\Partition6", 1, 6 },
  { "HDD1-G", "Harddisk1\\Partition7", 1, 7 },
  { "HDD1-R", "Harddisk1\\Partition8", 1, 8 },
  { "HDD1-S", "Harddisk1\\Partition9", 1, 9 },
  { "HDD1-V", "Harddisk1\\Partition10", 1, 10 },
  { "HDD1-W", "Harddisk1\\Partition11", 1, 11 },
  { "HDD1-A", "Harddisk1\\Partition12", 1, 12 },
  { "HDD1-B", "Harddisk1\\Partition13", 1, 13 },
  { "HDD0-N", "Harddisk1\\Partition14", 0, 14 },
};

PVOID CIoSupport::m_rawXferBuffer;
PARTITION_TABLE CIoSupport::m_partitionTable;
bool CIoSupport::m_fPartitionTableIsValid;


HRESULT CIoSupport::MapDrive(const char* name, const char* device)
{
#ifdef _XBOX
  char target[MAX_PATH + 32];
  char link[32];
  sprintf(target, "\\Device\\%s", device);
  sprintf(link, "\\??\\%s:", name);
  ANSI_STRING targetName, linkName;
  RtlInitAnsiString(&targetName, target);
  RtlInitAnsiString(&linkName, link);
  IoDeleteSymbolicLink(&linkName);
  NTSTATUS status = IoCreateSymbolicLink(&linkName, &targetName);
  if (!NT_SUCCESS(status))
    CLog::Log(LOGERROR, "Failed to map %s to %s (status=0x%08x)", name, device, status);
  return status;
#else
  return E_FAIL;
#endif
}

bool CIoSupport::DriveExists(const char* name)
{
  if (!DrivePath::IsName(name))
    return false;
#ifdef _XBOX
  char link[32];
  if (strlen(name) > 6)
    return false;
  sprintf(link, "\\??\\%s:", name);
  ANSI_STRING linkName;
  RtlInitAnsiString(&linkName, link);
  OBJECT_ATTRIBUTES oa;
  oa.Attributes = OBJ_CASE_INSENSITIVE;
  oa.ObjectName = &linkName;
  oa.RootDirectory = 0;
  IO_STATUS_BLOCK iosb;
  HANDLE handle;
  NTSTATUS status = NtOpenFile(&handle, GENERIC_READ, &oa, &iosb,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);
  if (NT_SUCCESS(status))
  {
    CloseHandle(handle);
    return true;
  }
#endif
  return false;
}

void CIoSupport::GetPartition(const char* name, char* partition)
{
  for (int i = 0; i < HDD_PARTITIONS_COUNT; ++i)
    if (stricmp(name, harddiskMapping[i].name) == 0)
    {
      strcpy(partition, harddiskMapping[i].device);
      return;
    }
  *partition = 0;
  if (stricmp(name, "D") == 0)
    strcpy(partition, "Cdrom0");
}

void CIoSupport::GetDriveName(const char* partition, char* name)
{
  if (strnicmp(partition, "\\Device\\", 8) == 0)
    partition += 8;
  for (int i = 0; i < HDD_PARTITIONS_COUNT; ++i)
  {
    size_t length = strlen(harddiskMapping[i].device);
    if (strnicmp(partition, harddiskMapping[i].device, length) == 0 &&
        (partition[length] == 0 || partition[length] == '\\'))
    {
      strcpy(name, harddiskMapping[i].name);
      return;
    }
  }
  strcpy(name, stricmp(partition, "Cdrom0") == 0 ||
               strnicmp(partition, "Cdrom0\\", 7) == 0 ? "D" : "");
}

const char* CIoSupport::GetHarddiskDriveName(int position)
{
  return position >= 0 && position < HDD_PARTITIONS_COUNT ? harddiskMapping[position].name : "";
}

void CIoSupport::MapHarddiskPartitions()
{
  for (int i = 0; i < HDD_PARTITIONS_COUNT; ++i)
  {
    const HarddiskMapping& mapping = harddiskMapping[i];
    if (PartitionExists(mapping.partition, mapping.disk))
      MapDrive(mapping.name, mapping.device);
  }
}

// cDriveLetter e.g. 'D'
// szDevice e.g. "Cdrom0" or "Harddisk0\Partition6"
HRESULT CIoSupport::MapDriveLetter(char cDriveLetter, const char* szDevice)
{
  if (!DrivePath::IsName(std::string(1, cDriveLetter)))
    return E_INVALIDARG;
#ifdef _XBOX
  char szSourceDevice[MAX_PATH+32];
  char szDestinationDrive[16];
  NTSTATUS status;

  sprintf(szSourceDevice, "\\Device\\%s", szDevice);
  sprintf(szDestinationDrive, "\\??\\%c:", cDriveLetter);

  ANSI_STRING DeviceName, LinkName;

  RtlInitAnsiString(&DeviceName, szSourceDevice);
  RtlInitAnsiString(&LinkName, szDestinationDrive);

  status = IoCreateSymbolicLink(&LinkName, &DeviceName);

  if (!NT_SUCCESS(status))
    CLog::Log(LOGERROR, "Failed to create symbolic link!  (status=0x%08x)", status);
  else
    CLog::Log(LOGINFO, "Mapping drive %c to %s", cDriveLetter, szDevice);

  return status;
#else
  if ((strnicmp(szDevice, "Harddisk0", 9) == 0) ||
      (strnicmp(szDevice, "Harddisk1", 9) == 0) ||
      (strnicmp(szDevice, "Cdrom", 5) == 0))
    return S_OK;
  return E_FAIL;
#endif
}

// cDriveLetter e.g. 'D'
HRESULT CIoSupport::UnmapDriveLetter(char cDriveLetter)
{
#ifdef _XBOX
  char szDestinationDrive[16];
  ANSI_STRING LinkName;
  NTSTATUS status;

  sprintf(szDestinationDrive, "\\??\\%c:", cDriveLetter);
  RtlInitAnsiString(&LinkName, szDestinationDrive);

  status =  IoDeleteSymbolicLink(&LinkName);

  if (NT_SUCCESS(status))
    CLog::Log(LOGINFO, "Unmapped drive %c", cDriveLetter);
  else if(status != NT_STATUS_OBJECT_NAME_NOT_FOUND)
    CLog::Log(LOGERROR, "Failed to delete symbolic link!  (status=0x%08x)", status);

  return status;
#else
  return S_OK;
#endif
}

HRESULT CIoSupport::RemapDriveLetter(char cDriveLetter, const char* szDevice)
{
  UnmapDriveLetter(cDriveLetter);

  return MapDriveLetter(cDriveLetter, szDevice);
}
// to be used with CdRom devices.
HRESULT CIoSupport::Dismount(const char* szDevice)
{
#ifdef _XBOX
  char szSourceDevice[MAX_PATH+32];
  ANSI_STRING DeviceName;
  NTSTATUS status;

  sprintf(szSourceDevice, "\\Device\\%s", szDevice);

  RtlInitAnsiString(&DeviceName, szSourceDevice);

  status = IoDismountVolumeByName(&DeviceName);

  if (NT_SUCCESS(status))
    CLog::Log(LOGINFO, "Dismounted %s", szDevice);
  else if(status != NT_STATUS_VOLUME_DISMOUNTED)
    CLog::Log(LOGERROR, "Failed to dismount volume!  (status=0x%08x)", status);

  return status;
#else
  return S_OK;
#endif
}

HRESULT CIoSupport::EjectTray()
{
#ifdef _WIN32PC
  BOOL bRet= FALSE;
  char cDL = cDriveLetter;
  if( !cDL )
  {
    char* dvdDevice = CLibcdio::GetInstance()->GetDeviceFileName();
    cDL = dvdDevice[4];
  }

  std::string strVolFormat = StringUtils::Format( _T("\\\\.\\%c:" ), cDL);
  HANDLE hDrive= CreateFile( strVolFormat, GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  std::string strRootFormat = StringUtils::Format( _T("%c:\\"), cDL);
  if( ( hDrive != INVALID_HANDLE_VALUE || GetLastError() == NO_ERROR) &&
      ( GetDriveType( strRootFormat ) == DRIVE_CDROM ) )
  {
    DWORD dwDummy;
    bRet= DeviceIoControl( hDrive, ( bEject ? IOCTL_STORAGE_EJECT_MEDIA : IOCTL_STORAGE_LOAD_MEDIA),
                                    NULL, 0, NULL, 0, &dwDummy, NULL);
    CloseHandle( hDrive );
  }
  return bRet? S_OK : S_FALSE;
#endif
#ifdef _XBOX
  HalWriteSMBusValue(0x20, 0x0C, FALSE, 0);  // eject tray
#endif
  return S_OK;
}

HRESULT CIoSupport::CloseTray()
{
#ifdef _XBOX
  HalWriteSMBusValue(0x20, 0x0C, FALSE, 1);  // close tray
#endif
  return S_OK;
}

DWORD CIoSupport::GetTrayState()
{
#ifdef _XBOX
  DWORD dwTrayState, dwTrayCount;
  HalReadSMCTrayState(&dwTrayState, &dwTrayCount);

  return dwTrayState;
#endif
  return DRIVE_NOT_READY;
}

HRESULT CIoSupport::ToggleTray()
{
  if (GetTrayState() == TRAY_OPEN || GetTrayState() == DRIVE_OPEN)
    return CloseTray();
  else
    return EjectTray();
}

HRESULT CIoSupport::Shutdown()
{
#ifdef _XBOX
  // fails assertion on debug bios (symptom lockup unless running dr watson)
  // so you can continue past the failed assertion).
  if (IsDebug())
    return E_FAIL;
  KeRaiseIrqlToDpcLevel();
  HalInitiateShutdown();
#endif
  return S_OK;
}

HANDLE CIoSupport::OpenCDROM()
{
  HANDLE hDevice;

#ifdef _XBOX
  IO_STATUS_BLOCK status;
  ANSI_STRING filename;
  OBJECT_ATTRIBUTES attributes;
  RtlInitAnsiString(&filename, "\\Device\\Cdrom0");
  InitializeObjectAttributes(&attributes, &filename, OBJ_CASE_INSENSITIVE, NULL);
  if (!NT_SUCCESS(NtOpenFile(&hDevice,
                             GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                             &attributes,
                             &status,
                             FILE_SHARE_READ,
                             FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT)))
  {
    return NULL;
  }
#else

  hDevice = CreateFile("\\\\.\\Cdrom0", GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING,
                       FILE_FLAG_RANDOM_ACCESS, NULL );

#endif
  return hDevice;
}

void CIoSupport::AllocReadBuffer()
{
  m_rawXferBuffer = GlobalAlloc(GPTR, RAW_SECTOR_SIZE);
}

void CIoSupport::FreeReadBuffer()
{
  GlobalFree(m_rawXferBuffer);
}

INT CIoSupport::ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)

{
  DWORD dwRead;
  DWORD dwSectorSize = 2048;
  LARGE_INTEGER Displacement;
  Displacement.QuadPart = ((INT64)dwSector) * dwSectorSize;

  for (int i = 0; i < 5; i++)
  {
    SetFilePointer(hDevice, Displacement.LowPart, &Displacement.HighPart, FILE_BEGIN);

    if (ReadFile(hDevice, m_rawXferBuffer, dwSectorSize, &dwRead, NULL))
    {
      memcpy(lpczBuffer, m_rawXferBuffer, dwSectorSize);
      return dwRead;
    }
  }

  OutputDebugString("CD Read error\n");
  return -1;
}


INT CIoSupport::ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
#ifdef HAS_OPTICAL_DRIVE
  DWORD dwBytesReturned;
  RAW_READ_INFO rawRead = {0};

  // Oddly enough, DiskOffset uses the Red Book sector size
  rawRead.DiskOffset.QuadPart = 2048 * dwSector;
  rawRead.SectorCount = 1;
  rawRead.TrackMode = XAForm2;

  for (int i = 0; i < 5; i++)
  {
    if ( DeviceIoControl( hDevice,
                          IOCTL_CDROM_RAW_READ,
                          &rawRead,
                          sizeof(RAW_READ_INFO),
                          m_rawXferBuffer,
                          RAW_SECTOR_SIZE,
                          &dwBytesReturned,
                          NULL ) != 0 )
    {
      memcpy(lpczBuffer, (char*)m_rawXferBuffer+MODE2_DATA_START, MODE2_DATA_SIZE);
      return MODE2_DATA_SIZE;
    }
    else
    {
      int iErr = GetLastError();
      //   printf("%i\n", iErr);
    }
  }
#endif
  return -1;
}

INT CIoSupport::ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
#ifdef HAS_OPTICAL_DRIVE
  DWORD dwBytesReturned;
  RAW_READ_INFO rawRead;

  // Oddly enough, DiskOffset uses the Red Book sector size
  rawRead.DiskOffset.QuadPart = 2048 * dwSector;
  rawRead.SectorCount = 1;
  rawRead.TrackMode = CDDA;

  for (int i = 0; i < 5; i++)
  {
    if ( DeviceIoControl( hDevice,
                          IOCTL_CDROM_RAW_READ,
                          &rawRead,
                          sizeof(RAW_READ_INFO),
                          m_rawXferBuffer,
                          sizeof(RAW_SECTOR_SIZE),
                          &dwBytesReturned,
                          NULL ) != 0 )
    {
      memcpy(lpczBuffer, m_rawXferBuffer, RAW_SECTOR_SIZE);
      return RAW_SECTOR_SIZE;
    }
  }
#endif
  return -1;
}

VOID CIoSupport::CloseCDROM(HANDLE hDevice)
{
  CloseHandle(hDevice);
}

// returns true if this is a debug machine
BOOL CIoSupport::IsDebug()
{
#ifdef _XBOX
  return (XboxKrnlVersion->Qfe & 0x8000) || ((DWORD)XboxHardwareInfo & 0x10);
#else
  return FALSE;
#endif
}


VOID CIoSupport::GetXbePath(char* szDest)
{
#ifdef _XBOX
  char devicePath[MAX_PATH];
  unsigned int length = XeImageFileName->Length;
  if (length >= sizeof(devicePath))
    length = sizeof(devicePath) - 1;
  memcpy(devicePath, XeImageFileName->Buffer, length);
  devicePath[length] = 0;

  const char* device = devicePath;
  if (strnicmp(device, "\\Device\\", 8) == 0)
    device += 8;
  char drive[7] = {0};
  GetDriveName(device, drive);
  if (!drive[0])
  {
    *szDest = 0;
    return;
  }
  char partition[MAX_PATH];
  GetPartition(drive, partition);
  sprintf(szDest, "%s:%s", drive, device + strlen(partition));

#else
  GetCurrentDirectory(XBMC_MAX_PATH, szDest);
  strcat(szDest, "\\XBMC_PC.exe");
#endif
}

bool CIoSupport::PartitionExists(int nPartition, int nDisk)
{
#ifdef _XBOX
  char szPartition[32];
  ANSI_STRING part_string;
  NTSTATUS status;
  HANDLE hTemp;
  OBJECT_ATTRIBUTES oa;
  IO_STATUS_BLOCK iosb;

  if (nDisk < 0 || nDisk > 1 || nPartition < 0 || nPartition > MAX_PARTITIONS)
    return false;
  sprintf(szPartition, "\\Device\\Harddisk%u\\Partition%u", nDisk, nPartition);
  RtlInitAnsiString(&part_string, szPartition);

  oa.Attributes = OBJ_CASE_INSENSITIVE;
  oa.ObjectName = &part_string;
  oa.RootDirectory = 0;

  status = NtOpenFile(&hTemp, GENERIC_READ | GENERIC_WRITE, &oa, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);

  if (NT_SUCCESS(status))
  {
    CloseHandle(hTemp);
    return true;
  }

  return false;
#else
  return false;
#endif
}

LARGE_INTEGER CIoSupport::GetDriveSize(int nDisk)
{
  LARGE_INTEGER drive_size;
#ifdef _XBOX
  HANDLE hDevice;
  char szHardDrive[32];
  drive_size.QuadPart = 0;
  if (nDisk < 0 || nDisk > 1)
    return drive_size;
  sprintf(szHardDrive, "\\Device\\Harddisk%u\\Partition0", nDisk);
  ANSI_STRING hd_string;
  OBJECT_ATTRIBUTES oa;
  IO_STATUS_BLOCK iosb;
  DISK_GEOMETRY disk_geometry;
  NTSTATUS status;

  RtlInitAnsiString(&hd_string, szHardDrive);
  drive_size.QuadPart = 0;

  oa.Attributes = OBJ_CASE_INSENSITIVE;
  oa.ObjectName = &hd_string;
  oa.RootDirectory = 0;

  status = NtOpenFile(&hDevice, GENERIC_READ | GENERIC_WRITE, &oa, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);
  if (!NT_SUCCESS(status))
    return drive_size;

  status = NtDeviceIoControlFile(hDevice, NULL, NULL, NULL, &iosb,
        IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &disk_geometry, sizeof(disk_geometry));

  CloseHandle(hDevice);

  if (!NT_SUCCESS(status))
    return drive_size;

  drive_size.QuadPart = disk_geometry.BytesPerSector *
                        disk_geometry.SectorsPerTrack *
                        disk_geometry.TracksPerCylinder *
                        disk_geometry.Cylinders.QuadPart;

  return drive_size;
#endif
  drive_size.QuadPart = 0;
  return drive_size;
}

bool CIoSupport::ReadPartitionTable()
{
#ifdef _XBOX
  unsigned int retval;

  retval = ReadPartitionTable(&m_partitionTable);
  if (retval == STATUS_SUCCESS)
  {
    m_fPartitionTableIsValid = true;
    return true;
  }
  else
  {
    m_fPartitionTableIsValid = false;
    return false;
  }
#else
  return false;
#endif
}

bool CIoSupport::HasPartitionTable()
{
  return m_fPartitionTableIsValid;
}

unsigned int CIoSupport::ReadPartitionTable(PARTITION_TABLE *p_table)
{
#ifdef _XBOX
  ANSI_STRING a_file;
  OBJECT_ATTRIBUTES obj_attr;
  IO_STATUS_BLOCK io_stat_block;
  HANDLE handle;
  unsigned int stat;
  unsigned int ioctl_cmd_in_buf[100];
  unsigned int ioctl_cmd_out_buf[100];
  unsigned int partition_table_addr;

  memset(p_table, 0, sizeof(PARTITION_TABLE));

  RtlInitAnsiString(&a_file, "\\Device\\Harddisk0\\partition0");
  obj_attr.RootDirectory = 0;
  obj_attr.ObjectName = &a_file;
  obj_attr.Attributes = OBJ_CASE_INSENSITIVE;

  stat = NtOpenFile(&handle, (GENERIC_READ | 0x00100000), &obj_attr, &io_stat_block, (FILE_SHARE_READ | FILE_SHARE_WRITE), 0x10);

  if (stat != STATUS_SUCCESS)
  {
    return stat;
  }

  memset(ioctl_cmd_out_buf, 0, sizeof(ioctl_cmd_out_buf));
  memset(ioctl_cmd_in_buf, 0, sizeof(ioctl_cmd_in_buf));
  ioctl_cmd_in_buf[0] = IOCTL_SUBCMD_GET_INFO;


  stat = NtDeviceIoControlFile(handle, 0, 0, 0, &io_stat_block,
                               IOCTL_CMD_LBA48_ACCESS,
                               ioctl_cmd_in_buf, sizeof(ioctl_cmd_in_buf),
                               ioctl_cmd_out_buf, sizeof(ioctl_cmd_out_buf));

  NtClose(handle);
  if (stat != STATUS_SUCCESS)
  {
    return stat;
  }

  if ((ioctl_cmd_out_buf[LBA48_GET_INFO_MAGIC1_IDX] != LBA48_GET_INFO_MAGIC1_VAL) ||
      (ioctl_cmd_out_buf[LBA48_GET_INFO_MAGIC2_IDX] != LBA48_GET_INFO_MAGIC2_VAL))
  {

    return STATUS_UNSUCCESSFUL;
  }

  partition_table_addr = ioctl_cmd_out_buf[LBA48_GET_INFO_LOWCODE_BASE_IDX];
  partition_table_addr += ioctl_cmd_out_buf[LBA48_GET_INFO_PART_TABLE_OFS_IDX];

  memcpy(p_table, (void *)partition_table_addr, sizeof(PARTITION_TABLE));

  return STATUS_SUCCESS;
#else
  return (unsigned int) -1;
#endif
}
