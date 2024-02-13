/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#include "video/dialogs/GUIDialogVideoSettings.h"
#include "GUIWindowManager.h"
#include "GUIPassword.h"
#include "Util.h"
#include "utils/MathUtils.h"
#include "LocalizeStrings.h"
#include "Application.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "video/VideoDatabase.h"
#include "dialogs/GUIDialogYesNo.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "addons/Skin.h"

CGUIDialogVideoSettings::CGUIDialogVideoSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_VIDEO_OSD_SETTINGS, "VideoOSDSettings.xml")
{
}

CGUIDialogVideoSettings::~CGUIDialogVideoSettings(void)
{
}

#define VIDEO_SETTINGS_CROP               1
#define VIDEO_SETTINGS_VIEW_MODE          2
#define VIDEO_SETTINGS_ZOOM               3
#define VIDEO_SETTINGS_PIXEL_RATIO        4
#define VIDEO_SETTINGS_BRIGHTNESS         5
#define VIDEO_SETTINGS_CONTRAST           6
#define VIDEO_SETTINGS_GAMMA              7
#define VIDEO_SETTINGS_INTERLACEMETHOD    8
// separator 9
#define VIDEO_SETTINGS_MAKE_DEFAULT       10

#define VIDEO_SETTINGS_CALIBRATION        11
#define VIDEO_SETTINGS_FLICKER            12
#define VIDEO_SETTINGS_SOFTEN             13
#define VIDEO_SETTINGS_FILM_GRAIN         14
#define VIDEO_SETTINGS_NON_INTERLEAVED    15
#define VIDEO_SETTINGS_NO_CACHE           16
#define VIDEO_SETTINGS_FORCE_INDEX        17

#define VIDEO_SETTINGS_POSTPROCESS        22

void CGUIDialogVideoSettings::CreateSettings()
{
  m_usePopupSliders = g_SkinInfo->HasSkinFile("DialogSlider.xml");
  // clear out any old settings
  m_settings.clear();
  // create our settings
  {
    const int entries[] = { 16018, 16019, 20131, 20130, 20129, 16022, 16021, 16020};
    AddSpin(VIDEO_SETTINGS_INTERLACEMETHOD, 38761, (int*)&CMediaSettings::Get().GetCurrentVideoSettings().m_InterlaceMethod, 8, entries);
  }
  AddBool(VIDEO_SETTINGS_CROP, 644, &CMediaSettings::Get().GetCurrentVideoSettings().m_Crop);
  {
    const int entries[] = {630, 631, 632, 633, 634, 635, 636 };
    AddSpin(VIDEO_SETTINGS_VIEW_MODE, 629, &CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode, 7, entries);
  }
  AddSlider(VIDEO_SETTINGS_ZOOM, 216, &CMediaSettings::Get().GetCurrentVideoSettings().m_CustomZoomAmount, 0.5f, 0.01f, 2.0f, FormatFloat);
  AddSlider(VIDEO_SETTINGS_PIXEL_RATIO, 217, &CMediaSettings::Get().GetCurrentVideoSettings().m_CustomPixelRatio, 0.5f, 0.01f, 2.0f, FormatFloat);
  AddBool(VIDEO_SETTINGS_POSTPROCESS, 16400, &CMediaSettings::Get().GetCurrentVideoSettings().m_PostProcess);

  AddSlider(VIDEO_SETTINGS_BRIGHTNESS, 464, &CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness, 0, 1, 100, FormatInteger);
  AddSlider(VIDEO_SETTINGS_CONTRAST, 465, &CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast, 0, 1, 100, FormatInteger);
  AddSlider(VIDEO_SETTINGS_GAMMA, 466, &CMediaSettings::Get().GetCurrentVideoSettings().m_Gamma, 0, 1, 100, FormatInteger);

  AddSeparator(8);
  AddButton(VIDEO_SETTINGS_MAKE_DEFAULT, 12376);
  m_flickerFilter = CSettings::Get().GetInt("videoplayer.flicker");
  AddSpin(VIDEO_SETTINGS_FLICKER, 13100, &m_flickerFilter, 0, 5, g_localizeStrings.Get(351).c_str());
  m_soften = CSettings::Get().GetBool("videoplayer.soften");
  AddBool(VIDEO_SETTINGS_SOFTEN, 215, &m_soften);
  AddButton(VIDEO_SETTINGS_CALIBRATION, 214);
  if (g_application.GetCurrentPlayer() == EPC_MPLAYER)
  {
    AddSlider(VIDEO_SETTINGS_FILM_GRAIN, 14058, &CMediaSettings::Get().GetCurrentVideoSettings().m_FilmGrain, 0, 1, 10, FormatInteger);
    AddBool(VIDEO_SETTINGS_NON_INTERLEAVED, 306, &CMediaSettings::Get().GetCurrentVideoSettings().m_NonInterleaved);
    AddBool(VIDEO_SETTINGS_NO_CACHE, 431, &CMediaSettings::Get().GetCurrentVideoSettings().m_NoCache);
    AddButton(VIDEO_SETTINGS_FORCE_INDEX, 12009);
  }
}

