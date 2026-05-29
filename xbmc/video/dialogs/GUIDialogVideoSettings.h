/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Interface/StreamInfo.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

#include <string>
#include <utility>
#include <vector>

struct IntegerSettingOption;

class CGUIDialogVideoSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogVideoSettings();
  virtual ~CGUIDialogVideoSettings();

protected:
  // implementations of ISettingCallback
  virtual void OnSettingChanged(const boost::shared_ptr<const CSetting>& setting);
  virtual void OnSettingAction(const boost::shared_ptr<const CSetting>& setting);

  void AddVideoStreams(const boost::shared_ptr<CSettingGroup>& group, const std::string& settingId);
  static void VideoStreamsOptionFiller(const boost::shared_ptr<const CSetting>& setting,
                                       std::vector<IntegerSettingOption>& list,
                                       int& current,
                                       void* data);

  static void VideoOrientationFiller(const boost::shared_ptr<const CSetting>& setting,
                                     std::vector<IntegerSettingOption>& list,
                                     int& current,
                                     void* data);

  static std::string FormatFlags(StreamFlags flags);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual bool Save();
  virtual void SetupView();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

private:
  int m_videoStream;
  bool m_viewModeChanged = false;
};
