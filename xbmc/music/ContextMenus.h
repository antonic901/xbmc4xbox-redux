/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ContextMenuItem.h"
#include "media/MediaType.h"

#include <memory>

class CFileItem;

namespace CONTEXTMENU
{

struct CMusicInfoBase : CStaticContextMenuAction
{
  explicit CMusicInfoBase(MediaType mediaType);
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;

private:
  const MediaType m_mediaType;
};

struct CMusicInfo : CMusicInfoBase
{
  CMusicInfo() : CMusicInfoBase(MediaTypeMusic) {}
  virtual bool IsVisible(const CFileItem& item) const;
};

struct CAlbumInfo : CMusicInfoBase
{
  CAlbumInfo() : CMusicInfoBase(MediaTypeAlbum) {}
};

struct CArtistInfo : CMusicInfoBase
{
  CArtistInfo() : CMusicInfoBase(MediaTypeArtist) {}
};

struct CSongInfo : CMusicInfoBase
{
  CSongInfo() : CMusicInfoBase(MediaTypeSong) {}
};

struct CMusicBrowse : CStaticContextMenuAction
{
  CMusicBrowse() : CStaticContextMenuAction(37015) {} // Browse into
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CMusicPlay : CStaticContextMenuAction
{
  CMusicPlay() : CStaticContextMenuAction(208) {} // Play
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CMusicPlayUsing : CStaticContextMenuAction
{
  CMusicPlayUsing() : CStaticContextMenuAction(15213) {} // Play using...
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& _item) const;
};

struct CMusicPlayNext : CStaticContextMenuAction
{
  CMusicPlayNext() : CStaticContextMenuAction(10008) {} // Play next
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CMusicQueue : CStaticContextMenuAction
{
  CMusicQueue() : CStaticContextMenuAction(13347) {} // Queue item
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

} // namespace CONTEXTMENU
