/*
 *  Copyright (C) 2010-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileUtils.h"

#include "FileOperationJob.h"
#include "ServiceBroker.h"
#include "URIUtils.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/LocalizeStrings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
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

bool CFileUtils::RemoteAccessAllowed(const std::string &strPath)
{
  std::string SourceNames[] = { "programs", "files", "video", "music", "pictures" };

  std::string realPath = URIUtils::GetRealPath(strPath);
  // for rar:// and zip:// paths we need to extract the path to the archive
  // instead of using the VFS path
  while (URIUtils::IsInArchive(realPath))
    realPath = CURL(realPath).GetHostName();

  if (StringUtils::StartsWithNoCase(realPath, "virtualpath://upnproot/"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "musicdb://"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "videodb://"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "library://video"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "library://music"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "sources://video"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://musicplaylists"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://profile/playlists"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://videoplaylists"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://skin"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://profile/addon_data"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "addons://sources"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "upnp://"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "plugin://"))
    return true;
  else
  {
    std::string strPlaylistsPath = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH);
    URIUtils::RemoveSlashAtEnd(strPlaylistsPath);
    if (StringUtils::StartsWithNoCase(realPath, strPlaylistsPath))
      return true;
  }
  bool isSource;
  // Check manually added sources (held in sources.xml)
  for (size_t i = 0; i < sizeof(SourceNames) / sizeof(std::string); ++i)
  {
    VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources(SourceNames[i]);
    int sourceIndex = CUtil::GetMatchingSource(realPath, *sources, isSource);
    if (sourceIndex >= 0 && sourceIndex < static_cast<int>(sources->size()) &&
        sources->at(sourceIndex).m_iHasLock != LOCK_STATE_LOCKED &&
        sources->at(sourceIndex).m_allowSharing)
      return true;
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

bool CFileUtils::CheckFileAccessAllowed(const std::string &filePath)
{
  // DENY access to paths matching
  std::vector<std::string> blacklist;
  blacklist.push_back("passwords.xml");
  blacklist.push_back("sources.xml");
  blacklist.push_back("guisettings.xml");
  blacklist.push_back("advancedsettings.xml");
  blacklist.push_back("server.key");
  blacklist.push_back("/.ssh/");
  // ALLOW kodi paths
  std::vector<std::string> whitelist;
  whitelist.push_back(CSpecialProtocol::TranslatePath("special://home"));
  whitelist.push_back(CSpecialProtocol::TranslatePath("special://xbmc"));
  whitelist.push_back(CSpecialProtocol::TranslatePath("special://musicartistsinfo"));

  std::vector<std::string> kodiExtraWhitelist = StringUtils::Split("", ',');
  whitelist.insert(whitelist.end(), kodiExtraWhitelist.begin(), kodiExtraWhitelist.end());

  // image urls come in the form of image://... sometimes with a / appended at the end
  // and can be embedded in a music or video file image://music@...
  // strip this off to get the real file path
  bool isImage = false;
  std::string decodePath = CURL::Decode(filePath);
  size_t pos = decodePath.find("image://");
  if (pos != std::string::npos)
  {
    isImage = true;
    decodePath.erase(pos, 8);
    URIUtils::RemoveSlashAtEnd(decodePath);
    if (StringUtils::StartsWith(decodePath, "music@") || StringUtils::StartsWith(decodePath, "video@"))
      decodePath.erase(pos, 6);
  }

  // check blacklist
  for (std::vector<std::string>::const_iterator b = blacklist.begin(); b != blacklist.end(); ++b)
  {
    if (decodePath.find(*b) != std::string::npos)
    {
      CLog::Log(LOGERROR, "%s denied access to %s", __FUNCTION__, decodePath.c_str());
      return false;
    }
  }

  // local file
  CURL url(decodePath);
  if (url.GetProtocol().empty())
    return true;

  // if it isn't a local file, it must be a vfs entry
  if (! isImage)
    return CFileUtils::RemoteAccessAllowed(decodePath);
  return true;
}

bool CFileUtils::Exists(const std::string& strFileName, bool bUseCache)
{
  return CFile::Exists(strFileName, bUseCache);
}