void CGUIDialogVideoSettings::OnSettingChanged(SettingInfo &setting)
{
  // check and update anything that needs it
  if (setting.id == VIDEO_SETTINGS_NON_INTERLEAVED ||  setting.id == VIDEO_SETTINGS_NO_CACHE)
    g_application.Restart(true);
  else if (setting.id == VIDEO_SETTINGS_FILM_GRAIN)
    g_application.DelayedPlayerRestart();
#ifdef HAS_VIDEO_PLAYBACK
  else if (setting.id == VIDEO_SETTINGS_CROP)
    g_renderManager.AutoCrop(CMediaSettings::Get().GetCurrentVideoSettings().m_Crop);
  else if (setting.id == VIDEO_SETTINGS_VIEW_MODE)
  {
    g_renderManager.SetViewMode(CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode);
    UpdateSetting(VIDEO_SETTINGS_ZOOM);
    UpdateSetting(VIDEO_SETTINGS_PIXEL_RATIO);
  }
  else if (setting.id == VIDEO_SETTINGS_ZOOM || setting.id == VIDEO_SETTINGS_PIXEL_RATIO)
  {
    CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode = ViewModeCustom;
    g_renderManager.SetViewMode(ViewModeCustom);
    UpdateSetting(VIDEO_SETTINGS_VIEW_MODE);
  }
#endif
  else if (setting.id == VIDEO_SETTINGS_BRIGHTNESS || setting.id == VIDEO_SETTINGS_CONTRAST || setting.id == VIDEO_SETTINGS_GAMMA)
    CUtil::SetBrightnessContrastGammaPercent(CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness, CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast, CMediaSettings::Get().GetCurrentVideoSettings().m_Gamma, true);
  else if (setting.id == VIDEO_SETTINGS_FLICKER || setting.id == VIDEO_SETTINGS_SOFTEN)
  {
    RESOLUTION res = g_graphicsContext.GetVideoResolution();
    CSettings::Get().SetInt("videoplayer.flicker", m_flickerFilter);
    CSettings::Get().SetBool("videoplayer.soften", m_soften);
    g_graphicsContext.SetVideoResolution(res);
  }
  else if (setting.id == VIDEO_SETTINGS_CALIBRATION)
  {
    // launch calibration window
    if (CProfilesManager::Get().GetCurrentProfile().settingsLocked() && CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return;
    g_windowManager.ActivateWindow(WINDOW_SCREEN_CALIBRATION);
  }
  else if (setting.id == VIDEO_SETTINGS_FORCE_INDEX)
  {
    CMediaSettings::Get().GetCurrentVideoSettings().m_bForceIndex = true;
    g_application.Restart(true);
  }
  else if (setting.id == VIDEO_SETTINGS_MAKE_DEFAULT)
  {
    if (CProfilesManager::Get().GetCurrentProfile().settingsLocked() && CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return;

    // prompt user if they are sure
    if (CGUIDialogYesNo::ShowAndGetInput(12376, 750, 0, 12377))
    { // reset the settings
      CVideoDatabase db;
      db.Open();
      db.EraseVideoSettings();
      db.Close();
      CMediaSettings::Get().GetDefaultVideoSettings() = CMediaSettings::Get().GetCurrentVideoSettings();
      CMediaSettings::Get().GetDefaultVideoSettings().m_SubtitleStream = -1;
      CMediaSettings::Get().GetDefaultVideoSettings().m_AudioStream = -1;
      CSettings::Get().Save();
    }
  }
}

CStdString CGUIDialogVideoSettings::FormatInteger(float value, float minimum)
{
  CStdString text;
  text.Format("%i", MathUtils::round_int(value));
  return text;
}

CStdString CGUIDialogVideoSettings::FormatFloat(float value, float minimum)
{
  CStdString text;
  text.Format("%2.2f", value);
  return text;
}

