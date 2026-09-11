/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/File.h"
#include "filesystem/IFile.h"

namespace XFILE
{
class COverrideFile : public IFile
{
public:
  explicit COverrideFile(bool writable);
  virtual ~COverrideFile();

  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual int Stat(struct __stat64* buffer);
  virtual bool OpenForWrite(const CURL& url, bool bOverWrite = false);
  virtual bool Delete(const CURL& url);
  virtual bool Rename(const CURL& url, const CURL& urlnew);

  virtual ssize_t Read(void* lpBuf, size_t uiBufSize);
  virtual ssize_t Write(const void* lpBuf, size_t uiBufSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  virtual int64_t GetPosition();
  virtual int64_t GetLength();

protected:
  virtual std::string TranslatePath(const CURL &url) = 0;

  CFile m_file;
  bool m_writable;
};
}
