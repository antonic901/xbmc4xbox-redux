/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "network/Network.h"
#include "FileFactory.h"
#include "HDFile.h"
#include "CurlFile.h"
#include "DAVFile.h"
#include "ShoutcastFile.h"
#ifdef HAS_FILESYSTEM_SMB
#include "SMBFile.h"
#endif
#include "SndtrkFile.h"
#include "FileReaderFile.h"
#include "CDDAFile.h"
#include "ISO9660File.h"
#include "ZipFile.h"
#ifdef HAS_UPNP
#include "UPnPFile.h"
#endif
#include "MemUnitFile.h"
#include "MusicDatabaseFile.h"
#include "VideoDatabaseFile.h"
#include "PluginFile.h"
#include "SpecialProtocolFile.h"
#include "MultiPathFile.h"
#include "ImageFile.h"
#include "RarFile.h"
#include "ResourceFile.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "ServiceBroker.h"

using namespace ADDON;
using namespace XFILE;

CFileFactory::CFileFactory() {}

CFileFactory::~CFileFactory() {}

IFile* CFileFactory::CreateLoader(const std::string& strFileName)
{
  CURL url(strFileName);
  return CreateLoader(url);
}

IFile* CFileFactory::CreateLoader(const CURL& url)
{
  if (url.IsProtocol("zip")) return new CZipFile();
  else if (url.IsProtocol("rar")) return new CRarFile();
  else if (url.IsProtocol("musicdb")) return new CMusicDatabaseFile();
  else if (url.IsProtocol("videodb")) return new CVideoDatabaseFile();
  else if (url.IsProtocol("plugin")) return new CPluginFile();
  else if (url.IsProtocol("library")) return NULL;
  else if (url.IsProtocol("special")) return new CSpecialProtocolFile();
  else if (url.IsProtocol("multipath")) return new CMultiPathFile();
  else if (url.IsProtocol("image")) return new CImageFile();
  else if (url.IsProtocol("file") || url.GetProtocol().empty())
  {
    return new CHDFile();
  }
  else if (url.IsProtocol("filereader")) return new CFileReaderFile();
#if defined(HAS_OPTICAL_DRIVE)
  else if (url.IsProtocol("cdda")) return new CFileCDDA();
#endif
  else if (url.IsProtocol("iso9660"))
    return new CISO9660File();
  else if (url.IsProtocol("resource")) return new CResourceFile();

  if (url.IsProtocol("ftp")
  ||  url.IsProtocol("ftps")
  ||  url.IsProtocol("ftpx")
  ||  url.IsProtocol("rss")
  ||  url.IsProtocol("rsss")
  ||  url.IsProtocol("http")
  ||  url.IsProtocol("https")) return new CCurlFile();
  else if (url.IsProtocol("dav") || url.IsProtocol("davs")) return new CDAVFile();
  else if (url.IsProtocol("shout") || url.IsProtocol("shouts")) return new CShoutcastFile();
#ifdef HAS_FILESYSTEM_SMB
  else if (url.IsProtocol("smb")) return new CSmbFile();
#endif
#ifdef HAS_UPNP
  else if (url.IsProtocol("upnp")) return new CUPnPFile();
#endif
  else if (url.IsProtocol("soundtrack")) return new CSndtrkFile();
  else if (url.IsProtocol("mem")) return new CMemUnitFile();

  CLog::Log(LOGWARNING, "%s - unsupported protocol(%s) in %s", __FUNCTION__, url.GetProtocol().c_str(),
            url.GetRedacted().c_str());
  return NULL;
}
