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

#include "windows/GUIMediaWindow.h"
#include "programs/ProgramDatabase.h"
#include "ThumbLoader.h"

class CGUIWindowProgramBase : public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowProgramBase(int id, const CStdString &xmlFile);
  virtual ~CGUIWindowProgramBase(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  static void OnScan(const CStdString& strPath, bool scanAll = false);
  virtual void OnInfo(CFileItem* pItem, const ADDON::ScraperPtr& scraper);

  /*! \brief Prompt the user for assigning content to a path.
   Based on changes, we then call OnUnassignContent, update or refresh scraper information in the database
   and optionally start a scan
   \param path the path to assign content for
   */
  static void OnAssignContent(const CStdString &path);

protected:
  virtual bool Update(const CStdString &strDirectory, bool updateFilterPath = true);
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void OnItemLoaded(CFileItem* pItem) {};

  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual CStdString GetStartFolder(const CStdString &dir);

  virtual CStdString GetQuickpathName(const CStdString& strPath) const {return strPath;};

  bool OnClick(int iItem);

  int GetScraperForItem(CFileItem *item, ADDON::ScraperPtr &info, PROGRAM::SScanSettings& settings);

  static bool OnUnAssignContent(const CStdString &path, int label1, int label2, int label3);

  CGUIDialogProgress* m_dlgProgress;
  CProgramDatabase m_database;

  CProgramThumbLoader m_thumbLoader;
  bool m_stackingAvailable;
};
