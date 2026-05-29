/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIWindowMusicBase.h"

class CFileItemList;

class CGUIWindowMusicPlaylistEditor : public CGUIWindowMusicBase
{
public:
  CGUIWindowMusicPlaylistEditor(void);
  virtual ~CGUIWindowMusicPlaylistEditor(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnClick(int iItem, const std::string &player = "");
  virtual bool OnBack(int actionID);

protected:
  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual bool Update(const std::string &strDirectory, bool updateFilterPath = true);
  virtual void OnPrepareFileItems(CFileItemList& items);
  virtual void OnQueueItem(int iItem, bool first = false);
  virtual std::string GetStartFolder(const std::string& dir) { return ""; }

  void OnSourcesContext();
  void OnPlaylistContext();
  int GetCurrentPlaylistItem();
  void OnDeletePlaylistItem(int item);
  void UpdatePlaylist();
  void ClearPlaylist();
  void OnSavePlaylist();
  void OnLoadPlaylist();
  void AppendToPlaylist(CFileItemList &newItems);
  void OnMovePlaylistItem(int item, int direction);

  void LoadPlaylist(const std::string &playlist);

  // new method
  virtual void PlayItem(int iItem);

  void DeleteRemoveableMediaDirectoryCache();

  CMusicThumbLoader m_playlistThumbLoader;

  CFileItemList* m_playlist;
  std::string m_strLoadedPlaylist;
};
