/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/File.h"
#include "taglib/XBOXbuild/xbox/taglib_xbmc.h"

namespace MUSIC_INFO
{
// Owns an XBMC VFS file. No TagLib headers or C++ classes are needed on the
// application side of the DLL boundary.
class TagLibVFSStream
{
public:
  explicit TagLibVFSStream(const std::string& filename);
  ~TagLibVFSStream();
  bool IsOpen() const { return m_open; }
  XbmcTagLibIO GetCallbacks();

private:
  TagLibVFSStream(const TagLibVFSStream&);
  TagLibVFSStream& operator=(const TagLibVFSStream&);
  static int XBMC_TAGLIB_CALL Read(void* context, void* buffer, unsigned int size);
  static XbmcTagLibOffset XBMC_TAGLIB_CALL Seek(void* context, XbmcTagLibOffset offset, int origin);
  static XbmcTagLibOffset XBMC_TAGLIB_CALL Length(void* context);
  XFILE::CFile m_file;
  bool m_open;
};
} // namespace MUSIC_INFO
