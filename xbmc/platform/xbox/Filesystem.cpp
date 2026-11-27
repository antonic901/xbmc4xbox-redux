/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/Filesystem.h"

#include "platform/xbox/storage/IoSupport.h"

namespace KODI
{
namespace PLATFORM
{
namespace FILESYSTEM
{
space_info space(const std::string& path, int& ec)
{
  ec = 0;
  space_info sp;

  // Translate mount path to real path
  const std::string diskPath = CIoSupport::TranslateMountPath(path);

  ULARGE_INTEGER capacity;
  ULARGE_INTEGER available;
  ULARGE_INTEGER free;
  BOOL result = GetDiskFreeSpaceEx(diskPath.c_str(), &available, &capacity, &free);

  if (result == FALSE)
  {
    ec = GetLastError();
    sp.available = static_cast<uintmax_t>(-1);
    sp.capacity = static_cast<uintmax_t>(-1);
    sp.free = static_cast<uintmax_t>(-1);
    return sp;
  }

  sp.available = static_cast<uintmax_t>(available.QuadPart);
  sp.capacity = static_cast<uintmax_t>(capacity.QuadPart);
  sp.free = static_cast<uintmax_t>(free.QuadPart);

  return sp;
}

} // namespace FILESYSTEM
} // namespace PLATFORM
} // namespace KODI
