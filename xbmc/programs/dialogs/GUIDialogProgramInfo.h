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
  void SetGame(const CFileItem *item);
  bool NeedRefresh() const;
  bool RefreshAll() const;
  bool HasUpdatedThumb() const { return m_hasUpdatedThumb; };

  const CStdString &GetThumbnail() const;
protected:
  PROGRAMDB_CONTENT_TYPE GetContentType(const CFileItem *pItem) const;

  CFileItemPtr m_gameItem;
  CFileItemList *m_patchList;
  bool m_bViewReview;
  bool m_bRefresh;
  bool m_bRefreshAll;
  bool m_hasUpdatedThumb;
  CGUIDialogProgress* m_dlgProgress;
  CProgramThumbLoader m_loader;
};
