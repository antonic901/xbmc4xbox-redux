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

class CFileItem;

namespace CONTEXTMENU
{

struct CAddonInfo : CStaticContextMenuAction
{
  CAddonInfo() : CStaticContextMenuAction(19033) {}
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CAddonSettings : CStaticContextMenuAction
{
  CAddonSettings() : CStaticContextMenuAction(10004) {}
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CCheckForUpdates : CStaticContextMenuAction
{
  CCheckForUpdates() : CStaticContextMenuAction(24034) {}
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CEnableAddon : CStaticContextMenuAction
{
  CEnableAddon() : CStaticContextMenuAction(24022) {}
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CDisableAddon : CStaticContextMenuAction
{
  CDisableAddon() : CStaticContextMenuAction(24021) {}
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};
}
