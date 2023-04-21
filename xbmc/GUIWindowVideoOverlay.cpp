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

#include "GUIWindowVideoOverlay.h"
#include "GUIInfoManager.h"
#include "GUIWindowManager.h"

#define CONTROL_PLAYTIME     2
#define CONTROL_PLAY_LOGO    3
#define CONTROL_PAUSE_LOGO   4
#define CONTROL_INFO         5
#define CONTROL_BIG_PLAYTIME 6
#define CONTROL_FF_LOGO      7
#define CONTROL_RW_LOGO      8


CGUIWindowVideoOverlay::CGUIWindowVideoOverlay()
    : CGUIDialog(WINDOW_VIDEO_OVERLAY, "VideoOverlay.xml")
{
  m_renderOrder = 0;
  m_visibleCondition = SKIN_HAS_VIDEO_OVERLAY;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowVideoOverlay::~CGUIWindowVideoOverlay()
{}

void CGUIWindowVideoOverlay::FrameMove()
{
  if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
  { // close immediately
    Close(true);
    return;
  }
  CGUIDialog::FrameMove();
}

bool CGUIWindowVideoOverlay::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  return false;
}

void CGUIWindowVideoOverlay::SetDefaults()
{
  CGUIDialog::SetDefaults();
  m_renderOrder = 0;
  m_visibleCondition = SKIN_HAS_VIDEO_OVERLAY;
}

