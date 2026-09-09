/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ContextMenuItem.h"

#include <memory>

class CFileItemList;

namespace CONTEXTMENU
{

class CFavouriteContextMenuAction : public CStaticContextMenuAction
{
public:
  explicit CFavouriteContextMenuAction(uint32_t label) : CStaticContextMenuAction(label) {}
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;

protected:
  virtual ~CFavouriteContextMenuAction() {}
  virtual bool DoExecute(CFileItemList& items, const boost::shared_ptr<CFileItem>& item) const = 0;
};

class CMoveUpFavourite : public CFavouriteContextMenuAction
{
public:
  CMoveUpFavourite() : CFavouriteContextMenuAction(13332) {} // Move up
  virtual bool IsVisible(const CFileItem& item) const;

protected:
  virtual bool DoExecute(CFileItemList& items, const boost::shared_ptr<CFileItem>& item) const;
};

class CMoveDownFavourite : public CFavouriteContextMenuAction
{
public:
  CMoveDownFavourite() : CFavouriteContextMenuAction(13333) {} // Move down
  virtual bool IsVisible(const CFileItem& item) const;

protected:
  virtual bool DoExecute(CFileItemList& items, const boost::shared_ptr<CFileItem>& item) const;
};

class CRemoveFavourite : public CFavouriteContextMenuAction
{
public:
  CRemoveFavourite() : CFavouriteContextMenuAction(15015) {} // Remove
protected:
  virtual bool DoExecute(CFileItemList& items, const boost::shared_ptr<CFileItem>& item) const;
};

class CRenameFavourite : public CFavouriteContextMenuAction
{
public:
  CRenameFavourite() : CFavouriteContextMenuAction(118) {} // Rename
protected:
  virtual bool DoExecute(CFileItemList& items, const boost::shared_ptr<CFileItem>& item) const;
};

class CChooseThumbnailForFavourite : public CFavouriteContextMenuAction
{
public:
  CChooseThumbnailForFavourite() : CFavouriteContextMenuAction(20019) {} // Choose thumbnail
protected:
  virtual bool DoExecute(CFileItemList& items, const boost::shared_ptr<CFileItem>& item) const;
};

class CFavouritesTargetBrowse : public CStaticContextMenuAction
{
public:
  explicit CFavouritesTargetBrowse() : CStaticContextMenuAction(37015) {} // Browse into
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

class CFavouritesTargetResume : public IContextMenuItem
{
public:
  virtual std::string GetLabel(const CFileItem& item) const;
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

class CFavouritesTargetPlay : public IContextMenuItem
{
public:
  virtual std::string GetLabel(const CFileItem& item) const;
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

class CFavouritesTargetInfo : public CStaticContextMenuAction
{
public:
  explicit CFavouritesTargetInfo() : CStaticContextMenuAction(19033) {} // Information
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

class CFavouritesTargetContextMenu : public CStaticContextMenuAction
{
public:
  explicit CFavouritesTargetContextMenu() : CStaticContextMenuAction(22082) {} // More...
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

} // namespace CONTEXTMENU
