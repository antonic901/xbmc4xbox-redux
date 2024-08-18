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
#include "music/windows/GUIWindowVisualisation.h"
#include "GUIVisualisationControl.h"
#include "Application.h"
#include "music/dialogs/GUIDialogMusicOSD.h"
#include "GUIInfoManager.h"
#include "input/ButtonTranslator.h"
#include "music/dialogs/GUIDialogVisualisationPresetList.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"

using namespace MUSIC_INFO;

#define START_FADE_LENGTH  2.0f // 2 seconds on startup

#define CONTROL_VIS          2

CGUIWindowVisualisation::CGUIWindowVisualisation(void)
    : CGUIWindow(WINDOW_VISUALISATION, "MusicVisualisation.xml"),
      m_initTimer(true), m_lockedTimer(true)
{
  m_bShowPreset = false;
  m_loadType = KEEP_IN_MEMORY;
}

bool CGUIWindowVisualisation::OnAction(const CAction &action)
{
  bool passToVis = false;
  switch (action.GetID())
  {
  case ACTION_VIS_PRESET_NEXT:
  case ACTION_VIS_PRESET_PREV:
  case ACTION_VIS_PRESET_RANDOM:
  case ACTION_VIS_RATE_PRESET_PLUS:
  case ACTION_VIS_RATE_PRESET_MINUS:
    passToVis = true;
    break;

  case ACTION_SHOW_INFO:
    {
      m_initTimer.Stop();
      CSettings::GetInstance().SetBool("mymusic.songthumbinvis", g_infoManager.ToggleShowInfo());
      return true;
    }
    break;

  case ACTION_SHOW_OSD:
    g_windowManager.ActivateWindow(WINDOW_DIALOG_MUSIC_OSD);
    return true;

  case ACTION_SHOW_GUI:
    // save the settings
    CSettings::GetInstance().Save();
    g_windowManager.PreviousWindow();
    return true;
    break;

  case ACTION_VIS_PRESET_LOCK:
    { // show the locked icon + fall through so that the vis handles the locking
      if (!m_bShowPreset)
      {
        m_lockedTimer.StartZero();
        g_infoManager.SetShowCodec(true);
      }
      passToVis = true;
    }
    break;
  case ACTION_VIS_PRESET_SHOW:
    {
      if (!m_lockedTimer.IsRunning() || m_bShowPreset)
        m_bShowPreset = !m_bShowPreset;
      g_infoManager.SetShowCodec(m_bShowPreset);
      return true;
    }
    break;

  case ACTION_DECREASE_RATING:
  case ACTION_INCREASE_RATING:
    {
      // actual action is taken care of in CApplication::OnAction()
      m_initTimer.StartZero();
      g_infoManager.SetShowInfo(true);
    }
    break;
    // TODO: These should be mapped to it's own function - at the moment it's overriding
    // the global action of fastforward/rewind and OSD.
/*  case KEY_BUTTON_Y:
    g_application.m_CdgParser.Pause();
    return true;
    break;

    case ACTION_ANALOG_FORWARD:
    // calculate the speed based on the amount the button is held down
    if (action.GetAmount())
    {
      float AVDelay = g_application.m_CdgParser.GetAVDelay();
      g_application.m_CdgParser.SetAVDelay(AVDelay - action.GetAmount() / 4.0f);
      return true;
    }
    break;*/
  }

  if (passToVis)
  {
    CGUIControl *control = GetControl(CONTROL_VIS);
    if (control)
      return control->OnAction(action);
  }

  return CGUIWindow::OnAction(action);
}

