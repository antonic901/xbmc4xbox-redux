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

#include <stdlib.h>
#include "xbox/Network.h"
#include "system.h"
#include "DirectoryFactory.h"
#include "HDDirectory.h"
#include "SpecialProtocolDirectory.h"
#include "MultiPathDirectory.h"
#include "StackDirectory.h"
#include "FileDirectoryFactory.h"
#include "PlaylistDirectory.h"
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
#include "Application.h"
#include "utils/StringUtils.h"
#include "addons/Addon.h"
#include "utils/log.h"

#ifdef HAS_FILESYSTEM_SMB
#ifdef _WIN32PC
#include "WINSMBDirectory.h"
#else
#include "SMBDirectory.h"
#endif
#endif
#ifdef HAS_FILESYSTEM_CDDA
#include "CDDADirectory.h"
#endif
#include "PluginDirectory.h"
#ifdef HAS_FILESYSTEM
#include "ISO9660Directory.h"
#include "SMBDirectory.h"
#include "CDDADirectory.h"
#include "RTVDirectory.h"
#include "SndtrkDirectory.h"
#include "DAAPDirectory.h"
#include "MemUnitDirectory.h"
#include "HTSPDirectory.h"
#endif
#ifdef HAS_UPNP
#include "UPnPDirectory.h"
#endif
#include "xbox/Network.h"
#include "ZipDirectory.h"
#include "RarDirectory.h"
#include "TuxBoxDirectory.h"
#include "HDHomeRunDirectory.h"
#include "SlingboxDirectory.h"
#include "MythDirectory.h"
#include "FileItem.h"
#include "URL.h"
#include "RSSDirectory.h"
#include "ResourceDirectory.h"

using namespace XFILE;

/*!
 \brief Create a IDirectory object of the share type specified in \e strPath .
 \param strPath Specifies the share type to access, can be a share or share with path.
 \return IDirectory object to access the directories on the share.
 \sa IDirectory
 */
IDirectory* CFactoryDirectory::Create(const CURL& url)
{
  CFileItem item(url.Get(), false);
  IFileDirectory* pDir=CFactoryFileDirectory::Create(url, &item);
  if (pDir)
    return pDir;

  if (url.GetProtocol().empty() || url.IsProtocol("file")) return new CHDDirectory();
  if (url.IsProtocol("special")) return new CSpecialProtocolDirectory();
  if (url.IsProtocol("sources")) return new CSourcesDirectory();
  if (url.IsProtocol("addons")) return new CAddonsDirectory();
#ifdef HAS_FILESYSTEM_CDDA
  if (url.IsProtocol("cdda")) return new CCDDADirectory();
#endif
#ifdef HAS_FILESYSTEM
  if (url.IsProtocol("iso9660")) return new CISO9660Directory();
  if (url.IsProtocol("soundtrack")) return new CSndtrkDirectory();
#endif
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
  if (url.IsProtocol("library")) return new CLibraryDirectory();
  if (url.IsProtocol("favourites")) return new CFavouritesDirectory();
  if (url.IsProtocol("filereader")) 
  {
    CURL url2(url.GetFileName());
    return CFactoryDirectory::Create(url2);
  }
#ifdef HAS_XBOX_HARDWARE
  // Is this same as url.IsProtocol("mem")?
  if (StringUtils::StartsWith(url.GetProtocol(), "mem")) return new CMemUnitDirectory();
#endif

  if( g_application.getNetwork().IsAvailable(true) )
  {
    if (url.IsProtocol("tuxbox")) return new CDirectoryTuxBox();
    if (url.IsProtocol("ftp") ||  url.IsProtocol("ftpx") ||  url.IsProtocol("ftps")) return new CFTPDirectory();
    if (url.IsProtocol("http") || url.IsProtocol("https")) return new CHTTPDirectory();
    if (url.IsProtocol("dav") || url.IsProtocol("davs")) return new CDAVDirectory();
#ifdef HAS_FILESYSTEM
    if (url.IsProtocol("smb")) return new CSMBDirectory();
    if (url.IsProtocol("daap")) return new CDAAPDirectory();
    if (url.IsProtocol("rtv")) return new CRTVDirectory();
    if (url.IsProtocol("htsp")) return new CHTSPDirectory();
#endif
#ifdef HAS_UPNP
    if (url.IsProtocol("upnp")) return new CUPnPDirectory();
#endif
    if (url.IsProtocol("hdhomerun")) return new CHomeRunDirectory();
    if (url.IsProtocol("sling")) return new CSlingboxDirectory();
    if (url.IsProtocol("myth")) return new CMythDirectory();
    if (url.IsProtocol("cmyth")) return new CMythDirectory();
    if (url.IsProtocol("rss")) return new CRSSDirectory();
    if (url.IsProtocol("resource")) return new CResourceDirectory();
  }

  CLog::Log(LOGWARNING, "%s - Unsupported protocol(%s) in %s", __FUNCTION__, url.GetProtocol().c_str(), url.Get().c_str() );
  return NULL;
}

