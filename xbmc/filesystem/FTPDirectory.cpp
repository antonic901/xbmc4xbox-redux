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

#include "FTPDirectory.h"
#include "FTPParse.h"
#include "URL.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "CurlFile.h"
#include "FileItem.h"
#include "utils/CharsetConverter.h"

using namespace XFILE;

CFTPDirectory::CFTPDirectory(void){}
CFTPDirectory::~CFTPDirectory(void){}

bool CFTPDirectory::GetDirectory(const CURL& url2, CFileItemList &items)
{
  CCurlFile reader;

  CURL url(url2);

  CStdString path = url.GetFileName();
  if( !path.IsEmpty() && !StringUtils2::EndsWith(path, "/") )
  {
    path += "/";
    url.SetFileName(path);
  }

  if (!reader.Open(url))
    return false;


  char buffer[MAX_PATH + 1024];
  while( reader.ReadString(buffer, sizeof(buffer)) )
  {
    CStdString strBuffer = buffer;

    StringUtils::RemoveCRLF(strBuffer);

    struct ftpparse lp = {};
    if (ftpparse(&lp, (char*)strBuffer.c_str(), strBuffer.size()) == 1)
    {
      if( lp.namelen == 0 )
        continue;

      if( lp.flagtrycwd == 0 && lp.flagtryretr == 0 )
        continue;

      /* buffer name as it's not allways null terminated */
      CStdString name;
      name.assign(lp.name, lp.namelen);

      if( name.Equals("..") || name.Equals(".") )
        continue;

      /* this should be conditional if we ever add    */
      /* support for the utf8 extension in ftp client */
      g_charsetConverter.unknownToUTF8(name);

      CFileItemPtr pItem(new CFileItem(name));

      CStdString filePath = path + name;
      pItem->m_bIsFolder = (bool)(lp.flagtrycwd != 0);
      if (pItem->m_bIsFolder)
        URIUtils::AddSlashAtEnd(filePath);

      /* qualify the url with host and all */
      url.SetFileName(filePath);
      pItem->SetPath(url.Get());

      pItem->m_dwSize = lp.size;
      pItem->m_dateTime=lp.mtime;

      items.Add(pItem);
    }
  }

  return true;
}

bool CFTPDirectory::Exists(const CURL& url)
{
  // make sure ftp dir ends with slash,
  // curl need to known it's a dir to check ftp directory existence.
  CStdString file = url.Get();
  URIUtils::AddSlashAtEnd(file);

  CCurlFile ftp;
  CURL url2(file);
  return ftp.Exists(url2);
}
