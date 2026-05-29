/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "view/GUIViewState.h"

class CGUIViewStateWindowMusic : public CGUIViewState
{
public:
  explicit CGUIViewStateWindowMusic(const CFileItemList& items) : CGUIViewState(items) {}
protected:
  virtual VECSOURCES& GetSources();
  virtual PLAYLIST::Id GetPlaylist() const;
  virtual bool AutoPlayNextItem();
  virtual std::string GetLockType();
  virtual std::string GetExtensions();
};

class CGUIViewStateMusicSearch : public CGUIViewStateWindowMusic
{
public:
  explicit CGUIViewStateMusicSearch(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateMusicDatabase : public CGUIViewStateWindowMusic
{
public:
  explicit CGUIViewStateMusicDatabase(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateMusicSmartPlaylist : public CGUIViewStateWindowMusic
{
public:
  explicit CGUIViewStateMusicSmartPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateMusicPlaylist : public CGUIViewStateWindowMusic
{
public:
  explicit CGUIViewStateMusicPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowMusicNav : public CGUIViewStateWindowMusic
{
public:
  explicit CGUIViewStateWindowMusicNav(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual VECSOURCES& GetSources();

private:
  void AddOnlineShares();
};

class CGUIViewStateWindowMusicPlaylist : public CGUIViewStateWindowMusic
{
public:
  explicit CGUIViewStateWindowMusicPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual PLAYLIST::Id GetPlaylist() const;
  virtual bool AutoPlayNextItem();
  virtual bool HideParentDirItems();
  virtual VECSOURCES& GetSources();
};
