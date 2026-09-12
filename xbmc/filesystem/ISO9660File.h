/*
 *  Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *  Copyright (C) 2002-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFile.h"

#include "utils/RingBuffer.h"

namespace XFILE
{

class CISO9660File : public IFile
{
public:
  CISO9660File();
  virtual ~CISO9660File();

  virtual bool Open(const CURL& url);
  virtual void Close();

  virtual int Stat(const CURL& url, struct __stat64* buffer);

  virtual ssize_t Read(void* buffer, size_t size);
  virtual int64_t Seek(int64_t filePosition, int whence);

  virtual int64_t GetLength();
  virtual int64_t GetPosition();

  virtual bool Exists(const CURL& url);

private:
  bool m_bOpened;

  HANDLE m_hFile;
  CRingBuffer m_cache;
};

} // namespace XFILE
