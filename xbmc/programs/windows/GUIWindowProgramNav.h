#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "programs/windows/GUIWindowProgramBase.h"
#include "ThumbLoader.h"

class CFileItemList;

class CGUIWindowProgramNav : public CGUIWindowProgramBase
{
public:

  CGUIWindowProgramNav(void);
  virtual ~CGUIWindowProgramNav(void);

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  static bool CanDelete(const CStdString& strPath);
  static bool DeleteItem(CFileItem* pItem, bool bUnavailable=false);

protected:
  /*! \brief Load program information from the database for these items
   Useful for grabbing information for file listings, from played status to full metadata
   \param items the items to load information for.
   */
  void LoadProgramInfo(CFileItemList &items);

  virtual void OnItemLoaded(CFileItem* pItem) {};
  // override base class methods
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void OnDeleteItem(CFileItemPtr pItem);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual bool OnClick(int iItem);
  virtual CStdString GetStartFolder(const CStdString &dir);

  virtual CStdString GetQuickpathName(const CStdString& strPath) const;

  bool GetItemsForTag(const CStdString &strHeading, const std::string &type, CFileItemList &items, int idTag = -1, bool showAll = true);
  static CStdString GetLocalizedType(const std::string &strType);

  VECSOURCES m_shares;
};
