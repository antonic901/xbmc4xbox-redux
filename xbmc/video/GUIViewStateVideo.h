/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "view/GUIViewState.h"

class CGUIViewStateWindowVideo : public CGUIViewState
{
public:
  explicit CGUIViewStateWindowVideo(const CFileItemList& items) : CGUIViewState(items) {}

protected:
  virtual VECSOURCES& GetSources();
  virtual std::string GetLockType();
  virtual PLAYLIST::Id GetPlaylist() const;
  virtual std::string GetExtensions();
  virtual bool AutoPlayNextItem();
};

class CGUIViewStateVideoPlaylist : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateVideoPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowVideoNav : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateWindowVideoNav(const CFileItemList& items);
  virtual bool AutoPlayNextItem();

protected:
  virtual void SaveViewState();
  virtual VECSOURCES& GetSources();
};

class CGUIViewStateWindowVideoPlaylist : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateWindowVideoPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual bool HideExtensions();
  virtual bool HideParentDirItems();
  virtual VECSOURCES& GetSources();
  virtual bool AutoPlayNextItem() { return false; }
};

class CGUIViewStateVideoMovies : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateVideoMovies(const CFileItemList& items);
protected:
  virtual void SaveViewState();
};

class CGUIViewStateVideoMusicVideos : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateVideoMusicVideos(const CFileItemList& items);
protected:
  virtual void SaveViewState();
};

class CGUIViewStateVideoTVShows : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateVideoTVShows(const CFileItemList& items);
protected:
  virtual void SaveViewState();
};

class CGUIViewStateVideoEpisodes : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateVideoEpisodes(const CFileItemList& items);
protected:
  virtual void SaveViewState();
};

