/*
 *  Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *  Copyright (C) 2002-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ISO9660File.h"

#include "URL.h"
#include "iso9660.h"

#include <algorithm>

#include <sys/stat.h>

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//*********************************************************************************************
CISO9660File::CISO9660File()
  : m_hFile(INVALID_HANDLE_VALUE)
{
  m_bOpened = false;
}

//*********************************************************************************************
CISO9660File::~CISO9660File()
{
  if (m_bOpened)
  {
    Close();
  }
}
//*********************************************************************************************
bool CISO9660File::Open(const CURL& url)
{
  std::string strFName = "\\";
  strFName += url.GetFileName();
  for (int i = 0; i < (int)strFName.size(); ++i )
  {
    if (strFName[i] == '/') strFName[i] = '\\';
  }
  m_hFile = m_isoReader.OpenFile(strFName.c_str());
  if (m_hFile == INVALID_HANDLE_VALUE)
  {
    m_bOpened = false;
    return false;
  }

  m_bOpened = true;
  return true;
}

//*********************************************************************************************
ssize_t CISO9660File::Read(void *buffer, size_t size)
{
  if (!m_bOpened)
    return -1;
  if (size > SSIZE_MAX)
    size = SSIZE_MAX;

  char *pData = (char *)buffer;

  if (m_cache.getSize() > 0)
  {
    long lTotalBytesRead = 0;
    while (size > 0)
    {
      if (m_cache.getMaxReadSize() )
      {
        unsigned int lBytes2Read = std::min(m_cache.getMaxReadSize(), static_cast<unsigned int>(size));
        m_cache.ReadData(pData, lBytes2Read );
        size -= lBytes2Read ;
        pData += lBytes2Read;
        lTotalBytesRead += lBytes2Read ;
      }

      if (m_cache.getMaxWriteSize() > 5000)
      {
        uint8_t buffer[5000];
        long lBytesRead = m_isoReader.ReadFile( m_hFile, buffer, sizeof(buffer));
        if (lBytesRead > 0)
          m_cache.WriteData((char*)buffer, lBytesRead);
        else
          return 0;
      }
    }
    return lTotalBytesRead;
  }

  return m_isoReader.ReadFile( m_hFile, (uint8_t*)pData, (long)size);
}

//*********************************************************************************************
void CISO9660File::Close()
{
  if (!m_bOpened) return ;
  m_isoReader.CloseFile( m_hFile);
}

//*********************************************************************************************
int64_t CISO9660File::Seek(int64_t filePosition, int whence)
{
  if (!m_bOpened) return -1;
  int64_t lNewPos = m_isoReader.Seek(m_hFile, filePosition, whence);
  if(lNewPos >= 0)
    m_cache.Clear();
  return lNewPos;
}

//*********************************************************************************************
int64_t CISO9660File::GetLength()
{
  if (!m_bOpened) return -1;
  return m_isoReader.GetFileSize(m_hFile);
}

//*********************************************************************************************
int64_t CISO9660File::GetPosition()
{
  if (!m_bOpened) return -1;
  return m_isoReader.GetFilePosition(m_hFile);
}

bool CISO9660File::Exists(const CURL& url)
{
  std::string strFName = "\\";
  strFName += url.GetFileName();
  for (int i = 0; i < (int)strFName.size(); ++i )
  {
    if (strFName[i] == '/') strFName[i] = '\\';
  }
  m_hFile = m_isoReader.OpenFile(strFName.c_str());
  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;

  m_isoReader.CloseFile(m_hFile);
  return true;
}

int CISO9660File::Stat(const CURL& url, struct __stat64* buffer)
{
  std::string strFName = "\\";
  strFName += url.GetFileName();
  for (int i = 0; i < (int)strFName.size(); ++i )
  {
    if (strFName[i] == '/') strFName[i] = '\\';
  }
  m_hFile = m_isoReader.OpenFile(strFName.c_str());
  if (m_hFile != INVALID_HANDLE_VALUE)
  {
    memset(buffer, 0, sizeof(struct __stat64));
    buffer->st_size = m_isoReader.GetFileSize(m_hFile);
    buffer->st_mode = _S_IFREG;
    m_isoReader.CloseFile(m_hFile);
    return 0;
  }
  errno = ENOENT;
  return -1;
}
