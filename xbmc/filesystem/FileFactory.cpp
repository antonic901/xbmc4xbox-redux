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

#include "xbox/Network.h"
#include "system.h"
#include "FileFactory.h"
#include "HDFile.h"
#include "CurlFile.h"
#include "DAVFile.h"
#include "ShoutcastFile.h"
#include "FileReaderFile.h"
#ifdef HAS_FILESYSTEM
#include "ISOFile.h"
#include "SMBFile.h"
#include "SndtrkFile.h"
#include "CDDAFile.h"
#include "MemUnitFile.h"
#endif
#include "ZipFile.h"
#include "RarFile.h"
#include "UPnPFile.h"
#include "MusicDatabaseFile.h"
#include "SpecialProtocolFile.h"
#include "MultiPathFile.h"
#include "ResourceFile.h"
#include "Application.h"
#include "ImageFile.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace XFILE;

CFileFactory::CFileFactory()
{
}

CFileFactory::~CFileFactory()
{
}

IFile* CFileFactory::CreateLoader(const CStdString& strFileName)
{
  CURL url(strFileName);
  return CreateLoader(url);
}

IFile* CFileFactory::CreateLoader(const CURL& url)
{
  if (url.IsProtocol("zip")) return new CZipFile();
  else if (url.IsProtocol("rar")) return new CRarFile();
  else if (url.IsProtocol("musicdb")) return new CMusicDatabaseFile();
  else if (url.IsProtocol("videodb")) return NULL;
  else if (url.IsProtocol("library")) return NULL;
  else if (url.IsProtocol("special")) return new CSpecialProtocolFile();
  else if (url.IsProtocol("multipath")) return new CMultiPathFile();
  else if (url.IsProtocol("image")) return new CImageFile();
  else if (url.IsProtocol("file") || url.GetProtocol().empty()) return new CFileHD();
  else if (url.IsProtocol("filereader")) return new CFileFileReader();
#ifdef HAS_FILESYSTEM
  else if (url.IsProtocol("iso9660")) return new CISOFile();
  else if (url.IsProtocol("soundtrack")) return new CSndtrkFile();
  else if (url.IsProtocol("cdda")) return new CFileCDDA();
  else if (url.IsProtocol("mem")) return new CMemUnitFile();
#endif
  if (url.IsProtocol("resource")) return new CResourceFile();

  if( g_application.getNetwork().IsAvailable() )
  {
    if (url.IsProtocol("ftp")
    ||  url.IsProtocol("ftpx")
    ||  url.IsProtocol("ftps")
    ||  url.IsProtocol("rss")
    ||  url.IsProtocol("http")
    ||  url.IsProtocol("https")) return new CCurlFile();
    else if (url.IsProtocol("dav") || url.IsProtocol("davs")) return new CDAVFile();
    else if (url.IsProtocol("shout")) return new CShoutcastFile();
#ifdef HAS_FILESYSTEM
    else if (url.IsProtocol("smb")) return new CSmbFile();
    else if (url.IsProtocol("upnp")) return new CUPnPFile();
#endif
  }

  CLog::Log(LOGWARNING, "%s - Unsupported protocol(%s) in %s", __FUNCTION__, url.GetProtocol().c_str(), url.Get().c_str() );
  return NULL;
}
