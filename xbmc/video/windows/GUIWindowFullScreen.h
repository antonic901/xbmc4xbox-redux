/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

#include <chrono>

class CGUIDialog;

class CGUIWindowFullScreen : public CGUIWindow
{
public:
  CGUIWindowFullScreen();
  virtual ~CGUIWindowFullScreen(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void ClearBackground();
  virtual void FrameMove();
  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregion);
  virtual void Render();
  virtual void RenderEx();
  virtual void OnWindowLoaded();
  virtual bool HasVisibleControls();

protected:
  virtual EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event);

private:
  void SeekChapter(int iChapter);
  void ToggleOSD();
  void TriggerOSD();
  CGUIDialog *GetOSD();

  bool m_viewModeChanged;
  std::chrono::time_point<std::chrono::steady_clock> m_dwShowViewModeTimeout;

  bool m_bShowCurrentTime;
};
