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
#include "ProgramDatabase.h"
#include "dialogs/GUIDialogProgress.h"
#include "ThumbLoader.h"

class CGUIWindowPrograms :
      public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowPrograms(void);
  virtual ~CGUIWindowPrograms(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void OnItemLoaded(CFileItem* pItem) {};
  virtual bool Update(const std::string& strDirectory, bool updateFilterPath = true);
  virtual bool OnPlayMedia(int iItem, const std::string &player = "");
  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual bool OnAddMediaSource();
  virtual std::string GetStartFolder(const std::string &dir);

  int GetRegion(int iItem, bool bReload=false);
  bool OnChooseVideoModeAndLaunch(int iItem);

  CGUIDialogProgress* m_dlgProgress;

  CProgramDatabase m_database;

  int m_iRegionSet; // for cd stuff

  CProgramThumbLoader m_thumbLoader;
};
