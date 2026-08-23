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

class CGUIDialogSubtitleSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogSubtitleSettings();
  virtual ~CGUIDialogSubtitleSettings();
  virtual bool OnMessage(CGUIMessage& message);

  // specialization of CGUIWindow
  virtual void FrameMove();

  static std::string BrowseForSubtitle();

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

private:
  void AddSubtitleStreams(const boost::shared_ptr<CSettingGroup>& group,
                          const std::string& settingId);

  int m_subtitleStream;
  bool m_subtitleVisible;
  boost::shared_ptr<CSettingInt> m_subtitleStreamSetting;

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
};