bool CGUIWindowVisualisation::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_GET_VISUALISATION:
  case GUI_MSG_VISUALISATION_RELOAD:
  case GUI_MSG_PLAYBACK_STARTED:
    {
      CGUIControl *control = GetControl(CONTROL_VIS);
      if (control)
        return control->OnMessage(message);
    }
    break;
  case GUI_MSG_VISUALISATION_ACTION:
  { 
    CAction action(message.GetParam1());
    return OnAction(action);
  }
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (IsActive()) // save any changed settings from the OSD
        CSettings::GetInstance().Save();

      // close all active modal dialogs
      g_windowManager.CloseInternalModalDialogs(true);
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      // check whether we've come back here from a window during which time we've actually
      // stopped playing music
      if (message.GetParam1() == WINDOW_INVALID && !g_application.IsPlayingAudio())
      { // why are we here if nothing is playing???
        g_windowManager.PreviousWindow();
        return true;
      }

      // hide or show the preset button(s)
      g_infoManager.SetShowCodec(m_bShowPreset);
      g_infoManager.SetShowInfo(true);  // always show the info initially.
      CGUIWindow::OnMessage(message);
      if (g_infoManager.GetCurrentSongTag())
        m_tag = *g_infoManager.GetCurrentSongTag();
      if (CSettings::GetInstance().GetBool("mymusic.songthumbinvis"))
      { // always on
        m_initTimer.Stop();
      }
      else
      {
        // start display init timer (fade out after 3 secs...)
        m_initTimer.StartZero();
      }
      return true;
    }
  }
  return CGUIWindow::OnMessage(message);
}

EVENT_RESULT CGUIWindowVisualisation::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_RIGHT_CLICK)
  { // no control found to absorb this click - go back to GUI
    OnAction(CAction(ACTION_SHOW_GUI));
    return EVENT_RESULT_HANDLED;
  }
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  { // no control found to absorb this click - toggle the track INFO
    return g_application.OnAction(CAction(ACTION_PAUSE)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }
  if (event.m_id != ACTION_MOUSE_MOVE || event.m_offsetX || event.m_offsetY)
  { // some other mouse action has occurred - bring up the OSD
    CGUIDialog *pOSD = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_OSD);
    if (pOSD)
    {
      pOSD->SetAutoClose(3000);
      pOSD->Open();
    }
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

void CGUIWindowVisualisation::FrameMove()
{
  g_application.ResetScreenSaver();
  // check for a tag change
  const CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();
  if (tag && *tag != m_tag)
  { // need to fade in then out again
    m_tag = *tag;
    // fade in
    m_initTimer.StartZero();
    g_infoManager.SetShowInfo(true);
  }
  if (m_initTimer.IsRunning() && m_initTimer.GetElapsedSeconds() > (float)g_advancedSettings.m_songInfoDuration)
  {
    m_initTimer.Stop();
    if (!CSettings::GetInstance().GetBool("mymusic.songthumbinvis"))
    { // reached end of fade in, fade out again
      g_infoManager.SetShowInfo(false);
    }
  }
  // show or hide the locked texture
  if (m_lockedTimer.IsRunning() && m_lockedTimer.GetElapsedSeconds() > START_FADE_LENGTH)
  {
    m_lockedTimer.Stop();
    if (!m_bShowPreset)
    {
      g_infoManager.SetShowCodec(false);
    }
  }
  CGUIWindow::FrameMove();
}

void CGUIWindowVisualisation::AllocResources(bool forceLoad)
{
  CGUIWindow::AllocResources(forceLoad);
  CGUIWindow *pWindow;
  pWindow = g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_OSD);
  if (pWindow) pWindow->AllocResources(true);
  pWindow = g_windowManager.GetWindow(WINDOW_DIALOG_VIS_SETTINGS);
  if (pWindow) pWindow->AllocResources(true);
  pWindow = g_windowManager.GetWindow(WINDOW_DIALOG_VIS_PRESET_LIST);
  if (pWindow) pWindow->AllocResources(true);
}

void CGUIWindowVisualisation::FreeResources(bool forceUnload)
{
  // Save changed settings from music OSD
  CSettings::GetInstance().Save();
  CGUIWindow *pWindow;
  pWindow = g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_OSD);
  if (pWindow) pWindow->FreeResources(true);
  pWindow = g_windowManager.GetWindow(WINDOW_DIALOG_VIS_SETTINGS);
  if (pWindow) pWindow->FreeResources(true);
  pWindow = g_windowManager.GetWindow(WINDOW_DIALOG_VIS_PRESET_LIST);
  if (pWindow) pWindow->FreeResources(true);
  CGUIWindow::FreeResources(forceUnload);
}

