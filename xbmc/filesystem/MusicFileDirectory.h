/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"
#include "music/tags/MusicInfoTag.h"

namespace XFILE
{
  class CMusicFileDirectory : public IFileDirectory
  {
    public:
      CMusicFileDirectory(void);
      virtual ~CMusicFileDirectory(void);
      virtual bool GetDirectory(const CURL& url, CFileItemList &items);
      virtual bool Exists(const CURL& url);
      virtual bool ContainsFiles(const CURL& url);
      virtual bool AllowAll() const { return true; }
    protected:
      virtual bool Load(const std::string& strFileName,
                        MUSIC_INFO::CMusicInfoTag& tag,
                        EmbeddedArt* art = nullptr) { return false; }
      virtual int GetTrackCount(const std::string& strPath) = 0;
      std::string m_strExt;
      MUSIC_INFO::CMusicInfoTag m_tag;
  };
}
