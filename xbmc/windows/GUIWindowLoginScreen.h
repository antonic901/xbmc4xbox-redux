/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "utils/Stopwatch.h"
#include "view/GUIViewControl.h"

class CFileItemList;

class CGUIWindowLoginScreen : public CGUIWindow
{
public:
  CGUIWindowLoginScreen(void);
  virtual ~CGUIWindowLoginScreen(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnBack(int actionID);
  virtual void FrameMove();
  virtual bool HasListItems() const { return true; }
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
  virtual int GetViewContainerID() const { return m_viewControl.GetCurrentControl(); }

protected:
  virtual void OnInitWindow();
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  void Update();
  void SetLabel(int iControl, const std::string& strLabel);

  bool OnPopupMenu(int iItem);
  CGUIViewControl m_viewControl;
  CFileItemList* m_vecItems;

  int m_iSelectedItem;
  CStopWatch watch;
};
