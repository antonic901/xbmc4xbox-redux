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
#include "xbox/XKEEPROM.h"
#include "InfoLoader.h"
#include "settings/lib/ISubSettings.h"

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
  CStdString systemUptime;
  CStdString systemTotalUptime;
  CStdString internetState;
  CStdString videoEncoder;
  CStdString cpuFrequency;
  CStdString kernelVersion;
  CStdString macAddress;

#ifdef _XBOX
  // info specific to xbox
  CStdString xboxBios;
  CStdString xboxModChip;
  CStdString mplayerversion;
  CStdString xboxversion;
  CStdString avpackinfo;
  CStdString xboxserial;
  CStdString hddlockkey;
  CStdString hddbootdate;
  CStdString hddcyclecount;
  CStdString videoxberegion;
  CStdString videodvdzone;
  CStdString produceinfo;

  CStdString HDDModel;
  CStdString HDDSerial;
  CStdString HDDFirmware;
  CStdString HDDpw;
  CStdString HDDLockState;
  signed char HDDTemp;

  CStdString DVDModel;
  CStdString DVDFirmware;
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
  CStdString GetInternetState();
  CStdString GetSystemUpTime(bool bTotalUptime);
  CStdString GetCPUFreqInfo();
  CStdString GetMACAddress();
  CStdString GetVideoEncoder();

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

  bool GetDVDInfo(CStdString& strDVDModel, CStdString& strDVDFirmware);
  bool GetHDDInfo(CStdString& strHDDModel, CStdString& strHDDSerial,CStdString& strHDDFirmware,CStdString& strHDDpw,CStdString& strHDDLockState);
  static bool GetRefurbInfo(CStdString& rfi_FirstBootTime, CStdString& rfi_PowerCycleCount);

  bool CreateBiosBackup();
  bool CreateEEPROMBackup();
  void WriteTXTInfoFile();

#ifdef _XBOX
  static CStdString SmartXXModCHIP();
  static CStdString GetAVPackInfo();
  static CStdString GetMPlayerVersion();
  CStdString GetUnits(int iFrontPort);
  CStdString GetXBOXSerial();
  CStdString GetXBProduceInfo();
  CStdString GetVideoXBERegion();
  CStdString GetDVDZone();
  CStdString GetXBLiveKey();
  CStdString GetHDDKey();
  static CStdString GetModChipInfo();
  CStdString GetBIOSInfo();
  CStdString GetTrayState();
#endif

  CStdString GetUserAgent();
  bool HasInternet() const;
  static CStdString GetKernelVersion();
  static CStdString GetXBVerInfo();
  bool GetDiskSpace(const CStdString drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed);
  CStdString GetHddSpaceInfo(int& percent, int drive, bool shortText=false);
  CStdString GetHddSpaceInfo(int drive, bool shortText=false);

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
  #define DEBUG_MOUSE

  XKEEPROM* m_XKEEPROM;
  XBOX_VERSION  m_XBOXVersion;

  static double RDTSC(void);
  static bool GetXBOXVersionDetected(CStdString& strXboxVer);
  static CStdString GetModCHIPDetected();

  static struct Bios * LoadBiosSigns();
  bool CheckBios(CStdString& strDetBiosNa);
  static char* ReturnBiosName(char *buffer, char *str);
  static char* ReturnBiosSign(char *buffer, char *str);
  char* CheckMD5 (struct Bios *Listone, char *Sign);
  char* MD5Buffer(char *filename,long PosizioneInizio,int KBytes);
  static CStdString MD5BufferNew(char *filename,long PosizioneInizio,int KBytes);
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

