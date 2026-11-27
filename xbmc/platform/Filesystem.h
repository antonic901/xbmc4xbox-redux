/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
namespace KODI
{
namespace PLATFORM
{
namespace FILESYSTEM
{
struct space_info {
  uint64_t capacity;
  uint64_t free;
  uint64_t available;
};

space_info space(const std::string &path, int &ec);
}
}
}
