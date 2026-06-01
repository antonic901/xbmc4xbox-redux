/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileItemListModifier.h"

#include <string>

class CSmartPlaylistFileItemListModifier : public IFileItemListModifier
{
public:
  CSmartPlaylistFileItemListModifier() {}
  virtual ~CSmartPlaylistFileItemListModifier() {}

  virtual bool CanModify(const CFileItemList &items) const;
  virtual bool Modify(CFileItemList &items) const;

private:
  static std::string GetUrlOption(const std::string &path, const std::string &option);
};
