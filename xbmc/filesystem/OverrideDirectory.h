/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IDirectory.h"

namespace XFILE
{
class COverrideDirectory : public IDirectory
{
public:
  COverrideDirectory();
  virtual ~COverrideDirectory();

  virtual bool Create(const CURL& url);
  virtual bool Exists(const CURL& url);
  virtual bool Remove(const CURL& url);

protected:
  virtual std::string TranslatePath(const CURL &url) = 0;
};
}
