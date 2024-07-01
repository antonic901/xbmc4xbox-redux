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

#include "system.h"
#include "GUIDialogVideoSettings.h"
#include "GUIPassword.h"
#include "addons/Skin.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#endif
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "profiles/ProfilesManager.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "Application.h"
#include "Util.h"

#define SETTING_VIDEO_CROP                "video.crop"
#define SETTING_VIDEO_VIEW_MODE           "video.viewmode"
#define SETTING_VIDEO_ZOOM                "video.zoom"
#define SETTING_VIDEO_PIXEL_RATIO         "video.pixelratio"
#define SETTING_VIDEO_BRIGHTNESS          "video.brightness"
#define SETTING_VIDEO_CONTRAST            "video.contrast"
#define SETTING_VIDEO_GAMMA               "video.gamma"
#define SETTING_VIDEO_POSTPROCESS         "video.postprocess"

#define SETTING_VIDEO_INTERLACEMETHOD     "video.interlacemethod"

#define SETTING_VIDEO_MAKE_DEFAULT        "video.save"
#define SETTING_VIDEO_CALIBRATION         "video.calibration"

#define SETTING_VIDEO_FLICKER             "video.flicker"
#define SETTING_VIDEO_SOFTEN              "video.soften"
#define SETTING_VIDEO_FILM_GRAIN          "video.filmgrain"
#define SETTING_VIDEO_NON_INTERLEAVED     "video.noninterleaved"
#define SETTING_VIDEO_NO_CACHE            "video.nocache"
#define SETTING_VIDEO_FORCE_INDEX         "video.forceindex"

using namespace std;

CGUIDialogVideoSettings::CGUIDialogVideoSettings()
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_VIDEO_OSD_SETTINGS, "DialogSettings.xml"),
      m_viewModeChanged(false)
{ }

CGUIDialogVideoSettings::~CGUIDialogVideoSettings()
{ }

void CGUIDialogVideoSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  CVideoSettings &videoSettings = CMediaSettings::Get().GetCurrentVideoSettings();

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_VIDEO_INTERLACEMETHOD)
    videoSettings.m_InterlaceMethod = static_cast<EINTERLACEMETHOD>(static_cast<const CSettingInt*>(setting)->GetValue());
#ifdef HAS_VIDEO_PLAYBACK
  else if (settingId == SETTING_VIDEO_CROP)
  {
    videoSettings.m_Crop = static_cast<const CSettingBool*>(setting)->GetValue();
    g_renderManager.AutoCrop(videoSettings.m_Crop);
  }
  else if (settingId == SETTING_VIDEO_VIEW_MODE)
  {
    videoSettings.m_ViewMode = static_cast<const CSettingInt*>(setting)->GetValue();

    g_renderManager.SetViewMode(videoSettings.m_ViewMode);

    m_viewModeChanged = true;
    m_settingsManager->SetNumber(SETTING_VIDEO_ZOOM, videoSettings.m_CustomZoomAmount);
    m_settingsManager->SetNumber(SETTING_VIDEO_PIXEL_RATIO, videoSettings.m_CustomPixelRatio);
    m_viewModeChanged = false;
  }
  else if (settingId == SETTING_VIDEO_ZOOM ||
           settingId == SETTING_VIDEO_PIXEL_RATIO)
  {
    if (settingId == SETTING_VIDEO_ZOOM)
      videoSettings.m_CustomZoomAmount = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    else if (settingId == SETTING_VIDEO_PIXEL_RATIO)
      videoSettings.m_CustomPixelRatio = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());

    if (!m_viewModeChanged)
    {
      // try changing the view mode to custom. If it already is set to custom
      // manually call the render manager
      if (m_settingsManager->GetInt(SETTING_VIDEO_VIEW_MODE) != ViewModeCustom)
        m_settingsManager->SetInt(SETTING_VIDEO_VIEW_MODE, ViewModeCustom);
      else
        g_renderManager.SetViewMode(videoSettings.m_ViewMode);
    }
  }
  else if (settingId == SETTING_VIDEO_POSTPROCESS)
    videoSettings.m_PostProcess = static_cast<const CSettingBool*>(setting)->GetValue();
  else if (settingId == SETTING_VIDEO_BRIGHTNESS)
  {
    videoSettings.m_Brightness = static_cast<float>(static_cast<const CSettingInt*>(setting)->GetValue());
    CUtil::SetBrightnessContrastGammaPercent(videoSettings.m_Brightness, videoSettings.m_Contrast, videoSettings.m_Gamma, true);
  }
  else if (settingId == SETTING_VIDEO_CONTRAST)
  {
    videoSettings.m_Contrast = static_cast<float>(static_cast<const CSettingInt*>(setting)->GetValue());
    CUtil::SetBrightnessContrastGammaPercent(videoSettings.m_Brightness, videoSettings.m_Contrast, videoSettings.m_Gamma, true);
  }
  else if (settingId == SETTING_VIDEO_GAMMA)
  {
    videoSettings.m_Gamma = static_cast<float>(static_cast<const CSettingInt*>(setting)->GetValue());
    CUtil::SetBrightnessContrastGammaPercent(videoSettings.m_Brightness, videoSettings.m_Contrast, videoSettings.m_Gamma, true);
  }
  else if (settingId == SETTING_VIDEO_FLICKER)
  {
    CSettings::Get().SetInt("videoplayer.flicker", static_cast<const CSettingInt*>(setting)->GetValue());
    RESOLUTION res = g_graphicsContext.GetVideoResolution();
    g_graphicsContext.SetVideoResolution(res);
  }
  else if (settingId == SETTING_VIDEO_SOFTEN)
  {
    CSettings::Get().SetBool("videoplayer.soften", static_cast<const CSettingBool*>(setting)->GetValue());
    RESOLUTION res = g_graphicsContext.GetVideoResolution();
    g_graphicsContext.SetVideoResolution(res);
  }
  else if (settingId == SETTING_VIDEO_NON_INTERLEAVED ||  settingId == SETTING_VIDEO_NO_CACHE)
    g_application.Restart(true);
  else if (settingId == SETTING_VIDEO_FILM_GRAIN)
    g_application.DelayedPlayerRestart();
