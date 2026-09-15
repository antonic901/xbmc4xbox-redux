/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IFile.h"

#include <string>

typedef void* HANDLE; // forward declaration

namespace XFILE
{
class CHDFile : public IFile
{
public:
  CHDFile();
  virtual ~CHDFile();

  virtual bool Open(const CURL& url);
  virtual bool OpenForWrite(const CURL& url, bool bOverWrite = false);
  virtual void Close();

  virtual ssize_t Read(void* lpBuf, size_t uiBufSize);
  virtual ssize_t Write(const void* lpBuf, size_t uiBufSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual int64_t GetPosition();
  virtual int64_t GetLength();
  virtual void Flush();

  virtual bool Delete(const CURL& url);
  virtual bool Rename(const CURL& url, const CURL& urlnew);
  virtual bool SetHidden(const CURL& url, bool hidden);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);

protected:
  std::string GetLocal(const CURL& url); /* crate a properly format path from an url */
  HANDLE m_hFile;

  int64_t m_i64FileLength;
  int64_t m_i64FilePos;
  int64_t m_i64FileLen;
};

} // namespace XFILE
