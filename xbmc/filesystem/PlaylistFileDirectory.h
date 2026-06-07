/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"

namespace XFILE
{
  class CPlaylistFileDirectory : public IFileDirectory
  {
  public:
    CPlaylistFileDirectory();
    virtual ~CPlaylistFileDirectory();
    virtual bool GetDirectory(const CURL& url, CFileItemList& items);
    virtual bool ContainsFiles(const CURL& url);
    virtual bool Remove(const CURL& url);
    virtual bool AllowAll() const { return true; }
  };
}
