/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileItemListModifier.h"

#include <set>

class CFileItemListModification : public IFileItemListModifier
{
public:
  virtual ~CFileItemListModification();

  static CFileItemListModification& GetInstance();

  virtual bool CanModify(const CFileItemList &items) const;
  virtual bool Modify(CFileItemList &items) const;

private:
  CFileItemListModification();
  CFileItemListModification(const CFileItemListModification&);
  CFileItemListModification& operator=(CFileItemListModification const&);

  std::set<IFileItemListModifier*> m_modifiers;
};
