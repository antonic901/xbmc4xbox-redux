/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ContextMenuItem.h"
#include "VideoLibraryQueue.h"
#include "media/MediaType.h"

#include <memory>

namespace CONTEXTMENU
{

class CVideoInfoBase : public CStaticContextMenuAction
{
public:
  explicit CVideoInfoBase(MediaType mediaType);
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;

private:
  const MediaType m_mediaType;
};

struct CVideoInfo : CVideoInfoBase
{
  CVideoInfo() : CVideoInfoBase(MediaTypeVideo) {}
  virtual bool IsVisible(const CFileItem& item) const;
};

struct CTVShowInfo : CVideoInfoBase
{
  CTVShowInfo() : CVideoInfoBase(MediaTypeTvShow) {}
};

struct CSeasonInfo : CVideoInfoBase
{
  CSeasonInfo() : CVideoInfoBase(MediaTypeSeason) {}
};

struct CEpisodeInfo : CVideoInfoBase
{
  CEpisodeInfo() : CVideoInfoBase(MediaTypeEpisode) {}
};

struct CMusicVideoInfo : CVideoInfoBase
{
  CMusicVideoInfo() : CVideoInfoBase(MediaTypeMusicVideo) {}
};

struct CMovieInfo : CVideoInfoBase
{
  CMovieInfo() : CVideoInfoBase(MediaTypeMovie) {}
};

struct CMovieSetInfo : CVideoInfoBase
{
  CMovieSetInfo() : CVideoInfoBase(MediaTypeVideoCollection) {}
};

struct CVideoRemoveResumePoint : CStaticContextMenuAction
{
  CVideoRemoveResumePoint() : CStaticContextMenuAction(38209) {}
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CVideoMarkWatched : CStaticContextMenuAction
{
  CVideoMarkWatched() : CStaticContextMenuAction(16103) {}
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CVideoMarkUnWatched : CStaticContextMenuAction
{
  CVideoMarkUnWatched() : CStaticContextMenuAction(16104) {}
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CVideoBrowse : CStaticContextMenuAction
{
  CVideoBrowse() : CStaticContextMenuAction(37015) {} // Browse into
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CVideoChooseVersion : CStaticContextMenuAction
{
  CVideoChooseVersion() : CStaticContextMenuAction(40221) {} // Choose version
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CVideoPlayVersionUsing : CStaticContextMenuAction
{
  CVideoPlayVersionUsing() : CStaticContextMenuAction(40209) {} // Play version using...
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& _item) const;
};

struct CVideoResume : IContextMenuItem
{
  virtual std::string GetLabel(const CFileItem& item) const;
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& _item) const;
};

struct CVideoPlay : IContextMenuItem
{
  virtual std::string GetLabel(const CFileItem& item) const;
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& _item) const;
};

struct CVideoPlayUsing : CStaticContextMenuAction
{
  CVideoPlayUsing() : CStaticContextMenuAction(15213) {} // Play using...
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& _item) const;
};

struct CVideoQueue : CStaticContextMenuAction
{
  CVideoQueue() : CStaticContextMenuAction(13347) {} // Queue item
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CVideoPlayNext : CStaticContextMenuAction
{
  CVideoPlayNext() : CStaticContextMenuAction(10008) {} // Play next
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CVideoPlayAndQueue : IContextMenuItem
{
  virtual std::string GetLabel(const CFileItem& item) const;
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

}
