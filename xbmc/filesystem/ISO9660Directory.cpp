/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ISO9660Directory.h"

#include "FileItem.h"
#include "URL.h"
#include "Util.h"
#include "utils/URIUtils.h"

#include "iso9660.h"

using namespace XFILE;

bool CISO9660Directory::GetDirectory(const CURL& url, CFileItemList& items)
{
  CURL url2(url);
  if (!url2.IsProtocol("iso9660"))
  {
    url2.Reset();
    url2.SetProtocol("iso9660");
    url2.SetHostName(url.Get());
  }

  std::string strRoot(url2.Get());
  std::string strSub(url2.GetFileName());

  URIUtils::AddSlashAtEnd(strRoot);
  URIUtils::AddSlashAtEnd(strSub);

  // Scan active disc if not done before
  if (!m_isoReader.IsScanned())
    m_isoReader.Scan();

  WIN32_FIND_DATA wfd;
  HANDLE hFind;

  memset(&wfd, 0, sizeof(wfd));

  std::string strSearchMask;
  if (strSub != "")
  {
    strSearchMask = "\\" + strSub;
  }
  else
  {
    strSearchMask = "\\";
  }
  for (int i = 0; i < (int)strSearchMask.size(); ++i )
  {
    if (strSearchMask[i] == '/') strSearchMask[i] = '\\';
  }

  hFind = m_isoReader.FindFirstFile((char*)strSearchMask.c_str(), &wfd);
  if (hFind == NULL)
    return false;

  do
  {
    if (wfd.cFileName[0] != 0)
    {
      if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      {
        std::string strDir = wfd.cFileName;
        if (strDir != "." && strDir != "..")
        {
          CFileItemPtr pItem(new CFileItem(wfd.cFileName));
          std::string path = strRoot + wfd.cFileName;
          URIUtils::AddSlashAtEnd(path);
          pItem->SetPath(path);
          pItem->m_bIsFolder = true;
          FILETIME localTime;
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          pItem->m_dateTime=localTime;
          items.Add(pItem);
        }
      }
      else
      {
        CFileItemPtr pItem(new CFileItem(wfd.cFileName));
        pItem->SetPath(strRoot + wfd.cFileName);
        pItem->m_bIsFolder = false;
        pItem->m_dwSize = CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
        FILETIME localTime;
        FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
        pItem->m_dateTime=localTime;
        items.Add(pItem);
      }
    }
  }
  while (m_isoReader.FindNextFile(hFind, &wfd));
  m_isoReader.FindClose(hFind);

  return true;
}

bool CISO9660Directory::Exists(const CURL& url)
{
  CFileItemList items;
  return GetDirectory(url, items);
}

bool CISO9660Directory::Resolve(CFileItem& item) const
{
  const CURL url(item.GetDynPath());
  if (url.GetProtocol() != "iso9660" && url.GetFileType() != "iso")
  {
    return false;
  }

  // translate a generic iso9660:// url to the actual disc drive for playback
  if (!url.GetHostName().empty() && url.GetFileName() == "VIDEO_TS/video_ts.ifo")
  {
    item.SetDynPath(url.GetHostName());
  }
  return true;
}
