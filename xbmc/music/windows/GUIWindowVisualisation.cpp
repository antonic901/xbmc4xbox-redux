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

#include "GUIWindowVisualisation.h"
#include "Application.h"
#include "FileItem.h"
#include "music/dialogs/GUIDialogMusicOSD.h"
#include "GUIUserMessages.h"
#include "GUIInfoManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/ButtonTranslator.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

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
#ifndef _XBOX
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH) &&
      CServiceBroker::GetGUI()->GetInfoManager().IsPlayerChannelPreviewActive() &&
      (action.GetID() == ACTION_SELECT_ITEM || CButtonTranslator::GetInstance().GetGlobalAction(action.GetButtonCode()).GetID() == ACTION_SELECT_ITEM))
  {
    // If confirm channel switch is active, channel preview is currently shown
    // and the button that caused this action matches (global) action "Select" (OK)
    // switch to the channel currently displayed within the preview.
    g_application.m_pPlayer->SwitchChannel(g_application.CurrentFileItem().GetPVRChannelInfoTag());
    return true;
  }
#endif

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
      CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool("mymusic.songthumbinvis", CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().ToggleShowInfo());
      return true;
    }
    break;

  case ACTION_SHOW_OSD:
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_DIALOG_MUSIC_OSD);
    return true;

  case ACTION_SHOW_GUI:
    // save the settings
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
    CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
    return true;
    break;

  case ACTION_VIS_PRESET_LOCK:
    { // show the locked icon + fall through so that the vis handles the locking
      if (!m_bShowPreset)
      {
        m_lockedTimer.StartZero();
      }
      passToVis = true;
    }
    break;
  case ACTION_VIS_PRESET_SHOW:
    {
      if (!m_lockedTimer.IsRunning() || m_bShowPreset)
        m_bShowPreset = !m_bShowPreset;
      return true;
    }
    break;

  case ACTION_DECREASE_RATING:
  case ACTION_INCREASE_RATING:
    {
      // actual action is taken care of in CApplication::OnAction()
      m_initTimer.StartZero();
      CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().SetShowInfo(true);
    }
    break;
    //! @todo These should be mapped to it's own function - at the moment it's overriding
    //! the global action of fastforward/rewind and OSD.
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
        CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

      // close all active modal dialogs
      CServiceBroker::GetGUI()->GetWindowManager().CloseInternalModalDialogs(true);
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      // check whether we've come back here from a window during which time we've actually
      // stopped playing music
      if (message.GetParam1() == WINDOW_INVALID && !g_application.m_pPlayer->IsPlayingAudio())
      { // why are we here if nothing is playing???
        CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
        return true;
      }

      // hide or show the preset button(s)
      CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().SetShowInfo(true);  // always show the info initially.
      CGUIWindow::OnMessage(message);
      if (CServiceBroker::GetGUI()->GetInfoManager().GetCurrentSongTag())
        m_tag = *CServiceBroker::GetGUI()->GetInfoManager().GetCurrentSongTag();

      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("mymusic.songthumbinvis"))
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

void CGUIWindowVisualisation::FrameMove()
{
  // check for a tag change
  const CMusicInfoTag* tag = CServiceBroker::GetGUI()->GetInfoManager().GetCurrentSongTag();
  if (tag && *tag != m_tag)
  { // need to fade in then out again
    m_tag = *tag;
    // fade in
    m_initTimer.StartZero();
    CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().SetShowInfo(true);
  }
  if (m_initTimer.IsRunning() && m_initTimer.GetElapsedSeconds() > (float)CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_songInfoDuration)
  {
    m_initTimer.Stop();
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("mymusic.songthumbinvis"))
    { // reached end of fade in, fade out again
      CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().SetShowInfo(false);
    }
  }
  // show or hide the locked texture
  if (m_lockedTimer.IsRunning() && m_lockedTimer.GetElapsedSeconds() > START_FADE_LENGTH)
  {
    m_lockedTimer.Stop();
  }
  CGUIWindow::FrameMove();
}