#endif
}

void CGUIDialogVideoSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_VIDEO_CALIBRATION)
  {
    // launch calibration window
    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE  &&
        g_passwordManager.CheckSettingLevelLock(CSettings::Get().GetSetting("videoscreen.guicalibration")->GetLevel()))
      return;
    g_windowManager.ForceActivateWindow(WINDOW_SCREEN_CALIBRATION);
  }
  // TODO
  else if (settingId == SETTING_VIDEO_MAKE_DEFAULT)
    Save();
  else if (settingId == SETTING_VIDEO_FORCE_INDEX)
  {
    CMediaSettings::Get().GetCurrentVideoSettings().m_bForceIndex = true;
    g_application.Restart(true);
  }
}

void CGUIDialogVideoSettings::Save()
{
  if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
      !g_passwordManager.CheckSettingLevelLock(::SettingLevelExpert))
    return;

  // prompt user if they are sure
  if (CGUIDialogYesNo::ShowAndGetInput(12376, 750, 0, 12377))
  { // reset the settings
    CVideoDatabase db;
    if (!db.Open())
      return;
    db.EraseVideoSettings();
    db.Close();

    CMediaSettings::Get().GetDefaultVideoSettings() = CMediaSettings::Get().GetCurrentVideoSettings();
    CMediaSettings::Get().GetDefaultVideoSettings().m_SubtitleStream = -1;
    CMediaSettings::Get().GetDefaultVideoSettings().m_AudioStream = -1;
    CSettings::Get().Save();
  }
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

  CSettingCategory *category = AddCategory("audiosubtitlesettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  CSettingGroup *groupVideo = AddGroup(category);
  if (groupVideo == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }
  CSettingGroup *groupVideoPlayback = AddGroup(category);
  if (groupVideoPlayback == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }
  CSettingGroup *groupSaveAsDefault = AddGroup(category);
  if (groupSaveAsDefault == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogVideoSettings: unable to setup settings");
    return;
  }

  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  CVideoSettings &videoSettings = CMediaSettings::Get().GetCurrentVideoSettings();
  
  StaticIntegerSettingOptions entries;

  entries.clear();
  entries.push_back(make_pair(16018, VS_INTERLACEMETHOD_NONE));
  entries.push_back(make_pair(16019, VS_INTERLACEMETHOD_AUTO));
  entries.push_back(make_pair(20131, VS_INTERLACEMETHOD_RENDER_BLEND));
  entries.push_back(make_pair(20130, VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED));
  entries.push_back(make_pair(20129, VS_INTERLACEMETHOD_RENDER_WEAVE));
  entries.push_back(make_pair(16022, VS_INTERLACEMETHOD_RENDER_BOB_INVERTED));
  entries.push_back(make_pair(16021, VS_INTERLACEMETHOD_RENDER_BOB));
  entries.push_back(make_pair(16020, VS_INTERLACEMETHOD_DEINTERLACE));

  if (!entries.empty())
    CSettingInt *settingInterlaceMethod = AddSpinner(groupVideo, SETTING_VIDEO_INTERLACEMETHOD, 16038, 0, static_cast<int>(videoSettings.m_InterlaceMethod), entries);

#ifdef HAS_VIDEO_PLAYBACK
  /*if (g_renderManager.Supports(RENDERFEATURE_CROP))*/
    AddToggle(groupVideo, SETTING_VIDEO_CROP, 644, 0, videoSettings.m_Crop);

  /*if (g_renderManager.Supports(RENDERFEATURE_STRETCH) || g_renderManager.Supports(RENDERFEATURE_PIXEL_RATIO))*/
  {
    entries.clear();
    for (int i = 0; i < 7; ++i)
      entries.push_back(make_pair(630 + i, i));
    AddSpinner(groupVideo, SETTING_VIDEO_VIEW_MODE, 629, 0, videoSettings.m_ViewMode, entries);
  }
  /*if (g_renderManager.Supports(RENDERFEATURE_ZOOM))*/
    AddSlider(groupVideo, SETTING_VIDEO_ZOOM, 216, 0, videoSettings.m_CustomZoomAmount, "%2.2f", 0.5f, 0.01f, 2.0f, -1, usePopup);
  /*if (g_renderManager.Supports(RENDERFEATURE_PIXEL_RATIO))*/
    AddSlider(groupVideo, SETTING_VIDEO_PIXEL_RATIO, 217, 0, videoSettings.m_CustomPixelRatio, "%2.2f", 0.5f, 0.01f, 2.0f, -1, usePopup);
  /*if (g_renderManager.Supports(RENDERFEATURE_POSTPROCESS))*/
    AddToggle(groupVideo, SETTING_VIDEO_POSTPROCESS, 16400, 0, videoSettings.m_PostProcess);
  /*if (g_renderManager.Supports(RENDERFEATURE_BRIGHTNESS))*/
    AddPercentageSlider(groupVideoPlayback, SETTING_VIDEO_BRIGHTNESS, 464, 0, static_cast<int>(videoSettings.m_Brightness), 14047, 1, 464, usePopup);
  /*if (g_renderManager.Supports(RENDERFEATURE_CONTRAST))*/
    AddPercentageSlider(groupVideoPlayback, SETTING_VIDEO_CONTRAST, 465, 0, static_cast<int>(videoSettings.m_Contrast), 14047, 1, 465, usePopup);
  /*if (g_renderManager.Supports(RENDERFEATURE_GAMMA))*/
    AddPercentageSlider(groupVideoPlayback, SETTING_VIDEO_GAMMA, 466, 0, static_cast<int>(videoSettings.m_Gamma), 14047, 1, 466, usePopup);
  AddSpinner(groupSaveAsDefault, SETTING_VIDEO_FLICKER, 13100, 0, CSettings::Get().GetInt("videoplayer.flicker"), 0, 1, 5, -1, 351);
  AddToggle(groupSaveAsDefault, SETTING_VIDEO_SOFTEN, 215, 0, CSettings::Get().GetBool("videoplayer.soften"));
  if (g_application.GetCurrentPlayer() == EPC_MPLAYER)
  {
    AddSlider(groupVideoPlayback, SETTING_VIDEO_FILM_GRAIN, 14058, 0, videoSettings.m_FilmGrain, "%f", 0.0f, 1.0f, 10.0f);
    AddToggle(groupVideoPlayback, SETTING_VIDEO_NON_INTERLEAVED, 306, videoSettings.m_NonInterleaved, 0);
    AddToggle(groupVideoPlayback, SETTING_VIDEO_NO_CACHE, 431, videoSettings.m_NoCache, 0);
    AddButton(groupSaveAsDefault, SETTING_VIDEO_FORCE_INDEX, 12009, 0);
  }
#endif

  // general settings
  AddButton(groupSaveAsDefault, SETTING_VIDEO_MAKE_DEFAULT, 12376, 0);
  AddButton(groupSaveAsDefault, SETTING_VIDEO_CALIBRATION, 214, 0);
}
