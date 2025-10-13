#pragma once
/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "IDirectory.h"

namespace XFILE
{
  class CProgramDatabaseDirectory : public IDirectory
  {
  public:
    CProgramDatabaseDirectory(void);
    virtual ~CProgramDatabaseDirectory(void);
    virtual bool GetDirectory(const CURL& url, CFileItemList &items);
    virtual bool AllowAll() const { return true; }
  };
}
