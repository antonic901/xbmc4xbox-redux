/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "favourites/FavouritesService.h"
#include "windows/GUIMediaWindow.h"

class CGUIWindowFavourites : public CGUIMediaWindow
{
public:
  CGUIWindowFavourites();
  virtual ~CGUIWindowFavourites();

protected:
  virtual std::string GetRootPath() const { return "favourites://"; }

  virtual bool OnSelect(int item);
  virtual bool OnAction(const CAction& action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual bool Update(const std::string& strDirectory, bool updateFilterPath = true);

private:
  void OnFavouritesEvent(const CFavouritesService::FavouritesUpdated& event);
  bool MoveItem(int item, int amount);
  bool RemoveItem(int item);
};
