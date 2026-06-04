/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVideoSettings.h"

#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "addons/Skin.h"
#include "Application.h"
#include "ApplicationPlayer.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfileManager.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingsManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/ViewModeSettings.h"

#include <utility>

#define SETTING_VIDEO_VIEW_MODE           "video.viewmode"
#define SETTING_VIDEO_ZOOM                "video.zoom"
#define SETTING_VIDEO_PIXEL_RATIO         "video.pixelratio"
#define SETTING_VIDEO_BRIGHTNESS          "video.brightness"
#define SETTING_VIDEO_CONTRAST            "video.contrast"
#define SETTING_VIDEO_GAMMA               "video.gamma"
#define SETTING_VIDEO_POSTPROCESS         "video.postprocess"

#define SETTING_VIDEO_INTERLACEMETHOD     "video.interlacemethod"

// Xbox specific settings
#define SETTING_VIDEO_CROP                "video.crop"
#define SETTING_VIDEO_FLICKER             "video.flicker"
#define SETTING_VIDEO_SOFTEN              "video.soften"
#define SETTING_VIDEO_FILM_GRAIN          "video.filmgrain"
#define SETTING_VIDEO_NON_INTERLEAVED     "video.noninterleaved"
#define SETTING_VIDEO_NO_CACHE            "video.nocache"
#define SETTING_VIDEO_FORCE_INDEX         "video.forceindex"

#define SETTING_VIDEO_MAKE_DEFAULT        "video.save"
#define SETTING_VIDEO_CALIBRATION         "video.calibration"

CGUIDialogVideoSettings::CGUIDialogVideoSettings()
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_VIDEO_OSD_SETTINGS, "DialogSettings.xml"), m_viewModeChanged(false)
{ }

CGUIDialogVideoSettings::~CGUIDialogVideoSettings() {}

