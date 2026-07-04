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

enum RESOLUTION
{
  RES_INVALID        = -1,
  RES_HDTV_1080i     =  0,
  RES_HDTV_720p      =  1,
  RES_HDTV_480p_4x3  =  2,
  RES_HDTV_480p_16x9 =  3,
  RES_NTSC_4x3       =  4,
  RES_NTSC_16x9      =  5,
  RES_PAL_4x3        =  6,
  RES_PAL_16x9       =  7,
  RES_PAL60_4x3      =  8,
  RES_PAL60_16x9     =  9,
  RES_AUTORES        = 10
};

struct OVERSCAN
{
  int left;
  int top;
  int right;
  int bottom;
public:
  OVERSCAN()
  {
    left = top = right = bottom = 0;
  }

  bool operator==(const OVERSCAN& other)
  {
    return left == other.left && right == other.right && top == other.top && bottom == other.bottom;
  }
  bool operator!=(const OVERSCAN& other)
  {
    return left != other.left || right != other.right || top != other.top || bottom != other.bottom;
  }
};

//! @brief Provide info of a resolution
struct RESOLUTION_INFO
{
  //!< Screen overscan boundary
  OVERSCAN Overscan;

  //!< Width GUI resolution (pixels), may differ from the screen value if GUI resolution limit, 3D is set or in HiDPI screens
  int iWidth;

  //!< Height GUI resolution (pixels), may differ from the screen value if GUI resolution limit, 3D is set or in HiDPI screens
  int iHeight;

  //!< The vertical subtitle baseline position, may be changed by Video calibration
  int iSubtitles;

  //!< Properties of the resolution e.g. interlaced mode
  uint32_t dwFlags;

  //!< Pixel aspect ratio
  float fPixelRatio;

  //!< Resolution mode description
  std::string strMode;

  //!< Resolution ID
  std::string strId;

public:
  RESOLUTION_INFO(int width = 1280, int height = 720, float aspect = 0, const std::string &mode = "");
  float DisplayRatio() const;
};
