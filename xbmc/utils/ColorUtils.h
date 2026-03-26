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

namespace UTILS
{
namespace COLOR
{

typedef uint32_t Color;

struct ColorInfo
{
  Color colorARGB;
  double hue;
  double saturation;
  double lightness;
};

struct ColorFloats
{
  float red;
  float green;
  float blue;
  float alpha;
};

/*!
 * \brief Convert given hex value to Color value
 * \param hexColor The original hex color
 * \return the original hex color converted to Color value
 */
Color ConvertHexToColor(const std::string& hexColor);

/*!
 * \brief Create a ColorInfo from an ARGB Color to
 *        get additional information of the color
 *        and allow to be sorted with a color comparer
 * \param argb The original ARGB color
 * \return the ColorInfo
 */
ColorInfo MakeColorInfo(const Color& argb);

/*!
 * \brief Create a ColorInfo from an HEX color value to
 *        get additional information of the color
 *        and allow to be sorted with a color comparer
 * \param hexColor The original ARGB color
 * \return the ColorInfo
 */
ColorInfo MakeColorInfo(const std::string& hexColor);

/*!
 * \brief Comparer for pair string/ColorInfo to sort colors in a hue scale
 */
bool comparePairColorInfo(const std::pair<std::string, ColorInfo>& a,
                          const std::pair<std::string, ColorInfo>& b);

/*!
 * \brief Convert given ARGB color to ColorFloats
 * \param color The original color
 * \return the original color converted to ColorFloats
 */
ColorFloats ConvertToFloats(const Color argb);

} // namespace COLOR
} // namespace UTILS
