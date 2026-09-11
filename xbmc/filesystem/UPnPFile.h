/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFile.h"

namespace XFILE
{
  class CUPnPFile : public IFile
  {
    public:
      CUPnPFile();
      virtual ~CUPnPFile();
      virtual bool Open(const CURL& url);
      virtual bool Exists(const CURL& url);
      virtual int Stat(const CURL& url, struct __stat64* buffer);

      virtual ssize_t Read(void* lpBuf, size_t uiBufSize) {return -1;}
      virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) {return -1;}
      virtual void Close(){}
      virtual int64_t GetPosition() {return -1;}
      virtual int64_t GetLength() {return -1;}
  };
}
