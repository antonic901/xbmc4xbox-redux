/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

namespace XFILE
{
  class CMusicSearchDirectory : public IDirectory
  {
  public:
    CMusicSearchDirectory(void);
    virtual ~CMusicSearchDirectory(void);
    virtual bool GetDirectory(const CURL& url, CFileItemList &items);
    virtual bool Exists(const CURL& url);
    virtual bool AllowAll() const { return true; }
  };
}
