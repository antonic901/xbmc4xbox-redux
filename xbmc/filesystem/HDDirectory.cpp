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
#include "utils/log.h"
#include "AutoPtrHandle.h"
#include "HDDirectory.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "xbox/IoSupport.h"
#include "iso9660.h"
#include "URL.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "utils/CharsetConverter.h"
#include "threads/CriticalSection.h"

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD) -1)
#endif

using namespace AUTOPTR;
using namespace XFILE;

CHDDirectory::CHDDirectory(void)
{}

CHDDirectory::~CHDDirectory(void)
{}

bool CHDDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  WIN32_FIND_DATA wfd;

  CStdString strPath=url.Get();
  g_charsetConverter.utf8ToStringCharset(strPath);

  CStdString strRoot = strPath;

  memset(&wfd, 0, sizeof(wfd));
  URIUtils::AddSlashAtEnd(strRoot);
  strRoot.Replace("/", "\\");
  if (URIUtils::IsDVD(strRoot) && m_isoReader.IsScanned())
  {
    // Reset iso reader and remount or
    // we can't access the dvd-rom
    m_isoReader.Reset();

    CIoSupport::Dismount("Cdrom0");
    CIoSupport::RemapDriveLetter('D', "Cdrom0");
  }

  CStdString strSearchMask = strRoot;
  strSearchMask += "*.*";

  FILETIME localTime;
  CAutoPtrFind hFind ( FindFirstFile(strSearchMask.c_str(), &wfd));

  // on error, check if path exists at all, this will return true if empty folder
  if (!hFind.isValid())
    return Exists(url);

  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
          CStdString strDir = wfd.cFileName;
          if (strDir != "." && strDir != "..")
          {
            CStdString strLabel=wfd.cFileName;
            g_charsetConverter.unknownToUTF8(strLabel);
            CFileItemPtr pItem(new CFileItem(strLabel));
            CStdString itemPath = strRoot + wfd.cFileName;
            g_charsetConverter.unknownToUTF8(itemPath);
            pItem->m_bIsFolder = true;
            URIUtils::AddSlashAtEnd(itemPath);
            pItem->SetPath(itemPath);
            FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
            pItem->m_dateTime=localTime;

            items.Add(pItem);
          }
        }
        else
        {
          CStdString strLabel=wfd.cFileName;
          g_charsetConverter.unknownToUTF8(strLabel);
          CFileItemPtr pItem(new CFileItem(strLabel));
          CStdString itemPath = strRoot + wfd.cFileName;
          g_charsetConverter.unknownToUTF8(itemPath);
          pItem->SetPath(itemPath);
          pItem->m_bIsFolder = false;
          pItem->m_dwSize = CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          pItem->m_dateTime=localTime;

          items.Add(pItem);
        }
      }
    }
    while (FindNextFile((HANDLE)hFind, &wfd));
#ifdef _XBOX
    // if we use AutoPtrHandle, this auto-closes
    FindClose((HANDLE)hFind); //should be closed
#endif
  }
  return true;
}

bool CHDDirectory::Create(const CURL& url)
{
  std::string strPath = url.Get();
  g_charsetConverter.utf8ToStringCharset(strPath);
  URIUtils::AddSlashAtEnd(strPath);

  // okey this is really evil, since the create will succeed
  // the caller will have no idea that a different directory was created
  if (CSettings::GetInstance().GetBool("services.ftpautofatx"))
  {
    CStdString strPath1(strPath);
    CUtil::GetFatXQualifiedPath(strPath1);
    if(strPath1 != strPath)
    {
      strPath = strPath1;
      CLog::Log(LOGNOTICE,"%s - FATX: %s -> %s", strPath.c_str(), strPath1.c_str());
    }
  }

  return Create(strPath);
}

bool CHDDirectory::Remove(const CURL& url)
{
  CStdString strPath1 = url.Get();
  g_charsetConverter.utf8ToStringCharset(strPath1);
  return (::RemoveDirectory(strPath1) || GetLastError() == ERROR_PATH_NOT_FOUND) ? true : false;
}

bool CHDDirectory::Exists(const CURL& url)
{
  CStdString strReplaced=url.Get();
  g_charsetConverter.utf8ToStringCharset(strReplaced);
  strReplaced.Replace("/","\\");
  CUtil::GetFatXQualifiedPath(strReplaced);
  URIUtils::AddSlashAtEnd(strReplaced);
  DWORD attributes = GetFileAttributes(strReplaced.c_str());
  if(attributes == INVALID_FILE_ATTRIBUTES)
    return false;
  if (FILE_ATTRIBUTE_DIRECTORY & attributes) return true;
  return false;
}

bool CHDDirectory::RemoveRecursive(const CURL& url)
{
  std::string pathWithSlash(url.Get());
  if (!pathWithSlash.empty() && pathWithSlash.at(pathWithSlash.size() - 1) != '\\')
    pathWithSlash.push_back('\\');

  // Do we need this here? If we get to this point we are already in Q:\\path\to\file format
  std::string basePath = pathWithSlash/*CWIN32Util::ConvertPathToWin32Form(pathWithSlash)*/;
  if (basePath.empty())
    return false;

  std::string searchMask = basePath + '*';

  HANDLE hSearch;
  WIN32_FIND_DATA findData = {};

  hSearch = FindFirstFile(searchMask.c_str(), &findData);

  if (hSearch == INVALID_HANDLE_VALUE)
    return GetLastError() == ERROR_FILE_NOT_FOUND ? Exists(url) : false; // return true if directory exist and empty

  do
  {
    std::string itemName(findData.cFileName);
    if (itemName == "." || itemName == "..")
      continue;

    std::string path = basePath + itemName;
    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      CURL url(path);
      if (!RemoveRecursive(url))
        return false;

      if (FALSE == RemoveDirectory(path.c_str()))
        return false;
    }
    else
    {
      if (FALSE == DeleteFile(path.c_str()))
        return false;
    }
  } while (FindNextFile(hSearch, &findData));

  FindClose(hSearch);

  return true;
}

bool CHDDirectory::Create(std::string path) const
{
  if (!CreateDirectoryA(path.c_str(), NULL))
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
