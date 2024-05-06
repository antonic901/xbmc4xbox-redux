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

#include "ThumbLoader.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "settings/Settings.h"
#include "TextureCache.h"
#include "Shortcut.h"
#include "Util.h"
#include "utils/URIUtils.h"

using namespace std;
using namespace XFILE;

CThumbLoader::CThumbLoader() :
  CBackgroundInfoLoader()
{
  m_textureDatabase = new CTextureDatabase();
}

CThumbLoader::~CThumbLoader()
{
  delete m_textureDatabase;
}

void CThumbLoader::OnLoaderStart()
{
  m_textureDatabase->Open();
}

void CThumbLoader::OnLoaderFinish()
{
  m_textureDatabase->Close();
}

CStdString CThumbLoader::GetCachedImage(const CFileItem &item, const CStdString &type)
{
  if (!item.GetPath().empty() && m_textureDatabase->Open())
  {
    CStdString image = m_textureDatabase->GetTextureForPath(item.GetPath(), type);
    m_textureDatabase->Close();
    return image;
  }
  return "";
}

void CThumbLoader::SetCachedImage(const CFileItem &item, const CStdString &type, const CStdString &image)
{
  if (!item.GetPath().empty() && m_textureDatabase->Open())
  {
    m_textureDatabase->SetTextureForPath(item.GetPath(), type, image);
    m_textureDatabase->Close();
  }
}

CProgramThumbLoader::CProgramThumbLoader()
{
}

CProgramThumbLoader::~CProgramThumbLoader()
{
}

bool CProgramThumbLoader::LoadItem(CFileItem *pItem)
{
  bool result  = LoadItemCached(pItem);
       result |= LoadItemLookup(pItem);

  return result;
}

bool CProgramThumbLoader::LoadItemCached(CFileItem *pItem)
{
  if (pItem->IsParentFolder())
    return false;

  return FillThumb(*pItem);
}

bool CProgramThumbLoader::LoadItemLookup(CFileItem *pItem)
{
  return false;
}

bool CProgramThumbLoader::FillThumb(CFileItem &item)
{
  // no need to do anything if we already have a thumb set
  CStdString thumb = item.GetArt("thumb");

  if (thumb.IsEmpty())
  { // see whether we have a cached image for this item
    CProgramThumbLoader loader;
    thumb = loader.GetCachedImage(item, "thumb");
    if (thumb.IsEmpty())
    {
      thumb = GetLocalThumb(item);
      if (!thumb.IsEmpty())
        loader.SetCachedImage(item, "thumb", thumb);
    }
  }

  if (!thumb.IsEmpty())
  {
    CTextureCache::Get().BackgroundCacheImage(thumb);
    item.SetArt("thumb", thumb);
  }
  return true;
}

CStdString CProgramThumbLoader::GetLocalThumb(const CFileItem &item)
{
  if (item.IsAddonsPath())
    return "";

  // look for the thumb
  if (item.IsShortCut())
  {
    CShortcut shortcut;
    if ( shortcut.Create( item.GetPath() ) )
    {
      // use the shortcut's thumb
      if (!shortcut.m_strThumb.IsEmpty())
        return shortcut.m_strThumb;
      else
      {
        CFileItem cut(shortcut.m_strPath,false);
        CProgramThumbLoader loader;
        if (loader.LoadItem(&cut))
          return cut.GetArt("thumb");
      }
    }
  }
#ifdef _XBOX
  else if (item.IsXBE())
  {
    CStdString directory = URIUtils::GetDirectory(item.GetPath());
    CStdString icon = URIUtils::AddFileToFolder(directory, "avalaunch_icon.jpg");

    // first check for avalaunch_icon.jpg
    if (CFile::Exists(icon) || CUtil::CacheXBEIcon(item.GetPath(), icon))
    {
      CFileItem item(icon,false);
      CProgramThumbLoader loader;
      if (loader.LoadItem(&item))
        return item.GetArt("thumb");
    }
  }
#endif
  else if (item.m_bIsFolder)
  {
    CStdString folderThumb = item.GetFolderThumb();
    if (CFile::Exists(folderThumb))
      return folderThumb;
  }
  else
  {
    CStdString fileThumb(item.GetTBNFile());
    if (CFile::Exists(fileThumb))
      return fileThumb;
  }
  return "";
}
