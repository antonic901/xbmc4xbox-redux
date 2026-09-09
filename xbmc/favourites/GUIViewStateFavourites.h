/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "view/GUIViewState.h"

class CFileItemList;

class CGUIViewStateFavourites : public CGUIViewState
{
public:
  CGUIViewStateFavourites(const CFileItemList& items);
  virtual ~CGUIViewStateFavourites() {}

protected:
  virtual void SaveViewState();
  virtual bool HideParentDirItems() { return true; };
};
