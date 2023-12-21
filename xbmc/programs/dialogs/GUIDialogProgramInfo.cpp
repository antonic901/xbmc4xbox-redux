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

#include "programs/dialogs/GUIDialogProgramInfo.h"
#include "FileItem.h"

using namespace std;
using namespace XFILE;

CGUIDialogProgramInfo::CGUIDialogProgramInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PROGRAM_INFO, "DialogProgramInfo.xml")
    , m_gameItem(new CFileItem)
{
  m_bRefreshAll = true;
  m_bRefresh = false;
  m_hasUpdatedThumb = false;
  m_patchList = new CFileItemList;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogProgramInfo::~CGUIDialogProgramInfo(void)
{
  delete m_patchList;
}

void CGUIDialogProgramInfo::SetGame(const CFileItem *item)
{
  *m_gameItem = *item;
  PROGRAMDB_CONTENT_TYPE type = GetContentType(m_gameItem.get());
  // set fanart property for games
  if (m_gameItem->CacheLocalFanart())
    m_gameItem->SetProperty("fanart_image", m_gameItem->GetCachedFanart());
  if (type == PROGRAMDB_CONTENT_GAMES)
  {
    if (m_gameItem->GetProgramInfoTag()->m_strTrailer.IsEmpty())
    {
      m_gameItem->GetProgramInfoTag()->m_strTrailer = m_gameItem->FindTrailer();
      if (!m_gameItem->GetProgramInfoTag()->m_strTrailer.IsEmpty())
      {
        CProgramDatabase database;
        if(database.Open())
        {
          database.SetDetail(m_gameItem->GetProgramInfoTag()->m_strTrailer,
                              m_gameItem->GetProgramInfoTag()->m_iDbId,
                              PROGRAMDB_ID_TRAILER, PROGRAMDB_CONTENT_GAMES);
          database.Close();
          CUtil::DeleteProgramDatabaseDirectoryCache();
        }
      }
    }
  }
  m_loader.LoadItem(m_gameItem.get());
}

bool CGUIDialogProgramInfo::NeedRefresh() const
{
  return m_bRefresh;
}

bool CGUIDialogProgramInfo::RefreshAll() const
{
  return m_bRefreshAll;
}

PROGRAMDB_CONTENT_TYPE CGUIDialogProgramInfo::GetContentType(const CFileItem *pItem) const
{
  PROGRAMDB_CONTENT_TYPE type = PROGRAMDB_CONTENT_GAMES;
  return type;
}

const CStdString& CGUIDialogProgramInfo::GetThumbnail() const 
{
  return m_gameItem->GetThumbnailImage();
}
