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

  class CHDDirectory : public IDirectory
  {
  public:
    CHDDirectory(void);
    virtual ~CHDDirectory(void);
    virtual bool GetDirectory(const CURL& url, CFileItemList &items);
    virtual bool Create(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual bool Remove(const CURL& url);
    virtual bool RemoveRecursive(const CURL& url);

  private:
    bool Create(std::string path) const;
  };
}
