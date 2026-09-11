/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileDirectoryFactory.h"

#include "ISO9660Directory.h"
#include "RSSDirectory.h"
#include "utils/URIUtils.h"
#include "Directory.h"
#include "FileItem.h"
#include "PlaylistFileDirectory.h"
#include "RarDirectory.h"
#include "ServiceBroker.h"
#include "SmartPlaylistDirectory.h"
#include "URL.h"
#include "ZipDirectory.h"
#include "addons/addoninfo/AddonInfo.h"
#include "filesystem/File.h"
#include "playlists/PlayListFactory.h"
#include "playlists/SmartPlayList.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

// These were replaced by Audio Encoder addons
#include "OGGFileDirectory.h"
#include "NSFFileDirectory.h"
#include "SIDFileDirectory.h"
#include "ASAPFileDirectory.h"
#include "cores/paplayer/ASAPCodec.h"

using namespace ADDON;
using namespace XFILE;
using namespace PLAYLIST;

CFileDirectoryFactory::CFileDirectoryFactory(void) {}

CFileDirectoryFactory::~CFileDirectoryFactory(void) {}

// return NULL + set pItem->m_bIsFolder to remove it completely from list.
IFileDirectory* CFileDirectoryFactory::Create(const CURL& url, CFileItem* pItem, const std::string& strMask)
{
  if (url.IsProtocol("stack")) // disqualify stack as we need to work with each of the parts instead
    return NULL;

  /**
   * Check available binary addons which can contain files with underlaid
   * folders / files.
   * Currently in vfs and audiodecoder addons.
   *
   * @note The file extensions are absolutely necessary for these in order to
   * identify the associated add-on.
   */
  /**@{*/

  // Get file extensions to find addon related to it.
  std::string strExtension = URIUtils::GetExtension(url);
  StringUtils::ToLower(strExtension);

  if ((url.IsFileType("ogg") || url.IsFileType("oga")) && CFile::Exists(url))
  {
    IFileDirectory* pDir=new COGGFileDirectory;
    //  Has the ogg file more than one bitstream?
    if (pDir->ContainsFiles(url))
    {
      return pDir; // treat as directory
    }

    delete pDir;
    return NULL;
  }
  if (url.IsFileType("nsf") && CFile::Exists(url))
  {
    IFileDirectory* pDir=new CNSFFileDirectory;
    //  Has the nsf file more than one track?
    if (pDir->ContainsFiles(url))
      return pDir; // treat as directory

    delete pDir;
    return NULL;
  }
  if (url.IsFileType("sid") && CFile::Exists(url))
  {
    IFileDirectory* pDir=new CSIDFileDirectory;
    //  Has the sid file more than one track?
    if (pDir->ContainsFiles(url))
      return pDir; // treat as directory

    delete pDir;
    return NULL;
  }
  if (ASAPCodec::IsSupportedFormat(url.GetFileType()) && CFile::Exists(url))
  {
    IFileDirectory* pDir=new CASAPFileDirectory;
    //  Has the asap file more than one track?
    if (pDir->ContainsFiles(url))
      return pDir; // treat as directory

    delete pDir;
    return NULL;
  }

  if (pItem->IsRSS())
    return new CRSSDirectory();


  if (pItem->IsDiscImage())
  {
    CISO9660Directory* iso = new CISO9660Directory();
    if (iso->Exists(pItem->GetURL()))
      return iso;

    delete iso;

    return NULL;
  }

  if (url.IsFileType("zip"))
  {
    CURL zipURL = URIUtils::CreateArchivePath("zip", url);

    CFileItemList items;
    CDirectory::GetDirectory(zipURL, items, strMask, DIR_FLAG_DEFAULTS);
    if (items.Size() == 0) // no files
      pItem->m_bIsFolder = true;
    else if (items.Size() == 1 && items[0]->m_idepth == 0 && !items[0]->m_bIsFolder)
    {
      // one STORED file - collapse it down
      *pItem = *items[0];
    }
    else
    { // compressed or more than one file -> create a zip dir
      pItem->SetURL(zipURL);
      return new CZipDirectory;
    }
    return NULL;
  }
  if (url.IsFileType("rar") || url.IsFileType("001"))
  {
    std::vector<std::string> tokens;
    const std::string strPath = url.Get();
    StringUtils::Tokenize(strPath,tokens,".");
    if (tokens.size() > 2)
    {
      if (url.IsFileType("001"))
      {
        if (StringUtils::EqualsNoCase(tokens[tokens.size()-2], "ts")) // .ts.001 - treat as a movie file to scratch some users itch
          return NULL;
      }
      std::string token = tokens[tokens.size()-2];
      if (StringUtils::StartsWith(token, "part")) // only list '.part01.rar'
      {
        // need this crap to avoid making mistakes - yeyh for the new rar naming scheme :/
        __stat64 stat;
        int digits = token.size()-4;
        std::string strFormat = StringUtils::Format("part%%0%ii", digits);
        std::string strNumber = StringUtils::Format(strFormat.c_str(), 1);
        std::string strPath2 = strPath;
        StringUtils::Replace(strPath2,token,strNumber);
        if (atoi(token.substr(4).c_str()) > 1 && CFile::Stat(strPath2,&stat) == 0)
        {
          pItem->m_bIsFolder = true;
          return NULL;
        }
      }
    }

    CURL rarURL = URIUtils::CreateArchivePath("rar", url);

    CFileItemList items;
    CDirectory::GetDirectory(rarURL, items, strMask, DIR_FLAG_DEFAULTS);
    if (items.Size() == 0) // no files - hide this
      pItem->m_bIsFolder = true;
    else if (items.Size() == 1 && items[0]->m_idepth == 0x30)
    {
      // one STORED file - collapse it down
      *pItem = *items[0];
    }
    else
    { // compressed or more than one file -> create a rar dir
      pItem->SetURL(rarURL);
      return new CRarDirectory;
    }
    return NULL;
  }
  if (url.IsFileType("xsp"))
  { // XBMC Smart playlist - just XML renamed to XSP
    // read the name of the playlist in
    CSmartPlaylist playlist;
    if (playlist.OpenAndReadName(url))
    {
      pItem->SetLabel(playlist.GetName());
      pItem->SetLabelPreformatted(true);
    }
    IFileDirectory* pDir=new CSmartPlaylistDirectory;
    return pDir; // treat as directory
  }
  if (CPlayListFactory::IsPlaylist(url))
  { // Playlist file
    // currently we only return the directory if it contains
    // more than one file.  Reason is that .pls and .m3u may be used
    // for links to http streams etc.
    IFileDirectory *pDir = new CPlaylistFileDirectory();
    CFileItemList items;
    if (pDir->GetDirectory(url, items))
    {
      if (items.Size() > 1)
        return pDir;
    }
    delete pDir;
    return NULL;
  }

  return NULL;
}

