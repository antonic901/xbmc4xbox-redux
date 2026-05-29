/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DirectoryNode.h"

namespace XFILE
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeInProgressTvShows : public CDirectoryNode
    {
    public:
      CDirectoryNodeInProgressTvShows(const std::string& strEntryName, CDirectoryNode* pParent);
    protected:
      virtual NODE_TYPE GetChildType() const;
      virtual bool GetContent(CFileItemList& items) const;
      virtual std::string GetLocalizedName() const;
    };
  }
}


