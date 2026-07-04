/*
 *  Copyright (C) 2005-2018 Nikola Antonic
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationXbox.h"

#include "CdgParser.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "threads/SystemClock.h"
#include "utils/LCD.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "video/dialogs/GUIDialogVideoOSD.h"
#include "windowing/GraphicContext.h"
#include "xbox/XKHDD.h"

#include <xtl.h>

#define SPIN_DOWN_NONE  0
#define SPIN_DOWN_MUSIC 1
#define SPIN_DOWN_VIDEO 2
#define SPIN_DOWN_BOTH  3

CApplicationXbox::CApplicationXbox()
{
  // TODO: add HLT (power saving)
  m_idleThread.Create(false, 0x100);

  MEMORYSTATUS stat;
  GlobalMemoryStatus(&stat);
  m_hasMemoryUpgrade = stat.dwTotalPhys > 67108864;

  m_bSpinDown = false;
  m_bNetworkSpinDown = false;
  m_dwSpinDownTime = XbmcThreads::SystemClockMillis();
  m_pCdgParser = new CCdgParser();
  // TODO: add support, make it possible to disable/enable etc.
  g_lcd = NULL;
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

  if (settingId == CSettings::SETTING_KARAOKE_PORT_ONE_VOICEMASK)
    CCdgParser::FillInVoiceMaskValues(0, static_cast<const CSettingString&>(setting).GetValue());
  else if (settingId == CSettings::SETTING_KARAOKE_PORT_TWO_VOICEMASK)
    CCdgParser::FillInVoiceMaskValues(1, static_cast<const CSettingString&>(setting).GetValue());
  else if (settingId == CSettings::SETTING_KARAOKE_PORT_THREE_VOICEMASK)
    CCdgParser::FillInVoiceMaskValues(2, static_cast<const CSettingString&>(setting).GetValue());
  else if (settingId == CSettings::SETTING_KARAOKE_PORT_FOUR_VOICEMASK)
    CCdgParser::FillInVoiceMaskValues(3, static_cast<const CSettingString&>(setting).GetValue());
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

float CApplicationXbox::GetCPUUsage()
{
  return 100.0f - m_idleThread.GetRelativeUsage() * 100.0f;
}

bool CApplicationXbox::HasMemoryUpgrade() const
{
  return m_hasMemoryUpgrade;
}

void CApplicationXbox::RenderMemoryStatus() const
{
  // TODO: implement this
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
