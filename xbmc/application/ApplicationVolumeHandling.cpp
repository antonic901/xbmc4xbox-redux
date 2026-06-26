/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationVolumeHandling.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogVolumeBar.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"

#include <tinyxml/tinyxml.h>

float CApplicationVolumeHandling::GetVolumePercent() const
{
  // converts the hardware volume to a percentage
  return (m_volumeLevel - VOLUME_MINIMUM) * 100.0f / (VOLUME_MAXIMUM - VOLUME_MINIMUM);
}

int CApplicationVolumeHandling::GetVolumeRatio() const
{
  return m_volumeLevel;
}

void CApplicationVolumeHandling::SetHardwareVolume(int hardwareVolume)
{
  m_volumeLevel = hardwareVolume;
  if (m_volumeLevel > VOLUME_MAXIMUM)
  {
    m_volumeLevel = VOLUME_MAXIMUM;
  }
  else if (m_volumeLevel < VOLUME_MINIMUM)
  {
    m_volumeLevel = VOLUME_MINIMUM;
  }
}

void CApplicationVolumeHandling::VolumeChanged()
{
  CVariant data(CVariant::VariantTypeObject);
  data["volume"] = static_cast<int>(GetVolumePercent() + 0.5f);
  data["muted"] = m_muted;
  const boost::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> announcementMgr = CServiceBroker::GetAnnouncementManager();
  announcementMgr->Announce(ANNOUNCEMENT::Application, "xbmc", "OnVolumeChanged", data);

  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
  // if player has volume control, set it.
  if (appPlayer)
  {
    appPlayer->SetVolume(m_volumeLevel);
    appPlayer->SetMute(m_muted);
  }
}

void CApplicationVolumeHandling::ShowVolumeBar(const CAction* action)
{
  const CGUIWindowManager &wm = CServiceBroker::GetGUI()->GetWindowManager();
  CGUIDialogVolumeBar *volumeBar = wm.GetWindow<CGUIDialogVolumeBar>(WINDOW_DIALOG_VOLUME_BAR);
  if (volumeBar != NULL)
  {
    volumeBar->Open();
    if (action)
      volumeBar->OnAction(*action);
  }
}

bool CApplicationVolumeHandling::IsMuted() const
{
  return m_muted;
}

void CApplicationVolumeHandling::ToggleMute(void)
{
  if (m_muted)
    UnMute();
  else
    Mute();
}

void CApplicationVolumeHandling::SetMute(bool mute)
{
  if (m_muted != mute)
  {
    ToggleMute();
    m_muted = mute;
  }
}

void CApplicationVolumeHandling::Mute()
{
  m_muted = true;
  VolumeChanged();
}

void CApplicationVolumeHandling::UnMute()
{
  m_muted = false;
  VolumeChanged();
}

void CApplicationVolumeHandling::SetVolume(int iValue, bool isPercentage)
{
  int hardwareVolume = iValue;

  if (isPercentage)
    hardwareVolume = static_cast<int>(iValue * 0.01f * (VOLUME_MAXIMUM - VOLUME_MINIMUM) + VOLUME_MINIMUM);

  SetHardwareVolume(hardwareVolume);
  VolumeChanged();
}

void CApplicationVolumeHandling::CacheReplayGainSettings(const CSettings& settings)
{
  // initialize m_replayGainSettings
  m_replayGainSettings.iType = settings.GetInt(CSettings::SETTING_MUSICPLAYER_REPLAYGAINTYPE);
  m_replayGainSettings.iPreAmp = settings.GetInt(CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP);
  m_replayGainSettings.iNoGainPreAmp =
      settings.GetInt(CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP);
  m_replayGainSettings.bAvoidClipping =
      settings.GetBool(CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING);
}

bool CApplicationVolumeHandling::Load(const TiXmlNode* settings)
{
  if (!settings)
    return false;

  const TiXmlElement* audioElement = settings->FirstChildElement("audio");
  if (audioElement)
  {
    XMLUtils::GetBoolean(audioElement, "mute", m_muted);
    if (!XMLUtils::GetInt(audioElement, "volumelevel", m_volumeLevel, VOLUME_MINIMUM,
                            VOLUME_MAXIMUM))
      m_volumeLevel = VOLUME_MAXIMUM;
  }

  return true;
}

bool CApplicationVolumeHandling::Save(TiXmlNode* settings) const
{
  if (!settings)
    return false;

  TiXmlElement volumeNode("audio");
  TiXmlNode* audioNode = settings->InsertEndChild(volumeNode);
  if (!audioNode)
    return false;

  XMLUtils::SetBoolean(audioNode, "mute", m_muted);
  XMLUtils::SetInt(audioNode, "volumelevel", m_volumeLevel);

  return true;
}

bool CApplicationVolumeHandling::OnSettingChanged(const CSetting& setting)
{
  const std::string& settingId = setting.GetId();

  if (StringUtils::EqualsNoCase(settingId, CSettings::SETTING_MUSICPLAYER_REPLAYGAINTYPE))
    m_replayGainSettings.iType = static_cast<const CSettingInt&>(setting).GetValue();
  else if (StringUtils::EqualsNoCase(settingId, CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP))
    m_replayGainSettings.iPreAmp = static_cast<const CSettingInt&>(setting).GetValue();
  else if (StringUtils::EqualsNoCase(settingId,
                                     CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP))
    m_replayGainSettings.iNoGainPreAmp = static_cast<const CSettingInt&>(setting).GetValue();
  else if (StringUtils::EqualsNoCase(settingId,
                                     CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING))
    m_replayGainSettings.bAvoidClipping = static_cast<const CSettingBool&>(setting).GetValue();
  else
    return false;

  return true;
}
