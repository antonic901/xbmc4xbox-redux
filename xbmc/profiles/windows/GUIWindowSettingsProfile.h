/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

class CFileItemList;

class CGUIWindowSettingsProfile :
      public CGUIWindow
{
public:
  CGUIWindowSettingsProfile(void);
  virtual ~CGUIWindowSettingsProfile(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void OnInitWindow();
  CFileItemList *m_listItems;

  void OnPopupMenu(int iItem);
  void DoRename(int iItem);
  void DoOverwrite(int iItem);
  int GetSelectedItem();
  void LoadList();
  void SetLastLoaded();
  void ClearListItems();
  bool GetAutoLoginProfileChoice(int &iProfile);
};
