/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlatformDefs.h"

class CAEUtil
{
public:
  /*! \brief convert a volume percentage (as a proportion) to a dB gain
   We assume a dB range of 60dB, i.e. assume that 0% volume corresponds
   to a reduction of 60dB.
   \param value the volume from 0..1
   \return the corresponding gain in dB from -60dB .. 0dB.
   \sa GainToScale
   */
  static inline float PercentToGain(const float value)
  {
    static const float db_range = 60.0f;
    return (value - 1)*db_range;
  }
};
