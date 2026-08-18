/*
 *  Copyright (C) 2005-2018 Nikola Antonic
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationXbox.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "addons/Skin.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlFactory.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUITextLayout.h"
#include "guilib/GUIWindowManager.h"
#include "input/ButtonTranslator.h"
#include "karaoke/CdgParser.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "threads/SystemClock.h"
#include "utils/JobManager.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/Updater.h"
#include "utils/XMLUtils.h"
#include "video/dialogs/GUIDialogVideoOSD.h"
#include "windowing/GraphicContext.h"

#include "platform/xbox/XKHDD.h"
#include "platform/xbox/lcd/LCD.h"
#include "platform/xbox/lcd/LCDFactory.h"
#include "platform/xbox/utils/LED.h"
#include "platform/xbox/utils/FanController.h"
#include "platform/xbox/utils/Trainer.h"

#include <xtl.h>

#define AAM_FAST 0
#define AAM_QUIET 1

#define APM_HIPOWER 0
#define APM_LOPOWER 1
#define APM_HIPOWER_STANDBY 2
#define APM_LOPOWER_STANDBY 3

#define LED_PLAYBACK_OFF 0
#define LED_PLAYBACK_VIDEO 1
#define LED_PLAYBACK_MUSIC 2
#define LED_PLAYBACK_VIDEO_MUSIC 3

#define SPIN_DOWN_NONE  0
#define SPIN_DOWN_MUSIC 1
#define SPIN_DOWN_VIDEO 2
#define SPIN_DOWN_BOTH  3

CApplicationXbox::CApplicationXbox()
{
  XSetProcessQuantumLength(5); // default is 20ms
  XSetFileCacheSize(256 * 1024); // default is 64KB

  m_DetectDVDType.Create(false, THREAD_MINSTACKSIZE);

  MEMORYSTATUS stat;
  GlobalMemoryStatus(&stat);
  m_hasMemoryUpgrade = stat.dwTotalPhys > 67108864;

  m_bSpinDown = false;
  m_bNetworkSpinDown = false;
  m_dwSpinDownTime = XbmcThreads::SystemClockMillis();

  m_pCdgParser = NULL;
  g_lcd = NULL;
  m_debugLayout = NULL;
}

bool CApplicationXbox::Load(const TiXmlNode* settings)
{
  if (!settings)
    return false;

  const TiXmlElement* audioElement = settings->FirstChildElement("audio");
  if (audioElement)
  {
    for (int i = 0; i < 4; i++)
    {
      std::string setting = StringUtils::Format("karaoke%i", i);
      if(!XMLUtils::GetFloat(audioElement, (setting + "energy").c_str(), m_karaokeVoiceMask[i].energy, XVOICE_MASK_PARAM_DISABLED, 1.0f))
        m_karaokeVoiceMask[i].energy = XVOICE_MASK_PARAM_DISABLED;
      if(!XMLUtils::GetFloat(audioElement, (setting + "pitch").c_str(), m_karaokeVoiceMask[i].pitch, XVOICE_MASK_PARAM_DISABLED, 1.0f))
        m_karaokeVoiceMask[i].pitch = XVOICE_MASK_PARAM_DISABLED;
      if(!XMLUtils::GetFloat(audioElement, (setting + "whisper").c_str(), m_karaokeVoiceMask[i].whisper, XVOICE_MASK_PARAM_DISABLED, 1.0f))
        m_karaokeVoiceMask[i].whisper = XVOICE_MASK_PARAM_DISABLED;
      if(!XMLUtils::GetFloat(audioElement, (setting + "robotic").c_str(), m_karaokeVoiceMask[i].robotic, XVOICE_MASK_PARAM_DISABLED, 1.0f))
        m_karaokeVoiceMask[i].robotic = XVOICE_MASK_PARAM_DISABLED;
    }
  }

  return true;
}

bool CApplicationXbox::Save(TiXmlNode* settings) const
{
  if (!settings)
    return false;

  TiXmlElement volumeNode("audio");
  TiXmlNode* audioNode = settings->InsertEndChild(volumeNode);
  if (!audioNode)
    return false;

  for (int i = 0; i < 4; i++)
  {
    std::string setting = StringUtils::Format("karaoke%i", i);
    XMLUtils::SetFloat(audioNode, (setting + "energy").c_str(), m_karaokeVoiceMask[i].energy);
    XMLUtils::SetFloat(audioNode, (setting + "pitch").c_str(), m_karaokeVoiceMask[i].pitch);
    XMLUtils::SetFloat(audioNode, (setting + "whisper").c_str(), m_karaokeVoiceMask[i].whisper);
    XMLUtils::SetFloat(audioNode, (setting + "robotic").c_str(), m_karaokeVoiceMask[i].robotic);
  }

  return true;
}

bool CApplicationXbox::OnSettingChanged(const CSetting& setting)
{
  const std::string& settingId = setting.GetId();

  if (settingId == CSettings::SETTING_KARAOKE_ENABLED)
  {
    if (HasKaraoke())
    {
      m_pCdgParser->Stop();
      delete m_pCdgParser;
      m_pCdgParser = NULL;
    }
    else
      m_pCdgParser = new CCdgParser();
  }
  else if (settingId == CSettings::SETTING_KARAOKE_PORT_ONE_VOICEMASK)
    CCdgParser::FillInVoiceMaskValues(0, static_cast<const CSettingString&>(setting).GetValue());
  else if (settingId == CSettings::SETTING_KARAOKE_PORT_TWO_VOICEMASK)
    CCdgParser::FillInVoiceMaskValues(1, static_cast<const CSettingString&>(setting).GetValue());
  else if (settingId == CSettings::SETTING_KARAOKE_PORT_THREE_VOICEMASK)
    CCdgParser::FillInVoiceMaskValues(2, static_cast<const CSettingString&>(setting).GetValue());
  else if (settingId == CSettings::SETTING_KARAOKE_PORT_FOUR_VOICEMASK)
    CCdgParser::FillInVoiceMaskValues(3, static_cast<const CSettingString&>(setting).GetValue());
  else if (settingId == CSettings::SETTING_LCD_TYPE)
  {
    if (HasLCD() && static_cast<const CSettingInt&>(setting).GetValue() == LCD_TYPE_NONE)
    {
      g_lcd->Stop();
      delete g_lcd;
      g_lcd = NULL;
    }
    if (HasLCD())
      g_lcd->Initialize();
  }
  else if (settingId == CSettings::SETTING_LCD_MODCHIP)
  {
    if (HasLCD())
    {
      g_lcd->Stop();
      delete g_lcd;
      g_lcd = NULL;
    }

    int value = static_cast<const CSettingInt&>(setting).GetValue();
    g_lcd = CLCDFactory::Create(static_cast<LCD_MODCHIP>(value));
    if (g_lcd)
      g_lcd->Initialize();
  }
  else if (settingId == CSettings::SETTING_LCD_BACKLIGHT)
  {
    if (HasLCD())
      g_lcd->SetBackLight(static_cast<const CSettingInt&>(setting).GetValue());
  }
  else if (settingId == CSettings::SETTING_LCD_CONTRAST)
  {
    if (HasLCD())
      g_lcd->SetContrast(static_cast<const CSettingInt&>(setting).GetValue());
  }
  else if (settingId == CSettings::SETTING_HARDDISK_AAMLEVEL)
  {
    if (static_cast<const CSettingInt&>(setting).GetValue() == AAM_QUIET)
      XKHDD::SetAAMLevel(0x80);
    else if (static_cast<const CSettingInt&>(setting).GetValue() == AAM_FAST)
      XKHDD::SetAAMLevel(0xFE);
  }
  else if (settingId == CSettings::SETTING_HARDDISK_APMLEVEL)
  {
    switch (static_cast<const CSettingInt&>(setting).GetValue())
    {
      case APM_LOPOWER:
        XKHDD::SetAPMLevel(0x80);
        break;
      case APM_HIPOWER:
        XKHDD::SetAPMLevel(0xFE);
        break;
      case APM_LOPOWER_STANDBY:
        XKHDD::SetAPMLevel(0x01);
        break;
      case APM_HIPOWER_STANDBY:
        XKHDD::SetAPMLevel(0x7F);
        break;
    }
  }
  else if (settingId == CSettings::SETTING_XBOX_LED_COLOUR)
  {
    int iData = static_cast<const CSettingInt&>(setting).GetValue();
    if (iData == LED_COLOUR_NO_CHANGE)
      // LED_COLOUR_NO_CHANGE: to prevent "led off" on colour immediately change, set to default green!
      //                       (we have no previos reference LED COLOUR, to set the LED colour back)
      //                       on next boot the colour will not changed and the default BIOS led colour will used
      ILED::CLEDControl(LED_COLOUR_GREEN);
    else
      ILED::CLEDControl(iData);
  }
  else if (settingId == CSettings::SETTING_TRAINER_SCAN)
    CTrainer::ScanTrainers();
  else if (settingId == CSettings::SETTING_UPDATER_CHECK)
    CServiceBroker::GetJobManager()->AddJob(new CUpdaterJob(false, true), NULL, CJob::PRIORITY_HIGH);
  else if (settingId == CSettings::SETTING_NETWORK_ASSIGNMENT || settingId == CSettings::SETTING_NETWORK_IPADDRESS ||
           settingId == CSettings::SETTING_NETWORK_SUBNET || settingId == CSettings::SETTING_NETWORK_GATEWAY ||
           settingId == CSettings::SETTING_NETWORK_DNS || settingId == CSettings::SETTING_NETWORK_DNS2)
  {
    CServiceBroker::GetNetwork().NetworkMessage(CNetwork::SERVICES_DOWN, 1);
    CServiceBroker::GetNetwork().SetupNetwork();
  }
  else
    return false;

  return true;
}

bool CApplicationXbox::OnSettingAction(const CSetting& setting)
{
  const std::string& settingId = setting.GetId();

  if (settingId == CSettings::SETTING_KARAOKE_EXPORT)
  {
    // TODO: implement this
    return false;
  }
  else if (settingId == CSettings::SETTING_KARAOKE_IMPORT)
  {
    // TODO: implement this
    return false;
  }
  else
    return false;

  return true;
}

void CApplicationXbox::OnCreate()
{
  bool bNeedReboot = false;
  if (CTrainer::RemoveTrainer())
    bNeedReboot = true;

  F_VIDEO ForceVideo = VIDEO_NULL;
  F_COUNTRY ForceCountry = COUNTRY_NULL;
  // TODO: add region switching

  if (bNeedReboot)
  {
    CUtil::LaunchXbe("special://xbmcbin/", "default.xbe", NULL, ForceVideo, ForceCountry);
  }
}

float CApplicationXbox::GetCPUUsage()
{
  return 100.0f - m_idleThread.GetRelativeUsage() * 100.0f;
}

bool CApplicationXbox::HasMemoryUpgrade() const
{
  return m_hasMemoryUpgrade;
}

bool CApplicationXbox::MustBlockHDSpinDown(bool bCheckThisForNormalSpinDown)
{
  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
  if (!appPlayer->IsPlayingVideo())
  {
    return false;
  }
  // block immediate spindown when playing a video non-fullscreen (videocontrol is playing)
  if (!bCheckThisForNormalSpinDown && !CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo())
  {
    return true;
  }
  // allow normal hd spindown always if the movie is paused
  if (bCheckThisForNormalSpinDown && appPlayer->IsPaused())
  {
    return false;
  }
  // don't allow hd spindown when playing files with vobsub subtitles.
  std::string strSubTitelExtension;
  if (appPlayer->GetSubtitleExtension(strSubTitelExtension))
  {
    return strSubTitelExtension == ".idx";
  }
  return false;
}

void CApplicationXbox::CheckNetworkHDSpinDown(bool playbackStarted)
{
  int iSpinDown = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_HDD_REMOTE_PLAY_SPINDOWN);
  if (iSpinDown == SPIN_DOWN_NONE || CServiceBroker::GetGUI()->GetWindowManager().HasModalDialog(true) || MustBlockHDSpinDown(false))
    return ;

  const CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();

  if (!m_bNetworkSpinDown || playbackStarted)
  {
    int iDuration = 0;

    // try to get duration from current tag because mplayer doesn't calculate vbr mp3 correctly
    if (appPlayer->IsPlayingAudio() && g_application.CurrentFileItem().HasMusicInfoTag())
    {
      iDuration = g_application.CurrentFileItem().GetMusicInfoTag()->GetDuration();
    }

    if (appPlayer->IsPlaying() && iDuration <= 0)
    {
      iDuration = static_cast<int>(g_application.GetTotalTime());
    }

    // spin down harddisk when the current file being played is not on local harddrive and
    // duration is more then spindown timeoutsetting or duration is unknown (streams)
    if (!g_application.CurrentFileItem().IsHD() &&
      (
        (iSpinDown == SPIN_DOWN_VIDEO && appPlayer->IsPlayingVideo()) ||
        (iSpinDown == SPIN_DOWN_MUSIC && appPlayer->IsPlayingAudio()) ||
        (iSpinDown == SPIN_DOWN_BOTH && (appPlayer->IsPlayingVideo() || appPlayer->IsPlayingAudio()))
      ) &&
      (
        iDuration <= 0 ||
        (iDuration > CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_HDD_REMOTE_PLAY_SPINDOWN_DURATION) * 60)
      ))
    {
      m_bNetworkSpinDown = true;
      if (!playbackStarted)
      {
        // if we got here not because of a playback start check what screen we are in
        // get the current active window
        int iWin = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
        if (iWin == WINDOW_FULLSCREEN_VIDEO)
        {
          // check if OSD is visible, if so don't do immediate spindown
          CGUIDialogVideoOSD *pOSD = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoOSD>(WINDOW_DIALOG_VIDEO_OSD);
          if (pOSD)
            m_bNetworkSpinDown = !pOSD->IsDialogRunning();
        }
      }
      if (m_bNetworkSpinDown)
      {
        // do the spindown right now + delayseconds
        m_dwSpinDownTime = XbmcThreads::SystemClockMillis();
      }
    }
  }
  if (m_bNetworkSpinDown)
  {
    // check the elapsed time
    unsigned int dwTimeSpan = XbmcThreads::SystemClockMillis() - m_dwSpinDownTime;
    if (m_dwSpinDownTime != 0 && (dwTimeSpan >= (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_HDD_REMOTE_PLAY_SPINDOWN_DELAY) * 1000UL)))
    {
      // time has elapsed, spin it down
      XKHDD::SpindownHarddisk();
      // stop checking until a key is pressed.
      m_dwSpinDownTime = 0;
      m_bNetworkSpinDown = true;
    }
    else if (m_dwSpinDownTime == 0 && appPlayer->IsPlaying())
    {
      // we are currently spun down - let's spin back up again if we are playing media
      // and we're within 10 seconds (or 0.5*spindown time) of the end.  This should
      // make returning to the GUI a bit snappier + speed up stacked item changes.
      int iMinSpinUp = 10;
      if (iMinSpinUp > CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_HDD_REMOTE_PLAY_SPINDOWN_DELAY) * 0.5f)
        iMinSpinUp = static_cast<int>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_HDD_REMOTE_PLAY_SPINDOWN_DELAY) * 0.5f);
      if ((MathUtils::round_int(g_application.GetTotalTime()) - MathUtils::round_int(g_application.GetTime())) == iMinSpinUp)
      {
        // spin back up
        XKHDD::SpindownHarddisk(false);
      }
    }
  }
}

bool CApplicationXbox::IsKaraokeRunning()
{
  if (HasKaraoke())
    return m_pCdgParser->IsRunning();

  return false;
}

void CApplicationXbox::StartKaraoke(const boost::shared_ptr<CFileItem>& pItem)
{
  const CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
  if (HasKaraoke() && appPlayer->IsPlayingAudio())
  {
    m_pCdgParser->Stop();
    if (pItem->IsMusicDb())
    {
      if (!pItem->HasMusicInfoTag() || !pItem->GetMusicInfoTag()->Loaded())
      {
        MUSIC_INFO::IMusicInfoTagLoader* tagloader = MUSIC_INFO::CMusicInfoTagLoaderFactory::CreateLoader(*pItem);
        tagloader->Load(pItem->GetPath(), *pItem->GetMusicInfoTag());
        delete tagloader;
      }
      m_pCdgParser->Start(pItem->GetMusicInfoTag()->GetURL());
    }
    else
      m_pCdgParser->Start(pItem->GetPath());
  }
}

void CApplicationXbox::StopKaraoke()
{
  if (HasKaraoke())
  {
    m_pCdgParser->Stop();
    m_pCdgParser->Free();
  }
}

void CApplicationXbox::ProcessKaraoke()
{
  if (HasKaraoke())
    m_pCdgParser->ProcessVoice();
}

void CApplicationXbox::CheckHDSpindown()
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_HDD_SPINDOWN_TIME) || CServiceBroker::GetGUI()->GetWindowManager().HasModalDialog(true) || MustBlockHDSpinDown())
    return ;

  const CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();

  if (!m_bSpinDown && (!appPlayer->IsPlaying() || (appPlayer->IsPlaying() && !g_application.CurrentFileItem().IsHD())))
  {
    m_bSpinDown = true;
    // let networkspindown override normal spindown
    m_bNetworkSpinDown = false;
    m_dwSpinDownTime = XbmcThreads::SystemClockMillis();
  }

  // Can we do a spindown right now?
  if (m_bSpinDown)
  {
    // yes, then check the elapsed time
    unsigned int dwTimeSpan = XbmcThreads::SystemClockMillis() - m_dwSpinDownTime;
    if (m_dwSpinDownTime != 0 && (dwTimeSpan >= (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_HDD_SPINDOWN_TIME) * 60UL * 1000UL)))
    {
      // time has elapsed, spin it down
      XKHDD::SpindownHarddisk();
      //stop checking until a key is pressed.
      m_dwSpinDownTime = 0;
    }
  }
}

void CApplicationXbox::StartLEDControl(bool switchoff)
{
  const boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (switchoff && settings->GetInt(CSettings::SETTING_XBOX_LED_COLOUR) != LED_COLOUR_NO_CHANGE)
  {
    const CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
    if ((appPlayer->IsPlayingVideo() && settings->GetInt(CSettings::SETTING_XBOX_LED_DISABLE_ON_PLAYBACK) == LED_PLAYBACK_VIDEO) ||
        (appPlayer->IsPlayingAudio() && settings->GetInt(CSettings::SETTING_XBOX_LED_DISABLE_ON_PLAYBACK) == LED_PLAYBACK_MUSIC) ||
        ((appPlayer->IsPlayingVideo() || appPlayer->IsPlayingAudio()) && settings->GetInt(CSettings::SETTING_XBOX_LED_DISABLE_ON_PLAYBACK) == LED_PLAYBACK_VIDEO_MUSIC))
      ILED::CLEDControl(LED_COLOUR_OFF);
  }
  else if (!switchoff)
    ILED::CLEDControl(settings->GetInt(CSettings::SETTING_XBOX_LED_COLOUR));
}

void CApplicationXbox::DimLCDOnPlayback(bool dim)
{
  if (HasLCD())
  {
    const boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (dim && settings->GetInt(CSettings::SETTING_LCD_DISABLE_ON_PLAYBACK) != LED_PLAYBACK_OFF)
    {
      const CApplicationComponents &components = CServiceBroker::GetAppComponents();
      const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
      if ((appPlayer->IsPlayingVideo() && settings->GetInt(CSettings::SETTING_LCD_DISABLE_ON_PLAYBACK) == LED_PLAYBACK_VIDEO) ||
          (appPlayer->IsPlayingAudio() && settings->GetInt(CSettings::SETTING_LCD_DISABLE_ON_PLAYBACK) == LED_PLAYBACK_MUSIC) ||
          ((appPlayer->IsPlayingVideo() || appPlayer->IsPlayingAudio()) && settings->GetInt(CSettings::SETTING_LCD_DISABLE_ON_PLAYBACK) == LED_PLAYBACK_VIDEO_MUSIC))
        g_lcd->SetBackLight(0);
    }
    else if (!dim)
    {
      g_lcd->SetBackLight(settings->GetInt(CSettings::SETTING_LCD_BACKLIGHT));
    }
  }
}

void CApplicationXbox::SetLCDBacklight(int iValue)
{
  if (HasLCD())
  {
    g_lcd->SetBackLight(iValue);
  }
}

void CApplicationXbox::PrintXBETitleToLCD(const std::string& strXbePath)
{
  if (HasLCD())
  {
    std::string strXBETitle;
    if (!CUtil::GetXBEDescription(strXbePath, strXBETitle))
    {
      CUtil::GetDirectoryName(strXbePath, strXBETitle);
      CUtil::ShortenFileName(strXBETitle);
      CUtil::RemoveIllegalChars(strXBETitle);
    }

    // crop to LCD screen size
    int lcdRowSize = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_lcdColumns;
    if (strXBETitle.size() > lcdRowSize)
      strXBETitle = strXBETitle.substr(0, lcdRowSize);

    g_lcd->Render(ILCD::LCD_MODE_XBE_LAUNCH);
  }
}

void CApplicationXbox::UpdateLCD()
{
  static unsigned int lTickCount = 0;
  if (HasLCD())
  {
    const CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();

    unsigned int lTimeOut = 0;
    if (appPlayer->GetPlaySpeed() == 1)
      lTimeOut = 1000;

    if (XbmcThreads::SystemClockMillis() - lTickCount >= lTimeOut)
    {
      CApplicationComponents &components = CServiceBroker::GetAppComponents();
      const boost::shared_ptr<CApplicationPowerHandling> appPower = components.GetComponent<CApplicationPowerHandling>();
      if (appPower->NavigationIdleTime() < 5)
        g_lcd->Render(ILCD::LCD_MODE_NAVIGATION);
      else if (appPlayer->IsPlayingVideo())
        g_lcd->Render(ILCD::LCD_MODE_VIDEO);
      else if (appPlayer->IsPlayingAudio())
        g_lcd->Render(ILCD::LCD_MODE_MUSIC);
      else if (appPower->IsInScreenSaver())
        g_lcd->Render(ILCD::LCD_MODE_SCREENSAVER);
      else
        g_lcd->Render(ILCD::LCD_MODE_GENERAL);

      // reset tick count
      lTickCount = XbmcThreads::SystemClockMillis();
    }
  }
}

void CApplicationXbox::StartServices()
{
  // Create idle thread
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bPowerSave)
  {
    CLog::Log(LOGNOTICE, "Using idle thread with HLT (power saving)");
    m_idleThread.Create(false, 0x100);
  }
  else
  {
    CLog::Log(LOGNOTICE, "Not using idle thread with HLT (no power saving)");
  }

  // Set LED color of front panel
  StartLEDControl(false);

  // Initialize Fan Controller
  const boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetBool(CSettings::SETTING_XBOX_AUTO_TEMPERATURE))
  {
    CFanController::Instance()->Start(settings->GetInt(CSettings::SETTING_XBOX_TARGET_TEMPERATURE), settings->GetInt(CSettings::SETTING_XBOX_MIN_FANSPEED));
  }
  else if (settings->GetBool(CSettings::SETTING_XBOX_FANSPEED_CONTROL))
  {
    CFanController::Instance()->SetFanSpeed(settings->GetInt(CSettings::SETTING_XBOX_FANSPEED));
  }

  // Initialize LCD
  if (settings->GetInt(CSettings::SETTING_LCD_TYPE) != LCD_TYPE_NONE)
  {
    int modchip = settings->GetInt(CSettings::SETTING_LCD_MODCHIP);
    g_lcd = CLCDFactory::Create(static_cast<LCD_MODCHIP>(modchip));
    if (g_lcd != NULL)
    {
      g_lcd->Initialize();
    }
  }

  // Initialize Karaoke
  if (settings->GetBool(CSettings::SETTING_KARAOKE_ENABLED))
    m_pCdgParser = new CCdgParser();

  // Configure Advanced Power Management for HDD
  if (settings->GetInt(CSettings::SETTING_HARDDISK_AAMLEVEL) == AAM_QUIET)
    XKHDD::SetAAMLevel(0x80);
  else if (settings->GetInt(CSettings::SETTING_HARDDISK_AAMLEVEL) == AAM_FAST)
    XKHDD::SetAAMLevel(0xFE);

  switch (settings->GetInt(CSettings::SETTING_HARDDISK_APMLEVEL))
  {
    case APM_LOPOWER:
      XKHDD::SetAPMLevel(0x80);
      break;
    case APM_HIPOWER:
      XKHDD::SetAPMLevel(0xFE);
      break;
    case APM_LOPOWER_STANDBY:
      XKHDD::SetAPMLevel(0x01);
      break;
    case APM_HIPOWER_STANDBY:
      XKHDD::SetAPMLevel(0x7F);
      break;
  }
}

void CApplicationXbox::StopServices()
{
  m_DetectDVDType.StopThread();
  m_idleThread.StopThread();

  CFanController::Instance()->Stop();
  CFanController::RemoveInstance();

  if (g_lcd != NULL)
  {
    g_lcd->Stop();
    delete g_lcd;
    g_lcd = NULL;
  }
  if (m_pCdgParser != NULL)
  {
    delete m_pCdgParser;
    m_pCdgParser = NULL;
  }
}