void CGUIDialogVideoSettings::OnSettingChanged(const boost::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_VIDEO_INTERLACEMETHOD)
  {
    CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
    vs.m_InterlaceMethod = static_cast<EINTERLACEMETHOD>(boost::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  }
  else if (settingId == SETTING_VIDEO_VIEW_MODE)
  {
    int value = boost::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    const CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();

    g_renderManager.SetViewMode(vs.m_ViewMode);

    m_viewModeChanged = true;
    GetSettingsManager()->SetNumber(SETTING_VIDEO_ZOOM, static_cast<double>(vs.m_CustomZoomAmount));
    GetSettingsManager()->SetNumber(SETTING_VIDEO_PIXEL_RATIO,
                                    static_cast<double>(vs.m_CustomPixelRatio));
    m_viewModeChanged = false;
  }
  else if (settingId == SETTING_VIDEO_ZOOM ||
           settingId == SETTING_VIDEO_PIXEL_RATIO)
  {
    CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
    if (settingId == SETTING_VIDEO_ZOOM)
      vs.m_CustomZoomAmount = static_cast<float>(boost::static_pointer_cast<const CSettingNumber>(setting)->GetValue());
    else if (settingId == SETTING_VIDEO_PIXEL_RATIO)
      vs.m_CustomPixelRatio = static_cast<float>(boost::static_pointer_cast<const CSettingNumber>(setting)->GetValue());

    // try changing the view mode to custom. If it already is set to custom
    // manually call the render manager
    if (GetSettingsManager()->GetInt(SETTING_VIDEO_VIEW_MODE) != ViewModeCustom)
      GetSettingsManager()->SetInt(SETTING_VIDEO_VIEW_MODE, ViewModeCustom);
    else
      g_renderManager.SetViewMode(vs.m_ViewMode);
  }
  else if (settingId == SETTING_VIDEO_POSTPROCESS)
  {
    CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
    vs.m_PostProcess = boost::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  }
  else if (settingId == SETTING_VIDEO_BRIGHTNESS)
  {
    CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
    vs.m_Brightness = static_cast<float>(boost::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    CUtil::SetBrightnessContrastGammaPercent(vs.m_Brightness, vs.m_Contrast, vs.m_Gamma, true);
  }
  else if (settingId == SETTING_VIDEO_CONTRAST)
  {
    CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
    vs.m_Contrast = static_cast<float>(boost::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    CUtil::SetBrightnessContrastGammaPercent(vs.m_Brightness, vs.m_Contrast, vs.m_Gamma, true);
  }
  else if (settingId == SETTING_VIDEO_GAMMA)
  {
    CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
    vs.m_Gamma = static_cast<float>(boost::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    CUtil::SetBrightnessContrastGammaPercent(vs.m_Brightness, vs.m_Contrast, vs.m_Gamma, true);
  }
  else if (settingId == SETTING_VIDEO_CROP)
  {
    CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
    vs.m_Crop = boost::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    g_renderManager.AutoCrop(vs.m_Crop);
  }
  else if (settingId == SETTING_VIDEO_FLICKER)
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt("videoplayer.flicker", boost::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    RESOLUTION res = CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution();
    CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(res);
  }
  else if (settingId == SETTING_VIDEO_SOFTEN)
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool("videoplayer.soften", boost::static_pointer_cast<const CSettingBool>(setting)->GetValue());
    RESOLUTION res = CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution();
    CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(res);
  }
  else if (settingId == SETTING_VIDEO_NON_INTERLEAVED ||  settingId == SETTING_VIDEO_NO_CACHE)
    g_application.Restart(true);
  else if (settingId == SETTING_VIDEO_FILM_GRAIN)
    g_application.DelayedPlayerRestart();
}

void CGUIDialogVideoSettings::OnSettingAction(const boost::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_VIDEO_CALIBRATION)
  {
    const boost::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

    boost::shared_ptr<CSettingsComponent> settingsComponent = CServiceBroker::GetSettingsComponent();
    if (!settingsComponent)
      return;

    boost::shared_ptr<CSettings> settings = settingsComponent->GetSettings();
    if (!settings)
      return;

    SettingPtr calibsetting = settings->GetSetting(CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION);
    if (!calibsetting)
    {
      CLog::Log(LOGERROR, "Failed to load setting for: %s",
                CSettings::SETTING_VIDEOSCREEN_GUICALIBRATION);
      return;
    }

    // launch calibration window
    if (profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
        g_passwordManager.CheckSettingLevelLock(calibsetting->GetLevel()))
      return;

    CServiceBroker::GetGUI()->GetWindowManager().ForceActivateWindow(WINDOW_SCREEN_CALIBRATION);
  }
  //! @todo implement
  else if (settingId == SETTING_VIDEO_MAKE_DEFAULT)
    Save();
  else if (settingId == SETTING_VIDEO_FORCE_INDEX)
  {
    CMediaSettings::GetInstance().GetCurrentVideoSettings().m_bForceIndex = true;
    g_application.Restart(true);
  }
}

bool CGUIDialogVideoSettings::Save()
{
  const boost::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
      !g_passwordManager.CheckSettingLevelLock(::SettingLevel::Expert))
    return true;

  // prompt user if they are sure
  if (CGUIDialogYesNo::ShowAndGetInput(12376, 12377))
  { // reset the settings
    CVideoDatabase db;
    if (!db.Open())
      return true;
    db.EraseAllVideoSettings();
    db.Close();

    CMediaSettings::GetInstance().GetDefaultVideoSettings() = CMediaSettings::GetInstance().GetCurrentVideoSettings();
    CMediaSettings::GetInstance().GetDefaultVideoSettings().m_SubtitleStream = -1;
    CMediaSettings::GetInstance().GetDefaultVideoSettings().m_AudioStream = -1;
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
  }

  return true;
}

void CGUIDialogVideoSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(13395);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_OKAY_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 15067);
}

void CGUIDialogVideoSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const boost::shared_ptr<CSettingCategory> category = AddCategory("videosettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  const boost::shared_ptr<CSettingGroup> groupVideo = AddGroup(category);
  if (groupVideo == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }
  const boost::shared_ptr<CSettingGroup> groupSaveAsDefault = AddGroup(category);
  if (groupSaveAsDefault == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }

  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  const CVideoSettings& videoSettings = CMediaSettings::GetInstance().GetCurrentVideoSettings();

  TranslatableIntegerSettingOptions entries;

  entries.clear();
  entries.push_back(TranslatableIntegerSettingOption(16039, VS_INTERLACEMETHOD_NONE));
  entries.push_back(TranslatableIntegerSettingOption(16019, VS_INTERLACEMETHOD_AUTO));
  entries.push_back(TranslatableIntegerSettingOption(20131, VS_INTERLACEMETHOD_RENDER_BLEND));
  entries.push_back(TranslatableIntegerSettingOption(20130, VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED));
  entries.push_back(TranslatableIntegerSettingOption(20129, VS_INTERLACEMETHOD_RENDER_WEAVE));
  entries.push_back(TranslatableIntegerSettingOption(16022, VS_INTERLACEMETHOD_RENDER_BOB_INVERTED));
  entries.push_back(TranslatableIntegerSettingOption(16021, VS_INTERLACEMETHOD_RENDER_BOB));
  entries.push_back(TranslatableIntegerSettingOption(16020, VS_INTERLACEMETHOD_DEINTERLACE));

  if (!entries.empty())
  {
    EINTERLACEMETHOD method = videoSettings.m_InterlaceMethod;
    AddSpinner(groupVideo, SETTING_VIDEO_INTERLACEMETHOD, 16038, SettingLevel::Basic, static_cast<int>(method), entries);
  }

  AddToggle(groupVideo, SETTING_VIDEO_CROP, 644, SettingLevel::Basic, videoSettings.m_Crop);

  AddList(groupVideo, SETTING_VIDEO_VIEW_MODE, 629, SettingLevel::Basic, videoSettings.m_ViewMode, CViewModeSettings::ViewModesFiller, 629);
  AddSlider(groupVideo, SETTING_VIDEO_ZOOM, 216, SettingLevel::Basic,
            videoSettings.m_CustomZoomAmount, "%2.2f", 0.5f, 0.01f, 2.0f, 216, usePopup);
  AddSlider(groupVideo, SETTING_VIDEO_PIXEL_RATIO, 217, SettingLevel::Basic,
            videoSettings.m_CustomPixelRatio, "%2.2f", 0.5f, 0.01f, 2.0f, 217, usePopup);

  AddToggle(groupVideo, SETTING_VIDEO_POSTPROCESS, 16400, SettingLevel::Basic, videoSettings.m_PostProcess);
  AddPercentageSlider(groupVideo, SETTING_VIDEO_BRIGHTNESS, 464, SettingLevel::Basic, static_cast<int>(videoSettings.m_Brightness), 14047, 1, 464, usePopup);
  AddPercentageSlider(groupVideo, SETTING_VIDEO_CONTRAST, 465, SettingLevel::Basic, static_cast<int>(videoSettings.m_Contrast), 14047, 1, 465, usePopup);
  AddPercentageSlider(groupVideo, SETTING_VIDEO_GAMMA, 466, SettingLevel::Basic, static_cast<int>(videoSettings.m_Gamma), 14047, 1, 466, usePopup);

  if (g_application.GetCurrentPlayer() == EPC_MPLAYER)
  {
    AddSlider(groupVideo, SETTING_VIDEO_FILM_GRAIN, 14058, SettingLevel::Basic, videoSettings.m_FilmGrain, "%f", 0.0f, 1.0f, 10.0f);
    AddToggle(groupVideo, SETTING_VIDEO_NON_INTERLEAVED, 306, SettingLevel::Basic, videoSettings.m_NonInterleaved);
    AddToggle(groupVideo, SETTING_VIDEO_NO_CACHE, 431, SettingLevel::Basic, videoSettings.m_NoCache);
    AddButton(groupSaveAsDefault, SETTING_VIDEO_FORCE_INDEX, 12009, SettingLevel::Basic);
  }
  AddSpinner(groupSaveAsDefault, SETTING_VIDEO_FLICKER, 13100, SettingLevel::Basic, CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("videoplayer.flicker"), 0, 1, 5, -1, 351);
  AddToggle(groupSaveAsDefault, SETTING_VIDEO_SOFTEN, 215, SettingLevel::Basic, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("videoplayer.soften"));

  // general settings
  AddButton(groupSaveAsDefault, SETTING_VIDEO_MAKE_DEFAULT, 12376, SettingLevel::Basic);
  AddButton(groupSaveAsDefault, SETTING_VIDEO_CALIBRATION, 214, SettingLevel::Basic);
}
