/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "File.h"
#include "IFile.h"

namespace XFILE
{
class CMusicDatabaseFile : public IFile
{
public:
  CMusicDatabaseFile(void);
  virtual ~CMusicDatabaseFile(void);
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);

  virtual ssize_t Read(void* lpBuf, size_t uiBufSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  virtual int64_t GetPosition();
  virtual int64_t GetLength();

  static std::string TranslateUrl(const CURL& url);
protected:
  CFile m_file;
};
}
