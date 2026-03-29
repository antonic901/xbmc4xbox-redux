/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVolumeBar.h"

#include "Application.h"
#include "guilib/GUIMessage.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"

#define VOLUME_BAR_DISPLAY_TIME 1000L

CGUIDialogVolumeBar::CGUIDialogVolumeBar(void)
  : CGUIDialog(WINDOW_DIALOG_VOLUME_BAR, "DialogVolumeBar.xml", MODELESS)
{
  m_loadType = LOAD_ON_GUI_INIT;
}

CGUIDialogVolumeBar::~CGUIDialogVolumeBar(void) {}

bool CGUIDialogVolumeBar::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_VOLUME_UP || action.GetID() == ACTION_VOLUME_DOWN || action.GetID() == ACTION_VOLUME_SET || action.GetID() == ACTION_MUTE)
  {
    if (g_application.IsMuted() ||
        g_application.GetVolume(false) <= VOLUME_MINIMUM)
    { // cancel the timer, dialog needs to stay visible
      CancelAutoClose();
    }
    else
    { // reset the timer, as we've changed the volume level
      SetAutoClose(VOLUME_BAR_DISPLAY_TIME);
    }
    MarkDirtyRegion();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogVolumeBar::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
  case GUI_MSG_WINDOW_DEINIT:
    return CGUIDialog::OnMessage(message);
  }
  return false; // don't process anything other than what we need!
}
