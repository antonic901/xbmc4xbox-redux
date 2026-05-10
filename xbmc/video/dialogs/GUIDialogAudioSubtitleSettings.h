#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

struct IntegerSettingOption;

class CGUIDialogAudioSubtitleSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogAudioSubtitleSettings();
  virtual ~CGUIDialogAudioSubtitleSettings();

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

  void AddAudioStreams(const boost::shared_ptr<CSettingGroup>& group, const std::string &settingId);
  void AddSubtitleStreams(const boost::shared_ptr<CSettingGroup>& group, const std::string &settingId);

  static void AudioStreamsOptionFiller(const boost::shared_ptr<const CSetting>& setting,
                                       std::vector<IntegerSettingOption>& list,
                                       int& current,
                                       void* data);
  static void SubtitleStreamsOptionFiller(const boost::shared_ptr<const CSetting>& setting,
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

  float m_volume;
  int m_audioStream;
  bool m_audioStreamStereoMode;
  int m_outputmode;
  int m_subtitleStream;
  bool m_subtitleVisible;
};
