/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"

#include <string>

class CSmartPlaylist;

namespace XFILE
{
  class CSmartPlaylistDirectory : public IFileDirectory
  {
  public:
    CSmartPlaylistDirectory();
    virtual ~CSmartPlaylistDirectory();
    virtual bool GetDirectory(const CURL& url, CFileItemList& items);
    virtual bool AllowAll() const { return true; }
    virtual bool ContainsFiles(const CURL& url);
    virtual bool Remove(const CURL& url);

    static bool GetDirectory(const CSmartPlaylist &playlist, CFileItemList& items, const std::string &strBaseDir = "", bool filter = false);

    static std::string GetPlaylistByName(const std::string& name, const std::string& playlistType);
  };
}
