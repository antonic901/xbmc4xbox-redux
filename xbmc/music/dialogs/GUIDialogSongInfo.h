#pragma once

/*
 *      Copyright (C) 2005-2018 Team XBMC
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

#include "guilib/GUIDialog.h"
#include "FileItem.h"
#include "threads/Event.h"

class CGUIDialogSongInfo :
      public CGUIDialog
{
public:
  CGUIDialogSongInfo(void);
  virtual ~CGUIDialogSongInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  bool SetSong(CFileItem* item);
  void SetArtTypeList(CFileItemList& artlist);
  bool OnAction(const CAction& action);
  virtual bool OnBack(int actionID);
  bool HasUpdatedUserrating() const { return m_hasUpdatedUserrating; };

  virtual bool HasListItems() const { return true; };
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
  std::string GetContent();
  //const CFileItemList& CurrentDirectory() const { return m_artTypeList; };
  bool IsCancelled() const { return m_cancelled; };
  void FetchComplete();

  static void ShowFor(CFileItem* pItem);
protected:
  virtual void OnInitWindow();
  void Update();
  void OnGetArt();
  void SetUserrating(int userrating);
  void OnSetUserrating();

  CFileItemPtr m_song;
  CFileItemList m_artTypeList;
  CEvent m_event;
  int m_startUserrating;
  bool m_cancelled;
  bool m_hasUpdatedUserrating;
  long m_albumId;
};
