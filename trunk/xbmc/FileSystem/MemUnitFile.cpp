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
#include "MemUnitFile.h"
#include <sys/stat.h>
#include "utils/MemoryUnitManager.h"
#include "MemoryUnits/IFileSystem.h"
#include "MemoryUnits/IDevice.h"
#include "URL.h"

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//*********************************************************************************************
CMemUnitFile::CMemUnitFile()
{
  m_fileSystem = NULL;
}

//*********************************************************************************************
CMemUnitFile::~CMemUnitFile()
{
  Close();
}
//*********************************************************************************************
bool CMemUnitFile::Open(const CURL& url)
{
  Close();

  m_fileSystem = GetFileSystem(url);
  if (!m_fileSystem) return false;

  return m_fileSystem->Open(GetPath(url));
}

bool CMemUnitFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  Close();

  m_fileSystem = GetFileSystem(url);
  if (!m_fileSystem) return false;

  return m_fileSystem->OpenForWrite(GetPath(url), bOverWrite);
}

//*********************************************************************************************
unsigned int CMemUnitFile::Read(void *lpBuf, int64_t uiBufSize)
{
  if (!m_fileSystem) return 0;
  return m_fileSystem->Read(lpBuf, uiBufSize);
}

int CMemUnitFile::Write(const void* lpBuf, int64_t uiBufSize)
{
  if (!m_fileSystem) return 0;
  return m_fileSystem->Write(lpBuf, uiBufSize);
}

//*********************************************************************************************
void CMemUnitFile::Close()
{
  if (m_fileSystem)
  {
    m_fileSystem->Close();
    delete m_fileSystem;
  }
  m_fileSystem = NULL;
}

//*********************************************************************************************
int64_t CMemUnitFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_fileSystem) return -1;
  int64_t position = iFilePosition;
  if (iWhence == SEEK_CUR)
    position += m_fileSystem->GetPosition();
  else if (iWhence == SEEK_END)
    position += m_fileSystem->GetLength();
  else if (iWhence != SEEK_SET)
    return -1;

  if (position < 0) position = 0;
  if (position > m_fileSystem->GetLength()) position = m_fileSystem->GetLength();
  return m_fileSystem->Seek(position);
}

//*********************************************************************************************
int64_t CMemUnitFile::GetLength()
{
  if (!m_fileSystem) return -1;
  return m_fileSystem->GetLength();
}

//*********************************************************************************************
int64_t CMemUnitFile::GetPosition()
{
  if (!m_fileSystem) return -1;
  return m_fileSystem->GetPosition();
}

bool CMemUnitFile::Exists(const CURL& url)
{
  if (Open(url))
  {
    Close();
    return true;
  }
  return false;
}

int CMemUnitFile::Stat(const CURL& url, struct __stat64* buffer)
{
  if (Open(url))
  {
    Close();
    return 0;
  }
  return -1;
}

bool CMemUnitFile::Delete(const CURL& url)
{
  IFileSystem *fileSystem = GetFileSystem(url);
  if (fileSystem)
    return fileSystem->Delete(GetPath(url));
  return false;
}

bool CMemUnitFile::Rename(const CURL& url, const CURL& urlnew)
{
  IFileSystem *fileSystem = GetFileSystem(url);
  if (fileSystem)
    return fileSystem->Rename(GetPath(url), GetPath(urlnew));
  return false;
}

IFileSystem *CMemUnitFile::GetFileSystem(const CURL& url)
{
  unsigned char unit = url.GetProtocol()[3] - '0';
  return g_memoryUnitManager.GetFileSystem(unit);
}

CStdString CMemUnitFile::GetPath(const CURL& url)
{
  CStdString path = url.GetFileName();
  path.Replace("\\", "/");
  return path;
}
