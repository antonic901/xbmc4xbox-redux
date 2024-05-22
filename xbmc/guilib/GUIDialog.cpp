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

#include "include.h"
#include "GUIDialog.h"
#include "GUIWindowManager.h"
#include "GUILabelControl.h"
#include "GUIAudioManager.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"
#include "Application.h"
#include "ApplicationMessenger.h"

CGUIDialog::CGUIDialog(int id, const CStdString &xmlFile)
    : CGUIWindow(id, xmlFile)
{
  m_bModal = true;
  m_bRunning = false;
  m_dialogClosing = false;
  m_renderOrder = 1;
  m_autoClosing = false;
  m_showStartTime = 0;
  m_showDuration = 0;
  m_enableSound = true;
  m_bAutoClosed = false;
}

CGUIDialog::~CGUIDialog(void)
{}

void CGUIDialog::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();

  // Clip labels to extents
  if (m_children.size())
  {
    CGUIControl* pBase = m_children[0];

    for (iControls p = m_children.begin() + 1; p != m_children.end(); ++p)
    {
      if ((*p)->GetControlType() == CGUIControl::GUICONTROL_LABEL)
      {
        CGUILabelControl* pLabel = (CGUILabelControl*)(*p);

        if (!pLabel->GetWidth())
        {
          float spacing = (pLabel->GetXPosition() - pBase->GetXPosition()) * 2;
          pLabel->SetWidth(pBase->GetWidth() - spacing);
        }
      }
    }
  }
}

bool CGUIDialog::OnAction(const CAction &action)
{
  // keyboard or controller movement should prevent autoclosing
  if (!action.IsMouse() && m_autoClosing)
    SetAutoClose(m_showDuration);

  return CGUIWindow::OnAction(action);
}

bool CGUIDialog::OnBack(int actionID)
{
  Close();
  return true;
}

bool CGUIDialog::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow::OnMessage(message);
      // if we were running, make sure we remove ourselves from the window manager
      if (m_bRunning)
      {
        g_windowManager.RemoveDialog(GetID());
        m_bRunning = false;
        m_dialogClosing = false;
        m_autoClosing = false;
      }
      return true;
    }
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      m_showStartTime = 0;
      return true;
    }
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIDialog::Close_Internal(bool forceClose /*= false*/)
{
  //Lock graphic context here as it is sometimes called from non rendering threads
  //maybe we should have a critical section per window instead??
  CSingleLock lock(g_graphicsContext);

  if (!m_bRunning) return;

  //  Play the window specific deinit sound
  if(!m_dialogClosing && m_enableSound)
    g_audioManager.PlayWindowSound(GetID(), SOUND_DEINIT);

  // don't close if we should be animating
  if (!forceClose && HasAnimation(ANIM_TYPE_WINDOW_CLOSE))
  {
    if (!m_dialogClosing && !IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
    {
      QueueAnimation(ANIM_TYPE_WINDOW_CLOSE);
      m_dialogClosing = true;
    }
    return;
  }

  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
  OnMessage(msg);
}

void CGUIDialog::DoModal_Internal(int iWindowID /*= WINDOW_INVALID */, const CStdString &param /* = "" */)
{
  //Lock graphic context here as it is sometimes called from non rendering threads
  //maybe we should have a critical section per window instead??
  CSingleLock lock(g_graphicsContext);

  if (!g_windowManager.Initialized())
    return; // don't do anything

  m_dialogClosing = false;
  m_bModal = true;
  // set running before it's added to the window manager, else the auto-show code
  // could show it as well if we are in a different thread from
  // the main rendering thread (this should really be handled via
  // a thread message though IMO)
  m_bRunning = true;
  g_windowManager.RouteToWindow(this);

  //  Play the window specific init sound
  if (m_enableSound)
    g_audioManager.PlayWindowSound(GetID(), SOUND_INIT);

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, iWindowID);
  msg.SetStringParam(param);
  OnMessage(msg);

//  m_bRunning = true;

  if (!m_windowLoaded)
    Close(true);

  lock.Leave();

  while (m_bRunning && !g_application.m_bStop)
  {
    g_windowManager.ProcessRenderLoop();
  }
}

void CGUIDialog::Show_Internal()
{
  //Lock graphic context here as it is sometimes called from non rendering threads
  //maybe we should have a critical section per window instead??
  CSingleLock lock(g_graphicsContext);

  if (m_bRunning && !m_dialogClosing && !IsAnimating(ANIM_TYPE_WINDOW_CLOSE)) return;

  if (!g_windowManager.Initialized())
    return; // don't do anything

  m_bModal = false;

  // set running before it's added to the window manager, else the auto-show code
  // could show it as well if we are in a different thread from
  // the main rendering thread (this should really be handled via
  // a thread message though IMO)
  m_bRunning = true;
  m_dialogClosing = false;
  g_windowManager.AddModeless(this);

  //  Play the window specific init sound
  if (m_enableSound)
    g_audioManager.PlayWindowSound(GetID(), SOUND_INIT);

  // active this window...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0);
  OnMessage(msg);

//  m_bRunning = true;
}

void CGUIDialog::Close(bool forceClose /* = false */)
{
  if (!g_application.IsCurrentThread())
  {
    // make sure graphics lock is not held
    CSingleExit leaveIt(g_graphicsContext);
    CApplicationMessenger::Get().Close(this, forceClose);
  }
  else
    CApplicationMessenger::Get().Close(this, forceClose);
}

void CGUIDialog::DoModal(int iWindowID /*= WINDOW_INVALID */, const CStdString &param)
{
  CApplicationMessenger::Get().DoModal(this, iWindowID, param);
}

void CGUIDialog::Show()
{
  CApplicationMessenger::Get().Show(this);
}

void CGUIDialog::FrameMove()
{
  if (m_autoClosing)
  { // check if our timer is running
    if (!m_showStartTime)
    {
      if (HasRendered()) // start timer
        m_showStartTime = CTimeUtils::GetFrameTime();
    }
    else
    {
      if (m_showStartTime + m_showDuration < CTimeUtils::GetFrameTime() && !m_dialogClosing)
      {
        m_bAutoClosed = true;
        Close();
      }
    }
  }
  CGUIWindow::FrameMove();
}

void CGUIDialog::Render()
{
  if (!m_bRunning)
    return;

  CGUIWindow::Render();
  // Check to see if we should close at this point
  // We check after the controls have finished rendering, as we may have to close due to
  // the controls rendering after the window has finished it's animation
  // we call the base class instead of this class so that we can find the change
  if (m_dialogClosing && !CGUIWindow::IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
  {
    Close(true);
  }
}

bool CGUIDialog::IsAnimating(ANIMATION_TYPE animType)
{
  if (animType == ANIM_TYPE_WINDOW_CLOSE)
    return m_dialogClosing;
  return CGUIWindow::IsAnimating(animType);
}

void CGUIDialog::SetDefaults()
{
  CGUIWindow::SetDefaults();
  m_renderOrder = 1;
}

void CGUIDialog::SetAutoClose(unsigned int timeoutMs)
{
   m_autoClosing = true;
   m_showDuration = timeoutMs;
   ResetAutoClose();
}

void CGUIDialog::ResetAutoClose(void)
{
  if (m_autoClosing && m_bRunning)
    m_showStartTime = CTimeUtils::GetFrameTime();
}

