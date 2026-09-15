/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HDDirectory.h"
#include "FileItem.h"
#include "URL.h"
#include "Util.h"

#include <xtl.h>

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD) - 1)
#endif

using namespace XFILE;

// check for empty string, remove trailing slash if any, convert to xbox form
inline static std::string prepareXboxDirectoryName(const std::string& strPath)
{
  if (strPath.empty())
    return std::string(); // empty string

  std::string name(CUtil::GetFatXQualifiedPath(strPath));
  if (!name.empty())
  {
    if (name[name.size() - 1] == '\\')
      name.erase(name.size() - 1); // remove slash at the end if any
    if (name.length() == 6 && name[name.size() - 1] == ':') // 6 is the length of "\\?\x:"
      name.push_back('\\'); // always add backslash for root folders
  }
  return name;
}

CHDDirectory::CHDDirectory(void)
{
}

CHDDirectory::~CHDDirectory(void)
{
}

bool CHDDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  std::string pathWithSlash(url.Get());
  if (!pathWithSlash.empty() && pathWithSlash[pathWithSlash.size() - 1] != '\\')
    pathWithSlash.push_back('\\');

  std::string searchMask(CUtil::GetFatXQualifiedPath(pathWithSlash));
  if (searchMask.empty())
    return false;

  //! @todo support m_strFileMask, require rewrite of internal caching
  searchMask += '*';

  WIN32_FIND_DATA findData = {};
  HANDLE hSearch = FindFirstFileA(searchMask.c_str(), &findData);

  if (hSearch == INVALID_HANDLE_VALUE)
    return GetLastError() == ERROR_FILE_NOT_FOUND
               ? Exists(url)
               : false; // return true if directory exist and empty

  do
  {
    std::string itemName(findData.cFileName);
    if (itemName == "." || itemName == "..")
      continue;

    CFileItemPtr pItem(new CFileItem(itemName));

    pItem->m_bIsFolder = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
    if (pItem->m_bIsFolder)
      pItem->SetPath(pathWithSlash + itemName + '\\');
    else
      pItem->SetPath(pathWithSlash + itemName);

    if ((findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0 ||
        itemName[0] == '.') // mark files starting from dot as hidden
      pItem->SetProperty("file:hidden", true);

    // calculation of size and date costs a little on win32
    // so DIR_FLAG_NO_FILE_INFO flag is ignored
    FILETIME fileTime;
    fileTime.dwLowDateTime = findData.ftLastWriteTime.dwLowDateTime;
    fileTime.dwHighDateTime = findData.ftLastWriteTime.dwHighDateTime;
    FILETIME localTime;
    if (FileTimeToLocalFileTime(&fileTime, &localTime) == TRUE)
      pItem->m_dateTime = localTime;
    else
      pItem->m_dateTime = 0;

    if (!pItem->m_bIsFolder)
      pItem->m_dwSize = (__int64(findData.nFileSizeHigh) << 32) + findData.nFileSizeLow;

    items.Add(pItem);
  } while (FindNextFileA(hSearch, &findData));

  FindClose(hSearch);

  return true;
}

bool CHDDirectory::Create(const CURL& url)
{
  std::string name(prepareXboxDirectoryName(url.Get()));
  if (name.empty())
    return false;

  if (!Create(name))
    return Exists(url);

  return true;
}

bool CHDDirectory::Remove(const CURL& url)
{
  std::string name(prepareXboxDirectoryName(url.Get()));
  if (name.empty())
    return false;

  if (RemoveDirectoryA(name.c_str()))
    return true;

  return !Exists(url);
}

bool CHDDirectory::Exists(const CURL& url)
{
  std::string name(prepareXboxDirectoryName(url.Get()));
  if (name.empty())
    return false;

  DWORD fileAttrs = GetFileAttributesA(name.c_str());
  if (fileAttrs == INVALID_FILE_ATTRIBUTES || (fileAttrs & FILE_ATTRIBUTE_DIRECTORY) == 0)
    return false;

  return true;
}

bool CHDDirectory::RemoveRecursive(const CURL& url)
{
  std::string pathWithSlash(url.Get());
  if (!pathWithSlash.empty() && pathWithSlash[pathWithSlash.size() - 1] != '\\')
    pathWithSlash.push_back('\\');

  std::string basePath = CUtil::GetFatXQualifiedPath(pathWithSlash);
  if (basePath.empty())
    return false;

  std::string searchMask = basePath + '*';

  WIN32_FIND_DATA findData = {};
  HANDLE hSearch = FindFirstFileA(searchMask.c_str(), &findData);

  if (hSearch == INVALID_HANDLE_VALUE)
    return GetLastError() == ERROR_FILE_NOT_FOUND
               ? Exists(url)
               : false; // return true if directory exist and empty

  bool success = true;
  do
  {
    std::string itemName(findData.cFileName);
    if (itemName == "." || itemName == "..")
      continue;

    std::string path = basePath + itemName;
    if (0 != (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      if (!RemoveRecursive(CURL(path)))
      {
        success = false;
        break;
      }
    }
    else
    {
      if (FALSE == DeleteFileA(path.c_str()))
      {
        success = false;
        break;
      }
    }
  } while (FindNextFileA(hSearch, &findData));

  FindClose(hSearch);

  if (success)
  {
    if (FALSE == RemoveDirectoryA(basePath.c_str()))
      success = false;
  }

  return success;
}

bool CHDDirectory::Create(std::string path) const
{
  if (!CreateDirectoryA(path.c_str(), nullptr))
  {
    if (GetLastError() == ERROR_ALREADY_EXISTS)
      return true;

    if (GetLastError() != ERROR_PATH_NOT_FOUND)
      return false;

    size_t sep = path.rfind('\\');
    if (sep == std::string::npos)
      return false;

    if (Create(path.substr(0, sep)))
      return Create(path);

    return false;
  }

  return true;
}
