#pragma once
/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "windows/GUIMediaWindow.h"
#include "ProgramDatabase.h"
#include "ThumbLoader.h"

class CGUIWindowPrograms : public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowPrograms(void);
  virtual ~CGUIWindowPrograms(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void OnItemLoaded(CFileItem* pItem) {};
  virtual bool Update(const std::string& strDirectory, bool updateFilterPath = true);
  virtual bool OnPlayMedia(int iItem, const std::string &player = "");
  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);
  virtual bool OnAddMediaSource();
  virtual std::string GetStartFolder(const std::string &dir);

  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);

  CProgramDatabase m_database;

  CProgramThumbLoader m_thumbLoader;
};
