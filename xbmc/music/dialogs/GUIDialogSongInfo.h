/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "guilib/GUIDialog.h"
#include "threads/Event.h"

#include <memory>

class CGUIDialogSongInfo :
      public CGUIDialog
{
public:
  CGUIDialogSongInfo(void);
  virtual ~CGUIDialogSongInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  bool SetSong(CFileItem* item);
  void SetArtTypeList(CFileItemList& artlist);
  virtual bool OnAction(const CAction& action);
  virtual bool OnBack(int actionID);
  bool HasUpdatedUserrating() const { return m_hasUpdatedUserrating; }

  virtual bool HasListItems() const { return true; }
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
  std::string GetContent();
  //const CFileItemList& CurrentDirectory() const { return m_artTypeList; }
  bool IsCancelled() const { return m_cancelled; }
  void FetchComplete();

  static void ShowFor(CFileItem* pItem);
protected:
  virtual void OnInitWindow();
  void Update();
  void OnGetArt();
  void SetUserrating(int userrating);
  void OnSetUserrating();
  void OnPlaySong(const boost::shared_ptr<CFileItem>& item);

  CFileItemPtr m_song;
  CFileItemList m_artTypeList;
  CEvent m_event;
  int m_startUserrating;
  bool m_cancelled;
  bool m_hasUpdatedUserrating;
  long m_albumId = -1;

};
