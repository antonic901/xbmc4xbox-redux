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

#include "MemUnitDirectory.h"

#include "FileItem.h"
#include "URL.h"
#include "filesystem/DirectoryCache.h"

#include "platform/xbox/filesystem/IDevice.h"
#include "platform/xbox/filesystem/IFileSystem.h"
#include "platform/xbox/filesystem/MemoryUnitManager.h"

using namespace XFILE;

CMemUnitDirectory::CMemUnitDirectory(void)
{}

CMemUnitDirectory::~CMemUnitDirectory(void)
{}

bool CMemUnitDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string strPath = url.Get();
  IFileSystem *fileSystem = GetFileSystem(strPath);
  if (!fileSystem) return false;

  g_directoryCache.ClearDirectory(strPath);
  CFileItemList cacheItems;
  if (!fileSystem->GetDirectory(strPath.substr(7), cacheItems))
  {
    delete fileSystem;
    return false;
  }

  for (int i = 0; i < cacheItems.Size(); i++)
  {
    CFileItemPtr item = cacheItems[i];
    if (item->m_bIsFolder || IsAllowed(item->GetURL()))
      items.Add(item);
  }

  delete fileSystem;
  return true;
}

bool CMemUnitDirectory::Create(const CURL& url)
{
  std::string strPath = url.Get();
  IFileSystem *fileSystem = GetFileSystem(strPath);
  if (!fileSystem) return false;
  return fileSystem->MakeDir(strPath.c_str() + 7);
}

bool CMemUnitDirectory::Remove(const CURL& url)
{
  std::string strPath = url.Get();
  IFileSystem *fileSystem = GetFileSystem(strPath);
  if (!fileSystem) return false;
  return fileSystem->RemoveDir(strPath.c_str() + 7);
}

bool CMemUnitDirectory::Exists(const CURL& url)
{
  CFileItemList items;
  if (GetDirectory(url, items))
    return true;
  return false;
}

IFileSystem *CMemUnitDirectory::GetFileSystem(const std::string &path)
{
  // format is mem#://folder/file
  if (path.substr(0, 3) != "mem" || path.size() < 7)
    return NULL;

  char unit = path[3] - '0';

  return g_memoryUnitManager.GetFileSystem(unit);
}
