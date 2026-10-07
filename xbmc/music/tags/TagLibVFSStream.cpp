/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TagLibVFSStream.h"

#include <climits>
#include <cstdio>

using namespace MUSIC_INFO;

TagLibVFSStream::TagLibVFSStream(const std::string& filename) : m_open(m_file.Open(filename))
{
}
TagLibVFSStream::~TagLibVFSStream()
{
  m_file.Close();
}

XbmcTagLibIO TagLibVFSStream::GetCallbacks()
{
  XbmcTagLibIO io = {this, Read, Seek, Length};
  return io;
}

int XBMC_TAGLIB_CALL TagLibVFSStream::Read(void* context, void* buffer, unsigned int size)
{
  try
  {
    if (size > INT_MAX)
      return -1;
    return static_cast<int>(static_cast<TagLibVFSStream*>(context)->m_file.Read(buffer, size));
  }
  catch (...)
  {
    return -1;
  }
}

XbmcTagLibOffset XBMC_TAGLIB_CALL TagLibVFSStream::Seek(void* context,
                                                        XbmcTagLibOffset offset,
                                                        int origin)
{
  try
  {
    if (origin < 0 || origin > 2)
      return -1;
    return static_cast<TagLibVFSStream*>(context)->m_file.Seek(offset, origin == 0   ? SEEK_SET
                                                                       : origin == 1 ? SEEK_CUR
                                                                                     : SEEK_END);
  }
  catch (...)
  {
    return -1;
  }
}

XbmcTagLibOffset XBMC_TAGLIB_CALL TagLibVFSStream::Length(void* context)
{
  try
  {
    return static_cast<TagLibVFSStream*>(context)->m_file.GetLength();
  }
  catch (...)
  {
    return -1;
  }
}
