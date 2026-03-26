/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ColorUtils.h"

using namespace UTILS::COLOR;

namespace
{

void GetHSLValues(ColorInfo& colorInfo)
{
  double r = (colorInfo.colorARGB & 0x00FF0000) >> 16;
  double g = (colorInfo.colorARGB & 0x0000FF00) >> 8;
  double b = (colorInfo.colorARGB & 0x000000FF);
  r /= 255;
  g /= 255;
  b /= 255;
  const double& maxVal = std::max<double>(std::max(r, g), b);
  const double& minVal = std::min<double>(std::min(r, g), b);
  double h = 0;
  double s = 0;
  double l = (minVal + maxVal) / 2;
  double d = maxVal - minVal;

  if (d == 0)
  {
    h = s = 0; // achromatic
  }
  else
  {
    s = l > 0.5 ? d / (2 - maxVal - minVal) : d / (maxVal + minVal);
    if (maxVal == r)
    {
      h = (g - b) / d + (g < b ? 6 : 0);
    }
    else if (maxVal == g)
    {
      h = (b - r) / d + 2;
    }
    else if (maxVal == b)
    {
      h = (r - g) / d + 4;
    }
    h /= 6;
  }

  colorInfo.hue = h;
  colorInfo.saturation = s;
  colorInfo.lightness = l;
}

} // unnamed namespace

Color UTILS::COLOR::ConvertHexToColor(const std::string& hexColor)
{
  Color value = 0;
  std::sscanf(hexColor.c_str(), "%x", &value);
  return value;
}

ColorInfo UTILS::COLOR::MakeColorInfo(const Color& argb)
{
  ColorInfo colorInfo;
  colorInfo.colorARGB = argb;
  GetHSLValues(colorInfo);
  return colorInfo;
}

ColorInfo UTILS::COLOR::MakeColorInfo(const std::string& hexColor)
{
  ColorInfo colorInfo;
  colorInfo.colorARGB = ConvertHexToColor(hexColor);
  GetHSLValues(colorInfo);
  return colorInfo;
}

bool UTILS::COLOR::comparePairColorInfo(const std::pair<std::string, ColorInfo>& a,
                                        const std::pair<std::string, ColorInfo>& b)
{
  if (a.second.hue == b.second.hue)
  {
    if (a.second.saturation == b.second.saturation)
      return (a.second.lightness < b.second.lightness);
    else
      return (a.second.saturation < b.second.saturation);
  }
  else
    return (a.second.hue < b.second.hue);
}

ColorFloats UTILS::COLOR::ConvertToFloats(const Color argb)
{
  ColorFloats c;
  c.alpha = static_cast<float>((argb >> 24) & 0xFF) * (1.0f / 255.0f);
  c.red = static_cast<float>((argb >> 16) & 0xFF) * (1.0f / 255.0f);
  c.green = static_cast<float>((argb >> 8) & 0xFF) * (1.0f / 255.0f);
  c.blue = static_cast<float>(argb & 0xFF) * (1.0f / 255.0f);
  return c;
}
