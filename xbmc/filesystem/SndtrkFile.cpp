/*
 * XBMC Media Center
 * Copyright (c) 2002 Frodo
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "SndtrkFile.h"
#include "URL.h"

using namespace XFILE;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*********************************************************************************************
CSndtrkFile::CSndtrkFile()
    : m_hFile(INVALID_HANDLE_VALUE)
{}

//*********************************************************************************************
CSndtrkFile::~CSndtrkFile()
{
}
//*********************************************************************************************
bool CSndtrkFile::Open(const CURL& url)
{
  m_hFile.attach( CreateFile(url.GetFileName().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL));
  if ( !m_hFile.isValid() ) return false;

  m_i64FilePos = 0;
  LARGE_INTEGER i64Size;
  GetFileSizeEx((HANDLE)m_hFile, &i64Size);
  m_i64FileLength = i64Size.QuadPart;
  Seek(0, SEEK_SET);

  return true;
}
//*********************************************************************************************
bool CSndtrkFile::OpenForWrite(const char* strFileName)
{
  m_hFile.attach(CreateFile(strFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
  if (!m_hFile.isValid()) return false;

  m_i64FilePos = 0;
  LARGE_INTEGER i64Size;
  GetFileSizeEx((HANDLE)m_hFile, &i64Size);
  m_i64FileLength = i64Size.QuadPart;
  Seek(0, SEEK_SET);

  return true;
}

//*********************************************************************************************
ssize_t CSndtrkFile::Read(void *lpBuf, size_t uiBufSize)
{
  if (!m_hFile.isValid())
    return -1;

  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  DWORD nBytesRead;
  if ( ReadFile((HANDLE)m_hFile, lpBuf, (DWORD)uiBufSize, &nBytesRead, NULL) )
  {
    m_i64FilePos += nBytesRead;
    return nBytesRead;
  }
  return -1;
}

//*********************************************************************************************
unsigned int CSndtrkFile::Write(void *lpBuf, int64_t uiBufSize)
{
  if (!m_hFile.isValid()) return 0;
  DWORD nBytesWriten;
  if ( WriteFile((HANDLE)m_hFile, lpBuf, (DWORD)uiBufSize, &nBytesWriten, NULL) )
  {
    return nBytesWriten;
  }
  return 0;
}

//*********************************************************************************************
void CSndtrkFile::Close()
{
  m_hFile.reset();
}

//*********************************************************************************************
int64_t CSndtrkFile::Seek(int64_t iFilePosition, int iWhence)
{
  LARGE_INTEGER lPos, lNewPos;
  lPos.QuadPart = iFilePosition;
  switch (iWhence)
  {
  case SEEK_SET:
    SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_BEGIN);
    break;

  case SEEK_CUR:
    SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_CURRENT);
    break;

  case SEEK_END:
    SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_END);
    break;
  default:
    return -1;
  }
  m_i64FilePos = lNewPos.QuadPart;
  return (lNewPos.QuadPart);
}

//*********************************************************************************************
int64_t CSndtrkFile::GetLength()
{
  LARGE_INTEGER i64Size;
  GetFileSizeEx((HANDLE)m_hFile, &i64Size);
  return i64Size.QuadPart;

  // return m_i64FileLength;
}

//*********************************************************************************************
int64_t CSndtrkFile::GetPosition()
{
  return m_i64FilePos;
}


ssize_t CSndtrkFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (!m_hFile.isValid()) return -1;
  DWORD dwNumberOfBytesWritten = 0;
  WriteFile((HANDLE)m_hFile, lpBuf, (DWORD)uiBufSize, &dwNumberOfBytesWritten, NULL);
  return (ssize_t)dwNumberOfBytesWritten;
}
