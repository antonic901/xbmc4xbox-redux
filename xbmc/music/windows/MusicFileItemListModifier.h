/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileItemListModifier.h"

class CMusicFileItemListModifier : public IFileItemListModifier
{
public:
  CMusicFileItemListModifier() {}
  virtual ~CMusicFileItemListModifier() {}

  virtual bool CanModify(const CFileItemList &items) const;
  virtual bool Modify(CFileItemList &items) const;

private:
  static void AddQueuingFolder(CFileItemList & items);
};
