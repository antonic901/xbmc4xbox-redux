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

#include "system.h"
#include "PlaylistDirectory.h"
#include "settings/Settings.h"
#include "filesystem/HDDirectory.h"
#include "playlists/PlayListFactory.h"
#include "Util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace XFILE;
using namespace PLAYLIST;

CPlayListDirectory::CPlayListDirectory(void)
{}

CPlayListDirectory::~CPlayListDirectory(void)
{}



bool CPlayListDirectory::GetDirectory(const CStdString& strPath, VECFILEITEMS &items)
{
  if ( !CUtil::IsPlayList(strPath) )
  {
    CHDDirectory dirLoader;
    dirLoader.SetMask(".m3u|.b4s|.pls|.strm");
    VECFILEITEMS tmpitems;
    CStdString strDir = CSettings::Get().m_szAlbumDirectory;
    strDir += "\\playlists";
    dirLoader.GetDirectory(strDir, tmpitems);

    // for each playlist found
    for (int i = 0; i < (int)tmpitems.size(); ++i)
    {
      CFileItem* pItem = tmpitems[i];
      CStdString strPlayListName, strPlayList;
      strPlayList = URIUtils::GetFileName( pItem->GetPath() );
      strPlayListName = URIUtils::GetFileName( strPlayList );
      delete pItem;

      CPlayListFactory factory;
      CPlayList* pPlayList = factory.Create(strPlayList);
      if (pPlayList)
      {
        if ( pPlayList->Load(strPlayList) )
        {
          CStdString strPlayListName = pPlayList->GetName();
          if (strPlayListName.size())
          {
            strPlayListName = strPlayListName;
          }
        }
        delete pPlayList;
      }
    }
    CUtil::SetThumbs(items);
    CUtil::FillInDefaultIcons(items);

    return true;
  }

  // yes, first add parent path
  {
    CFileItem *pItem = new CFileItem("..");
    pItem->SetPath("");
    pItem->m_bIsFolder = true;
    pItem->m_bIsShareOrDrive = false;
    items.push_back(pItem);
  }

  // open the playlist
  CPlayListFactory factory;
  CPlayList* pPlayList = factory.Create(strPath);
  if (!pPlayList) return false;

  if ( pPlayList->Load(strPath) )
  {

    for (int i = 0; i < (int)pPlayList->size(); ++i)
    {
      const CPlayList::CPlayListItem& playlistItem = (*pPlayList)[i];

      CStdString strLabel;
      strLabel = URIUtils::GetFileName( playlistItem.GetFileName() );
      CFileItem* pItem = new CFileItem(strLabel);
      pItem->SetPath(playlistItem.GetFileName());
      pItem->m_bIsFolder = false;
      pItem->m_bIsShareOrDrive = false;
      items.push_back(pItem);
    }
  }
  delete pPlayList;
  CUtil::SetThumbs(items);
  CUtil::FillInDefaultIcons(items);

  return true;
}

