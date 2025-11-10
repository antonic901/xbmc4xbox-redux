/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "ContextMenuItem.h"

class FileItem;

namespace CONTEXTMENU
{

class CProgramInfoBase : public CStaticContextMenuAction
{
public:
  CProgramInfoBase();
  bool IsVisible(const CFileItem& item) const;
  bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

class CProgramSettings : public CStaticContextMenuAction
{
public:
  CProgramSettings();
  bool IsVisible(const CFileItem& item) const;
  bool Execute(const boost::shared_ptr<CFileItem>& item) const;
};

class CScriptLaunch : public CStaticContextMenuAction
{
public:
  CScriptLaunch();
  bool IsVisible(const CFileItem& item) const;
  bool Execute(const boost::shared_ptr<CFileItem>& _item) const;
};

class CScraperConfig : public CStaticContextMenuAction
{
public:
  CScraperConfig();
  bool IsVisible(const CFileItem& item) const;
  bool Execute(const boost::shared_ptr<CFileItem>& _item) const;
};

class CContentScan : public CStaticContextMenuAction
{
public:
  CContentScan();
  bool IsVisible(const CFileItem& item) const;
  bool Execute(const boost::shared_ptr<CFileItem>& _item) const;
};
}
