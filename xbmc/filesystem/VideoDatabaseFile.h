/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/OverrideFile.h"
#include "video/VideoDatabase.h" // VideoDbContentType

class CVideoInfoTag;
class CURL;

namespace XFILE
{
class CVideoDatabaseFile : public COverrideFile
{
public:
  CVideoDatabaseFile(void);
  virtual ~CVideoDatabaseFile(void);

  static CVideoInfoTag GetVideoTag(const CURL& url);

protected:
  virtual std::string TranslatePath(const CURL& url);
  static VideoDbContentType::Type GetType(const CURL& url);
};
}
