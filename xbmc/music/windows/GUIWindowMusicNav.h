/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIWindowMusicBase.h"
#include "utils/Stopwatch.h"

class CFileItemList;

class CGUIWindowMusicNav : public CGUIWindowMusicBase
{
public:

  CGUIWindowMusicNav(void);
  virtual ~CGUIWindowMusicNav(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  virtual void FrameMove();

protected:
  virtual void OnItemLoaded(CFileItem* pItem) {};
  // override base class methods
  virtual bool Update(const std::string &strDirectory, bool updateFilterPath = true);
  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual void PlayItem(int iItem);
  virtual void OnWindowLoaded();
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnPopupMenu(int iItem);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual bool OnClick(int iItem, const std::string &player = "");
  virtual std::string GetStartFolder(const std::string &url);

  bool GetSongsFromPlayList(const std::string& strPlayList, CFileItemList &items);
  bool ManageInfoProvider(const CFileItemPtr& item);

  VECSOURCES m_shares;

  // searching
  void OnSearchUpdate();
  void AddSearchFolder();
  CStopWatch m_searchTimer; ///< Timer to delay a search while more characters are entered
  bool m_searchWithEdit;    ///< Whether the skin supports the new edit control searching
};
