/*
 *  Copyright (C) 2023-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

class CFileItem;
class CFileItemList;

class CGUIDialogProgramInfo :
      public CGUIDialog
{
public:
  CGUIDialogProgramInfo(void);
  virtual ~CGUIDialogProgramInfo(void);
  bool OnMessage(CGUIMessage& message);
  void SetProgram(const CFileItem *item);

protected:
  void OnInitWindow();
  void Update();
  void SetLabel(int iControl, const std::string& strLabel);

  // link screenshot to games
  void ClearScreenshotList();

  void PlayTrailer();

  boost::shared_ptr<CFileItem> m_programItem;
  CFileItemList *m_screenshotList;
};
