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

namespace CONTEXTMENU
{

struct CEjectDisk : CStaticContextMenuAction
{
  CEjectDisk() : CStaticContextMenuAction(13391) {} // Eject/Load CD/DVD!
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

struct CAddRemoveFavourite : IContextMenuItem
{
  CAddRemoveFavourite() {}
  virtual std::string GetLabel(const CFileItem& item) const;
  virtual bool IsVisible(const CFileItem& item) const;
  virtual bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

}
