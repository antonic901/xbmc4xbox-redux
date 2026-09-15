/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HDFile.h"

#include "utils/log.h"
#include "utils/StringUtils.h"
#include "Util.h"
#include "URL.h"

#include <sys/stat.h>
#include <xtl.h>

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD) - 1)
#endif

using namespace XFILE;

CHDFile::CHDFile() : m_hFile(INVALID_HANDLE_VALUE)
{
}

CHDFile::~CHDFile()
{
  Close();
}

std::string CHDFile::GetLocal(const CURL& url)
{
  std::string path(url.GetFileName());

  if (url.IsProtocol("file"))
  {
    // file://drive[:]/path
    // file:///drive:/path
    std::string host(url.GetHostName());

    if (!host.empty())
    {
      if (host.size() > 0 && host.substr(host.size() - 1) == ":")
        path = host + "/" + path;
      else
        path = host + ":/" + path;
    }
  }

  StringUtils::Replace(path, '/', '\\');
  return path;
}

bool CHDFile::Open(const CURL& url)
{
  std::string strFile = GetLocal(url);

  m_hFile =
      CreateFile(strFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;

  m_i64FilePos = 0;
  m_i64FileLen = 0;

  LARGE_INTEGER i64Size;
  GetFileSizeEx(m_hFile, &i64Size);
  m_i64FileLength = i64Size.QuadPart;

  return true;
}

bool CHDFile::Exists(const CURL& url)
{
  std::string strFile = GetLocal(url);
  const DWORD attrs = GetFileAttributesA(strFile.c_str());
  return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

int CHDFile::Stat(const CURL& url, struct __stat64* buffer)
{
  std::string strFile = GetLocal(url);

  HANDLE hSearch;
  WIN32_FIND_DATAA findData;
  hSearch = FindFirstFile(strFile.c_str(), &findData);
  if (hSearch == INVALID_HANDLE_VALUE)
    return -1;
  CloseHandle(hSearch);

  buffer->st_dev = 0;
  buffer->st_ino = 0;
  buffer->st_mode = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? S_IFDIR : S_IFREG;
  buffer->st_nlink = 1;
  buffer->st_uid = 0;
  buffer->st_gid = 0;
  buffer->st_rdev = 0;

  buffer->st_size = static_cast<_off_t>((static_cast<__int64>(findData.nFileSizeHigh) << 32) |
                                        findData.nFileSizeLow);

  buffer->st_atime = static_cast<__time64_t>(findData.ftLastAccessTime.dwLowDateTime);
  buffer->st_mtime = static_cast<__time64_t>(findData.ftLastWriteTime.dwLowDateTime);
  buffer->st_ctime = static_cast<__time64_t>(findData.ftCreationTime.dwLowDateTime);

  return 0;
}

bool CHDFile::SetHidden(const CURL& url, bool hidden)
{
  std::string path = GetLocal(url);

  DWORD attributes = hidden ? FILE_ATTRIBUTE_HIDDEN : FILE_ATTRIBUTE_NORMAL;
  if (SetFileAttributesA(path.c_str(), attributes))
    return true;

  return false;
}

bool CHDFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  // make sure it's a legal FATX filename (we are writing to the harddisk)
  std::string strPath = GetLocal(url);

  std::string strPathOriginal = strPath;
  strPath = CUtil::GetFatXQualifiedPath(strPath);
  if (strPathOriginal != strPath)
    CLog::Log(LOGINFO, "CHDFile::OpenForWrite - Truncated filename: %s -> %s",
              strPathOriginal.c_str(), strPath.c_str());

  m_hFile = CreateFile(strPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                       bOverWrite ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;

  m_i64FilePos = 0;
  LARGE_INTEGER i64Size;
  GetFileSizeEx(m_hFile, &i64Size);
  m_i64FileLength = i64Size.QuadPart;
  Seek(0, SEEK_SET);

  return true;
}

ssize_t CHDFile::Read(void* lpBuf, size_t uiBufSize)
{
  assert(lpBuf != NULL);
  if (m_hFile == INVALID_HANDLE_VALUE)
    return -1;

  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  DWORD nBytesRead;
  if (ReadFile(m_hFile, lpBuf, (DWORD)uiBufSize, &nBytesRead, NULL))
  {
    m_i64FilePos += nBytesRead;
    return nBytesRead;
  }
  return -1;
}

ssize_t CHDFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return 0;

  DWORD nBytesWriten;
  if (WriteFile(m_hFile, (void*)lpBuf, (DWORD)uiBufSize, &nBytesWriten, NULL))
    return nBytesWriten;

  return 0;
}

void CHDFile::Close()
{
  if (m_hFile != INVALID_HANDLE_VALUE)
    CloseHandle(m_hFile);

  m_hFile = INVALID_HANDLE_VALUE;
}

int64_t CHDFile::Seek(int64_t iFilePosition, int iWhence)
{
  LARGE_INTEGER lPos, lNewPos;
  lPos.QuadPart = iFilePosition;
  int bSuccess;

  switch (iWhence)
  {
    case SEEK_SET:
      bSuccess = SetFilePointerEx(m_hFile, lPos, &lNewPos, FILE_BEGIN);
      break;

    case SEEK_CUR:
      bSuccess = SetFilePointerEx(m_hFile, lPos, &lNewPos, FILE_CURRENT);
      break;

    case SEEK_END:
      bSuccess = SetFilePointerEx(m_hFile, lPos, &lNewPos, FILE_END);
      break;

    default:
      return -1;
  }
  if (bSuccess)
  {
    m_i64FilePos = lNewPos.QuadPart;
    return m_i64FilePos;
  }
  else
    return -1;
}

int64_t CHDFile::GetLength()
{
  if (m_i64FileLen <= m_i64FilePos || m_i64FileLen == 0)
  {
    LARGE_INTEGER i64Size;
    if (GetFileSizeEx((HANDLE)m_hFile, &i64Size))
      m_i64FileLen = i64Size.QuadPart;
    else
      CLog::Log(LOGERROR, "CHDFile::GetLength - GetFileSizeEx failed with error %d",
                GetLastError());
  }
  return m_i64FileLen;
}

int64_t CHDFile::GetPosition()
{
  return m_i64FilePos;
}

bool CHDFile::Delete(const CURL& url)
{
  std::string strFile = GetLocal(url);

  return ::DeleteFile(strFile.c_str()) ? true : false;
}

bool CHDFile::Rename(const CURL& url, const CURL& urlnew)
{
  std::string strFile = GetLocal(url);
  std::string strNewFile = GetLocal(urlnew);

  return ::MoveFile(strFile.c_str(), strNewFile.c_str()) ? true : false;
}

void CHDFile::Flush()
{
  FlushFileBuffers(m_hFile);
}
