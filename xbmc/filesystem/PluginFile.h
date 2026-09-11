/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/OverrideFile.h"

namespace XFILE
{
class CPluginFile : public COverrideFile
{
public:
  CPluginFile(void);
  virtual ~CPluginFile(void);
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual int Stat(struct __stat64* buffer);
  virtual bool OpenForWrite(const CURL& url, bool bOverWrite = false);
  virtual bool Delete(const CURL& url);
  virtual bool Rename(const CURL& url, const CURL& urlnew);

protected:
  virtual std::string TranslatePath(const CURL& url);
};
} // namespace XFILE
