#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "md5.h"
#include "InfoLoader.h"
#include "settings/ISubSettings.h"

#include "platform/xbox/XKEEPROM.h"

#include <string>

#define KB  (1024)          // 1 KiloByte (1KB)   1024 Byte (2^10 Byte)
#define MB  (1024*KB)       // 1 MegaByte (1MB)   1024 KB (2^10 KB)
#define GB  (1024*MB)       // 1 GigaByte (1GB)   1024 MB (2^10 MB)
#define TB  (1024*GB)       // 1 TerraByte (1TB)  1024 GB (2^10 GB)

#define SMARTXX_LED_OFF        0 // SmartXX ModCHIP LED Controll
#define SMARTXX_LED_BLUE       1
#define SMARTXX_LED_RED        2
#define SMARTXX_LED_BLUE_RED   3
#define SMARTXX_LED_CYCLE      4

#define MAX_KNOWN_ATTRIBUTES  46

struct Bios
{
 char Name[50];
 char Signature[33];
};

class CSysData
{
public:
  CSysData()
  {
    Reset();
  };

  void Reset()
  {
#ifdef _XBOX
    xboxBios = "";
    xboxModChip = "";

    HDDModel = "";
    HDDSerial = "";
    HDDFirmware = "";
    HDDpw = "";
    HDDLockState = "";
    DVDModel = "";
    DVDFirmware = "";
#endif

    haveInternetState = false;
    internetState = "";
  };

  bool haveInternetState;
  std::string systemUptime;
  std::string systemTotalUptime;
  std::string internetState;
  std::string videoEncoder;
  std::string cpuFrequency;
  std::string kernelVersion;
  std::string macAddress;

#ifdef _XBOX
  // info specific to xbox
  std::string xboxBios;
  std::string xboxModChip;
  std::string mplayerversion;
  std::string xboxversion;
  std::string avpackinfo;
  std::string xboxserial;
  std::string hddlockkey;
  std::string hddbootdate;
  std::string hddcyclecount;
  std::string videoxberegion;
  std::string videodvdzone;
  std::string produceinfo;

  std::string HDDModel;
  std::string HDDSerial;
  std::string HDDFirmware;
  std::string HDDpw;
  std::string HDDLockState;
  signed char HDDTemp;

  std::string DVDModel;
  std::string DVDFirmware;
#endif
};

class CSysInfoJob : public CJob
{
public:
  CSysInfoJob();

  virtual bool DoWork();
  const CSysData &GetData() const;

private:
  bool SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays);
  double GetCPUFrequency();
  std::string GetInternetState();
  std::string GetSystemUpTime(bool bTotalUptime);
  std::string GetCPUFreqInfo();
  std::string GetMACAddress();
  std::string GetVideoEncoder();

  CSysData m_info;
};

class CSysInfo : public CInfoLoader, public ISubSettings
{
public:
  CSysInfo(void);
  virtual ~CSysInfo();

  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;

  char MD5_Sign[32 + 1];

  bool GetDVDInfo(std::string& strDVDModel, std::string& strDVDFirmware);
  bool GetHDDInfo(std::string& strHDDModel, std::string& strHDDSerial,std::string& strHDDFirmware,std::string& strHDDpw,std::string& strHDDLockState);
  static bool GetRefurbInfo(std::string& rfi_FirstBootTime, std::string& rfi_PowerCycleCount);

  bool CreateBiosBackup();
  bool CreateEEPROMBackup();
  void WriteTXTInfoFile();

#ifdef _XBOX
  static std::string SmartXXModCHIP();
  static std::string GetAVPackInfo();
  static std::string GetMPlayerVersion();
  std::string GetUnits(int iFrontPort);
  std::string GetXBOXSerial();
  std::string GetXBProduceInfo();
  std::string GetVideoXBERegion();
  std::string GetDVDZone();
  std::string GetXBLiveKey();
  std::string GetHDDKey();
  static std::string GetModChipInfo();
  std::string GetBIOSInfo();
  std::string GetTrayState();
#endif

  static std::string GetUserAgent();
  bool HasInternet() const;
  static std::string GetKernelVersion();
  static std::string GetXBVerInfo();
  bool GetDiskSpace(const std::string drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed);
  std::string GetHddSpaceInfo(int& percent, int drive, bool shortText=false);
  std::string GetHddSpaceInfo(int drive, bool shortText=false);

  int GetTotalUptime() const { return m_iSystemTimeTotalUp; }
  void SetTotalUptime(int uptime) { m_iSystemTimeTotalUp = uptime; }

#ifdef _XBOX
  bool m_bRequestDone;
  bool m_bSmartSupported;
  bool m_bSmartEnabled;

  bool m_hddRequest;
  bool m_dvdRequest;

  #define XBOX_BIOS_ID_INI_FILE "Q:\\System\\SystemInfo\\BiosIDs.ini"
  #define XBOX_BIOS_BACKUP_FILE "Q:\\System\\SystemInfo\\BIOSBackup.bin"
  #define XBOX_EEPROM_BIN_BACKUP_FILE "Q:\\System\\SystemInfo\\EEPROMBackup.bin"
  #define XBOX_EEPROM_CFG_BACKUP_FILE "Q:\\System\\SystemInfo\\EEPROMBackup.cfg"
  #define XBOX_XBMC_TXT_INFOFILE "Q:\\System\\SystemInfo\\XBMCSystemInfo.txt"
  #define SYSINFO_TMP_SIZE 256
  #define XDEVICE_TYPE_IR_REMOTE  (&XDEVICE_TYPE_IR_REMOTE_TABLE)
  #define DEBUG_KEYBOARD

  XKEEPROM* m_XKEEPROM;
  XBOX_VERSION  m_XBOXVersion;

  static double RDTSC(void);
  static bool GetXBOXVersionDetected(std::string& strXboxVer);
  static std::string GetModCHIPDetected();

  static struct Bios * LoadBiosSigns();
  bool CheckBios(std::string& strDetBiosNa);
  static char* ReturnBiosName(char *buffer, char *str);
  static char* ReturnBiosSign(char *buffer, char *str);
  char* CheckMD5 (struct Bios *Listone, char *Sign);
  char* MD5Buffer(char *filename,long PosizioneInizio,int KBytes);
  static std::string MD5BufferNew(char *filename,long PosizioneInizio,int KBytes);
#endif

protected:
  virtual CJob *GetJob() const;
  virtual std::string TranslateInfo(int info) const;
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

private:
  CSysData m_info;
  int m_iSystemTimeTotalUp; // Uptime in minutes!
  void Reset();
};

extern CSysInfo g_sysinfo;

