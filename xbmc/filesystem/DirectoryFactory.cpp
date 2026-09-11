/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <stdlib.h>
#include "network/Network.h"
#include "DirectoryFactory.h"
#include "SpecialProtocolDirectory.h"
#include "MemUnitDirectory.h"
#include "MultiPathDirectory.h"
#include "StackDirectory.h"
#include "FileDirectoryFactory.h"
#include "GameSavesDirectory.h"
#include "PlaylistDirectory.h"
#include "ProgramDatabaseDirectory.h"
#include "MusicDatabaseDirectory.h"
#include "MusicSearchDirectory.h"
#include "VideoDatabaseDirectory.h"
#include "FavouritesDirectory.h"
#include "LibraryDirectory.h"
#include "AddonsDirectory.h"
#include "SourcesDirectory.h"
#include "FTPDirectory.h"
#include "HTTPDirectory.h"
#include "DAVDirectory.h"
#include "utils/log.h"

#include "HDDirectory.h"
#ifdef HAS_FILESYSTEM_SMB
#include "SMBDirectory.h"
#endif
#include "CDDADirectory.h"
#include "PluginDirectory.h"
#include "ISO9660Directory.h"
#ifdef HAS_UPNP
#include "UPnPDirectory.h"
#endif
#include "ZipDirectory.h"
#include "FileItem.h"
#include "URL.h"
#include "RarDirectory.h"
#include "RSSDirectory.h"
#include "ResourceDirectory.h"
#include "ServiceBroker.h"
#include "SndtrkDirectory.h"
#include "utils/StringUtils.h"

using namespace ADDON;

using namespace XFILE;

/*!
 \brief Create a IDirectory object of the share type specified in a given item path.
 \param item Specifies the item to which the factory will create the directory instance
 \return IDirectory object to access the directories on the share.
 \sa IDirectory
 */
IDirectory* CDirectoryFactory::Create(const CFileItem& item)
{
  CURL curl(item.GetDynPath());

  // Store the mimetype, allowing the PlayListFactory to set it on the created FileItem
  const std::string& mimeType = item.GetMimeType();
  if (!mimeType.empty())
    curl.SetOption("mimetype", mimeType);

  return Create(curl);
}

/*!
 \brief Create a IDirectory object of the share type specified in \e strPath .
 \param strPath Specifies the share type to access, can be a share or share with path.
 \return IDirectory object to access the directories on the share.
 \sa IDirectory
 */
IDirectory* CDirectoryFactory::Create(const CURL& url)
{
  CFileItem item(url.Get(), true);
  IFileDirectory* pDir = CFileDirectoryFactory::Create(url, &item);
  if (pDir)
    return pDir;

  if (url.GetProtocol().empty() || url.IsProtocol("file")) return new CHDDirectory();
  if (url.IsProtocol("special")) return new CSpecialProtocolDirectory();
  if (url.IsProtocol("sources")) return new CSourcesDirectory();
  if (url.IsProtocol("addons")) return new CAddonsDirectory();
#if defined(HAS_OPTICAL_DRIVE)
  if (url.IsProtocol("cdda")) return new CCDDADirectory();
#endif
  if (url.IsProtocol("iso9660")) return new CISO9660Directory();
  if (url.IsProtocol("soundtrack")) return new CSndtrkDirectory();
  if (url.IsProtocol("plugin")) return new CPluginDirectory();
  if (url.IsProtocol("zip")) return new CZipDirectory();
  if (url.IsProtocol("rar")) return new CRarDirectory();
  if (url.IsProtocol("multipath")) return new CMultiPathDirectory();
  if (url.IsProtocol("stack")) return new CStackDirectory();
  if (url.IsProtocol("playlistmusic")) return new CPlaylistDirectory();
  if (url.IsProtocol("playlistvideo")) return new CPlaylistDirectory();
  if (url.IsProtocol("musicdb")) return new CMusicDatabaseDirectory();
  if (url.IsProtocol("musicsearch")) return new CMusicSearchDirectory();
  if (url.IsProtocol("videodb")) return new CVideoDatabaseDirectory();
  if (url.IsProtocol("programdb")) return new CProgramDatabaseDirectory();
  if (url.IsProtocol("library")) return new CLibraryDirectory();
  if (url.IsProtocol("favourites")) return new CFavouritesDirectory();
  if (url.IsProtocol("resource")) return new CResourceDirectory();

  if (url.IsProtocol("ftp") || url.IsProtocol("ftps") || url.IsProtocol("ftpx")) return new CFTPDirectory();
  if (url.IsProtocol("http") || url.IsProtocol("https")) return new CHTTPDirectory();
  if (url.IsProtocol("dav") || url.IsProtocol("davs")) return new CDAVDirectory();
#ifdef HAS_FILESYSTEM_SMB
  if (url.IsProtocol("smb")) return new CSMBDirectory();
#endif
#ifdef HAS_UPNP
  if (url.IsProtocol("upnp")) return new CUPnPDirectory();
#endif
  if (url.IsProtocol("rss") || url.IsProtocol("rsss")) return new CRSSDirectory();

  if (url.IsProtocol("gamesaves")) return new CGameSavesDirectory();
  if (url.IsProtocol("mem")) return new CMemUnitDirectory();

  CLog::Log(LOGWARNING, "%s - unsupported protocol(%s) in %s", __FUNCTION__, url.GetProtocol().c_str(),
            url.GetRedacted().c_str());
  return NULL;
}

