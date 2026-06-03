/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIWindowVideoBase.h"

class CFileItemList;

namespace SelectFirstUnwatchedItem
{
  enum Type
  {
    NEVER = 0,
    ON_FIRST_ENTRY = 1,
    ALWAYS = 2
  };
}

namespace IncludeAllSeasonsAndSpecials
{
  enum Type
  {
    NEITHER = 0,
    BOTH = 1,
    ALL_SEASONS = 2,
    SPECIALS = 3
  };
}

class CGUIWindowVideoNav : public CGUIWindowVideoBase
{
public:

  CGUIWindowVideoNav(void);
  virtual ~CGUIWindowVideoNav(void);

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  bool ApplyWatchedFilter(CFileItemList &items);
  virtual bool GetFilteredItems(const std::string &filter, CFileItemList &items);

  virtual void OnItemLoaded(CFileItem* pItem) {};

  // override base class methods
  virtual bool Update(const std::string &strDirectory, bool updateFilterPath = true);
  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual void DoSearch(const std::string& strSearch, CFileItemList& items);
  virtual void OnDeleteItem(const CFileItemPtr& pItem);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnPopupMenu(int iItem);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual bool OnAddMediaSource();
  virtual bool OnClick(int iItem, const std::string &player = "");
  virtual std::string GetStartFolder(const std::string &dir);

  VECSOURCES m_shares;

private:
  virtual SelectFirstUnwatchedItem::Type GetSettingSelectFirstUnwatchedItem();
  virtual IncludeAllSeasonsAndSpecials::Type GetSettingIncludeAllSeasonsAndSpecials();
  virtual int GetFirstUnwatchedItemIndex(bool includeAllSeasons, bool includeSpecials);
  void SelectFirstUnwatched();
};
