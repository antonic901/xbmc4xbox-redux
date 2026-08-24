/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

#include <string>
#include <utility>
#include <vector>

class CVariant;
struct IntegerSettingOption;

class CGUIDialogAudioSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogAudioSettings();
  virtual ~CGUIDialogAudioSettings();

  // specialization of CGUIWindow
  virtual void FrameMove();

  static std::string FormatDelay(float value, float interval);
  static std::string FormatDecibel(float value);
  static std::string FormatPercentAsDecibel(float value);

protected:
  // implementations of ISettingCallback
  virtual void OnSettingChanged(const boost::shared_ptr<const CSetting>& setting);
  virtual void OnSettingAction(const boost::shared_ptr<const CSetting>& setting);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual bool Save();
  virtual void SetupView();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

  void AddAudioStreams(const boost::shared_ptr<CSettingGroup>& group, const std::string& settingId);

  static void AudioStreamsOptionFiller(const boost::shared_ptr<const CSetting>& setting,
                                       std::vector<IntegerSettingOption>& list,
                                       int& current,
                                       void* data);

  static std::string SettingFormatterDelay(
      const boost::shared_ptr<const CSettingControlSlider>& control,
      const CVariant& value,
      const CVariant& minimum,
      const CVariant& step,
      const CVariant& maximum);
  static std::string SettingFormatterPercentAsDecibel(
      const boost::shared_ptr<const CSettingControlSlider>& control,
      const CVariant& value,
      const CVariant& minimum,
      const CVariant& step,
      const CVariant& maximum);

  int m_volume;
  int m_audioStream;
  bool m_passthrough;
};
