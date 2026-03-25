/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSeekBar.h"

#include "Application.h"
#include "GUIInfoManager.h"
#include "SeekHandler.h"
#include "guilib/GUIComponent.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

#include <math.h>

#define POPUP_SEEK_PROGRESS           401

CGUIDialogSeekBar::CGUIDialogSeekBar(void)
  : CGUIDialog(WINDOW_DIALOG_SEEK_BAR, "DialogSeekBar.xml", MODELESS)
  , m_lastProgress(0)
{
  m_loadType = LOAD_ON_GUI_INIT;    // the application class handles our resources
}

CGUIDialogSeekBar::~CGUIDialogSeekBar(void) {};

bool CGUIDialogSeekBar::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
  case GUI_MSG_WINDOW_DEINIT:
    return CGUIDialog::OnMessage(message);
  case GUI_MSG_ITEM_SELECT:
    if (message.GetSenderId() == GetID() &&
        message.GetControlId() == POPUP_SEEK_PROGRESS)
      return CGUIDialog::OnMessage(message);
    break;
  case GUI_MSG_REFRESH_TIMER:
    return CGUIDialog::OnMessage(message);
  }
  return false; // don't process anything other than what we need!
}

void CGUIDialogSeekBar::FrameMove()
{
  if (!g_application.m_pPlayer->HasPlayer())
  {
    Close(true);
    return;
  }

  int progress = GetProgress();
  if (progress != m_lastProgress)
    CONTROL_SELECT_ITEM(POPUP_SEEK_PROGRESS, m_lastProgress = progress);

  CGUIDialog::FrameMove();
}

int CGUIDialogSeekBar::GetProgress() const
{
  const CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

  int progress = 0;

  if (CSeekHandler::GetInstance().GetSeekSize() != 0)
    infoMgr.GetInt(progress, PLAYER_SEEKBAR, INFO::DEFAULT_CONTEXT);
  else
    infoMgr.GetInt(progress, PLAYER_PROGRESS, INFO::DEFAULT_CONTEXT);

  return progress;
}
