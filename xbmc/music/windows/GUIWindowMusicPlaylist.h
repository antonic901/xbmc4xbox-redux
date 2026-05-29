/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIWindowMusicBase.h"

class CGUIWindowMusicPlayList : public CGUIWindowMusicBase
{
public:
  CGUIWindowMusicPlayList(void);
  virtual ~CGUIWindowMusicPlayList(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnBack(int actionID);

  void RemovePlayListItem(int iItem);
  void MoveItem(int iStart, int iDest);

protected:
  virtual bool GoParentFolder() { return false; }
  virtual void UpdateButtons();
  virtual void OnItemLoaded(CFileItem* pItem);
  virtual bool Update(const std::string& strDirectory, bool updateFilterPath = true);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  void OnMove(int iItem, int iAction);
  virtual bool OnPlayMedia(int iItem, const std::string &player = "");

  void SavePlayList();
  void ClearPlayList();
  void MarkPlaying();

  bool MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate = true);

  int m_movingFrom;
  VECSOURCES m_shares;
};
