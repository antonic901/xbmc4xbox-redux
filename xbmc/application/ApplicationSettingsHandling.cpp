/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationSettingsHandling.h"

#include "ServiceBroker.h"
#include "XBAudioConfig.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationSkinHandling.h"
#include "application/ApplicationVolumeHandling.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"

namespace
{
bool IsPlaying(const std::string& condition,
               const std::string& value,
               const SettingConstPtr& setting,
               void* data)
{
  return data ? static_cast<CApplicationPlayer*>(data)->IsPlaying() : false;
}
} // namespace

void CApplicationSettingsHandling::RegisterSettings()
{
  const boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  CSettingsManager* settingsMgr = settings->GetSettingsManager();

  settingsMgr->RegisterSettingsHandler(this);

  std::set<std::string> temp;
  temp.insert(CSettings::SETTING_AUDIOOUTPUT_AACPASSTHROUGH);
  temp.insert(CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH);
  temp.insert(CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH);
  temp.insert(CSettings::SETTING_AUDIOOUTPUT_MP1PASSTHROUGH);
  temp.insert(CSettings::SETTING_AUDIOOUTPUT_MP2PASSTHROUGH);
  temp.insert(CSettings::SETTING_AUDIOOUTPUT_MP3PASSTHROUGH);
  temp.insert(CSettings::SETTING_LOOKANDFEEL_SKIN);
  temp.insert(CSettings::SETTING_LOOKANDFEEL_SKINSETTINGS);
  temp.insert(CSettings::SETTING_LOOKANDFEEL_FONT);
  temp.insert(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
  temp.insert(CSettings::SETTING_LOOKANDFEEL_SKINCOLORS);
  temp.insert(CSettings::SETTING_LOOKANDFEEL_SKINZOOM);
  temp.insert(CSettings::SETTING_MUSICPLAYER_REPLAYGAINPREAMP);
  temp.insert(CSettings::SETTING_MUSICPLAYER_REPLAYGAINNOGAINPREAMP);
  temp.insert(CSettings::SETTING_MUSICPLAYER_REPLAYGAINTYPE);
  temp.insert(CSettings::SETTING_MUSICPLAYER_REPLAYGAINAVOIDCLIPPING);
  temp.insert(CSettings::SETTING_SCRAPERS_MUSICVIDEOSDEFAULT);
  temp.insert(CSettings::SETTING_SCREENSAVER_MODE);
  temp.insert(CSettings::SETTING_SCREENSAVER_PREVIEW);
  temp.insert(CSettings::SETTING_SCREENSAVER_SETTINGS);
  temp.insert(CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION);
  temp.insert(CSettings::SETTING_SOURCE_VIDEOS);
  temp.insert(CSettings::SETTING_SOURCE_MUSIC);
  temp.insert(CSettings::SETTING_SOURCE_PICTURES);
  settingsMgr->RegisterCallback(this, temp);

  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
  if (!appPlayer)
    return;

  temp.clear();
  temp.insert(CSettings::SETTING_VIDEOPLAYER_SEEKDELAY);
  temp.insert(CSettings::SETTING_VIDEOPLAYER_SEEKSTEPS);
  temp.insert(CSettings::SETTING_MUSICPLAYER_SEEKDELAY);
  temp.insert(CSettings::SETTING_MUSICPLAYER_SEEKSTEPS);
  settingsMgr->RegisterCallback(
      &appPlayer->GetSeekHandler(),
      temp);

  settingsMgr->AddDynamicCondition("isplaying", IsPlaying, appPlayer.get());

  settings->RegisterSubSettings(this);
}

void CApplicationSettingsHandling::UnregisterSettings()
{
  const boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  CSettingsManager* settingsMgr = settings->GetSettingsManager();
  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
  if (!appPlayer)
    return;

  settings->UnregisterSubSettings(this);
  settingsMgr->RemoveDynamicCondition("isplaying");
  settingsMgr->UnregisterCallback(&appPlayer->GetSeekHandler());
  settingsMgr->UnregisterCallback(this);
  settingsMgr->UnregisterSettingsHandler(this);
}

void CApplicationSettingsHandling::OnSettingChanged(const boost::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationSkinHandling> appSkin = components.GetComponent<CApplicationSkinHandling>();
  if (appSkin->OnSettingChanged(*setting))
    return;

  const boost::shared_ptr<CApplicationVolumeHandling> appVolume = components.GetComponent<CApplicationVolumeHandling>();
  if (appVolume->OnSettingChanged(*setting))
    return;

  const std::string& settingId = setting->GetId();

  if (StringUtils::StartsWithNoCase(settingId, "audiooutput."))
  {
    if (settingId == CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH)
      g_audioConfig.SetAC3Enabled(boost::static_pointer_cast<const CSettingBool>(setting)->GetValue());
    else if (settingId == CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH)
      g_audioConfig.SetDTSEnabled(boost::static_pointer_cast<const CSettingBool>(setting)->GetValue());
    else if (settingId == CSettings::SETTING_AUDIOOUTPUT_AACPASSTHROUGH)
      g_audioConfig.SetAACEnabled(boost::static_pointer_cast<const CSettingBool>(setting)->GetValue());
    else if (settingId == CSettings::SETTING_AUDIOOUTPUT_MP1PASSTHROUGH)
      g_audioConfig.SetMP1Enabled(boost::static_pointer_cast<const CSettingBool>(setting)->GetValue());
    else if (settingId == CSettings::SETTING_AUDIOOUTPUT_MP2PASSTHROUGH)
      g_audioConfig.SetMP2Enabled(boost::static_pointer_cast<const CSettingBool>(setting)->GetValue());
    else if (settingId == CSettings::SETTING_AUDIOOUTPUT_MP3PASSTHROUGH)
      g_audioConfig.SetMP3Enabled(boost::static_pointer_cast<const CSettingBool>(setting)->GetValue());

    g_audioConfig.Save();
  }
}

void CApplicationSettingsHandling::OnSettingAction(const boost::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationPowerHandling> appPower = components.GetComponent<CApplicationPowerHandling>();
  if (appPower->OnSettingAction(*setting))
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOOKANDFEEL_SKINSETTINGS)
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SKIN_SETTINGS);
  else if (settingId == CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION)
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SCREEN_CALIBRATION);
  else if (settingId == CSettings::SETTING_SOURCE_VIDEOS)
  {
    std::vector<std::string> params;
    params.push_back("library://video/files.xml");
    params.push_back("return");
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VIDEO_NAV, params);
  }
  else if (settingId == CSettings::SETTING_SOURCE_MUSIC)
  {
    std::vector<std::string> params;
    params.push_back("library://music/files.xml");
    params.push_back("return");
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_MUSIC_NAV, params);
  }
  else if (settingId == CSettings::SETTING_SOURCE_PICTURES)
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_PICTURES);
}

bool CApplicationSettingsHandling::Load(const TiXmlNode* settings)
{
  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationVolumeHandling> appVolume = components.GetComponent<CApplicationVolumeHandling>();
  return appVolume->Load(settings);
}

bool CApplicationSettingsHandling::Save(TiXmlNode* settings) const
{
  const CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<const CApplicationVolumeHandling> appVolume = components.GetComponent<CApplicationVolumeHandling>();
  return appVolume->Save(settings);
}
