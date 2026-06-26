/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/IApplicationComponent.h"

class CAction;
class CApplication;
class CSetting;
class CSettings;
class TiXmlNode;

/*!
 * \brief Class handling application support for audio volume management.
 */
class CApplicationVolumeHandling : public IApplicationComponent
{
  friend class CApplication;

public:
  CApplicationVolumeHandling() : m_muted(false), m_volumeLevel(VOLUME_MAXIMUM) {}

  // replay gain settings struct for quick access by the player multiple
  // times per second (saves doing settings lookup)
  struct ReplayGainSettings
  {
    int iPreAmp;
    int iNoGainPreAmp;
    int iType;
    bool bAvoidClipping;
  };

  float GetVolumePercent() const;
  float GetVolumeRatio() const;
  bool IsMuted() const;

  void SetVolume(float iValue, bool isPercentage = true);
  void SetMute(bool mute);
  void ToggleMute(void);

  const ReplayGainSettings& GetReplayGainSettings() const { return m_replayGainSettings; }

  static const int VOLUME_MINIMUM = -6000; // -60dB
  static const int VOLUME_MAXIMUM = 0; // 0dB

  bool Load(const TiXmlNode* settings);
  bool Save(TiXmlNode* settings) const;
  bool OnSettingChanged(const CSetting& setting);

protected:
  bool IsMutedInternal() const { return m_muted; }
  void ShowVolumeBar(const CAction* action = 0);

  void CacheReplayGainSettings(const CSettings& settings);

  void Mute();
  void UnMute();

  void SetHardwareVolume(float hardwareVolume);

  void VolumeChanged();

  bool m_muted;
  float m_volumeLevel;
  ReplayGainSettings m_replayGainSettings;
};
