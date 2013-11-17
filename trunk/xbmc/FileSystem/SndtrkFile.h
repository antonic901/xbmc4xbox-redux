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
// FileSndtrk.h: interface for the CSndtrkFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FileSndtrk_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_)
#define AFX_FileSndtrk_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"
#include "AutoPtrHandle.h"

namespace XFILE
{
class CSndtrkFile : public IFile
{
public:
  CSndtrkFile();
  virtual ~CSndtrkFile();
  virtual int64_t GetPosition();
  virtual int64_t GetLength();
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url) { return true;};
  virtual int Stat(const CURL& url, struct __stat64* buffer) { errno = ENOENT; return -1; };
  virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
  virtual int Write(const void* lpBuf, int64_t uiBufSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();

  bool OpenForWrite(const char* strFileName);
  unsigned int Write(void *lpBuf, int64_t uiBufSize);
protected:
  AUTOPTR::CAutoPtrHandle m_hFile;
  int64_t m_i64FileLength;
  int64_t m_i64FilePos;
};

};
#endif // !defined(AFX_FileSndtrk_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_)
