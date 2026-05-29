/*
 *  Copyright (C) 2010-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileUtils.h"

#include "FileOperationJob.h"
#include "URIUtils.h"
#include "URL.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"

#include <vector>

using namespace XFILE;

bool CFileUtils::DeleteItem(const std::string &strPath)
{
  CFileItemPtr item(new CFileItem(strPath));
  item->SetPath(strPath);
  item->m_bIsFolder = URIUtils::HasSlashAtEnd(strPath);
  item->Select(true);
  return DeleteItem(item);
}

bool CFileUtils::DeleteItem(const boost::shared_ptr<CFileItem>& item)
{
  if (!item || item->IsParentFolder())
    return false;

  // Create a temporary item list containing the file/folder for deletion
  CFileItemPtr pItemTemp(new CFileItem(*item));
  pItemTemp->Select(true);
  CFileItemList items;
  items.Add(pItemTemp);

  // grab the real filemanager window, set up the progress bar,
  // and process the delete action
  CFileOperationJob op(CFileOperationJob::ActionDelete, items, "");

  return op.DoWork();
}

bool CFileUtils::RenameFile(const std::string &strFile)
{
  std::string strFileAndPath(strFile);
  URIUtils::RemoveSlashAtEnd(strFileAndPath);
  std::string strFileName = URIUtils::GetFileName(strFileAndPath);
  std::string strPath = URIUtils::GetDirectory(strFileAndPath);
  if (CGUIKeyboardFactory::ShowAndGetInput(strFileName, g_localizeStrings.Get(16013), false))
  {
    strPath = URIUtils::AddFileToFolder(strPath, strFileName);
    CLog::Log(LOGINFO, "FileUtils: rename %s->%s", strFileAndPath.c_str(), strPath.c_str());
    if (URIUtils::IsMultiPath(strFileAndPath))
    { // special case for multipath renames - rename all the paths.
      std::vector<std::string> paths;
      CMultiPathDirectory::GetPaths(strFileAndPath, paths);
      bool success = false;
      for (unsigned int i = 0; i < paths.size(); ++i)
      {
        std::string filePath(paths[i]);
        URIUtils::RemoveSlashAtEnd(filePath);
        filePath = URIUtils::GetDirectory(filePath);
        filePath = URIUtils::AddFileToFolder(filePath, strFileName);
        if (CFile::Rename(paths[i], filePath))
          success = true;
      }
      return success;
    }
    return CFile::Rename(strFileAndPath, strPath);
  }
  return false;
}

CDateTime CFileUtils::GetModificationDate(const std::string& strFileNameAndPath,
                                          const bool& bUseLatestDate)
{
  if (bUseLatestDate)
    return GetModificationDate(1, strFileNameAndPath);
  else
    return GetModificationDate(0, strFileNameAndPath);
}

CDateTime CFileUtils::GetModificationDate(const int& code, const std::string& strFileNameAndPath)
{
  CDateTime dateAdded;
  if (strFileNameAndPath.empty())
  {
    CLog::Log(LOGDEBUG, "%s empty strFileNameAndPath variable", __FUNCTION__);
    return dateAdded;
  }

  try
  {
    std::string file = strFileNameAndPath;
    if (URIUtils::IsStack(strFileNameAndPath))
      file = CStackDirectory::GetFirstStackedFile(strFileNameAndPath);

    if (URIUtils::IsInArchive(file))
      file = CURL(file).GetHostName();

    // Try to get ctime (creation on Windows, metadata change on Linux) and mtime (modification)
    struct __stat64 buffer;
    if (CFile::Stat(file, &buffer) == 0 && (buffer.st_mtime != 0 || buffer.st_ctime != 0))
    {
      time_t now = time(NULL);
      time_t addedTime;
      // Prefer the modification time if it's valid, fallback to ctime
      if (code == 0)
      {
        if (buffer.st_mtime != 0 && static_cast<time_t>(buffer.st_mtime) <= now)
          addedTime = static_cast<time_t>(buffer.st_mtime);
        else
          addedTime = static_cast<time_t>(buffer.st_ctime);
      }
      // Use the later of the ctime and mtime
      else if (code == 1)
      {
        addedTime =
            std::max(static_cast<time_t>(buffer.st_ctime), static_cast<time_t>(buffer.st_mtime));
        // if the newer of the two dates is in the future, we try it with the older one
        if (addedTime > now)
          addedTime =
              std::min(static_cast<time_t>(buffer.st_ctime), static_cast<time_t>(buffer.st_mtime));
      }
      // Prefer the earliest of ctime and mtime, fallback to other
      else
      {
        addedTime =
            std::min(static_cast<time_t>(buffer.st_ctime), static_cast<time_t>(buffer.st_mtime));
        // if the older of the two dates is invalid, we try it with the newer one
        if (addedTime == 0)
          addedTime =
              std::max(static_cast<time_t>(buffer.st_ctime), static_cast<time_t>(buffer.st_mtime));
      }


      // make sure the datetime does is not in the future
      if (addedTime <= now)
      {
        struct tm* time;
#ifdef HAVE_LOCALTIME_R
        struct tm result = {};
        time = localtime_r(&addedTime, &result);
#else
        time = localtime(&addedTime);
#endif
        if (time)
          dateAdded = *time;
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to extract modification date for file (%s)", __FUNCTION__,
              strFileNameAndPath.c_str());
  }
  return dateAdded;
}

bool CFileUtils::Exists(const std::string& strFileName, bool bUseCache)
{
  return CFile::Exists(strFileName, bUseCache);
}
