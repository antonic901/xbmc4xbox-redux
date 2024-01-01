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

#include "GUIDialog.h"
#include "programs/windows/GUIWindowProgramBase.h"
#include "ThumbLoader.h"

class CFileItem;

class CGUIDialogProgramInfo :
      public CGUIDialog
{
public:
  CGUIDialogProgramInfo(void);
  virtual ~CGUIDialogProgramInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  void SetGame(const CFileItem *item);
  bool NeedRefresh() const;
  bool RefreshAll() const;
  bool HasUpdatedThumb() const { return m_hasUpdatedThumb; };

  const CStdString &GetThumbnail() const;
  virtual CFileItemPtr GetCurrentListItem(int offset = 0) { return m_gameItem; }
  const CFileItemList& CurrentDirectory() const { return *m_patchList; };
  virtual bool HasListItems() const { return true; };
protected:
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);
  PROGRAMDB_CONTENT_TYPE GetContentType(const CFileItem *pItem) const;

  // hold all XBEs witch name starts with 'patch_'
  void ClearPatchList();

  void Play(const CStdString& strPath = "");
  void OnGetThumb();
  void OnGetFanart();
  void PlayTrailer();

  CFileItemPtr m_gameItem;
  CFileItemList *m_patchList;
  bool m_bViewReview;
  bool m_bRefresh;
  bool m_bRefreshAll;
  bool m_hasUpdatedThumb;
  CGUIDialogProgress* m_dlgProgress;
  CProgramThumbLoader m_loader;
};
