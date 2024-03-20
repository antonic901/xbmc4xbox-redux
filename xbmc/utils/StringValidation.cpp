/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "StringValidation.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

bool StringValidation::IsInteger(const std::string &input, void *data)
{
  return StringUtils2::IsInteger(input);
}

bool StringValidation::IsPositiveInteger(const std::string &input, void *data)
{
  return StringUtils2::IsNaturalNumber(input);
}

bool StringValidation::IsTime(const std::string &input, void *data)
{
  std::string strTime = input;
  StringUtils2::Trim(strTime);

  if (StringUtils2::EndsWith(strTime, " min"))
  {
    strTime = StringUtils2::Left(strTime, strTime.size() - 4);
    StringUtils2::TrimRight(strTime);

    return IsPositiveInteger(strTime, NULL);
  }
  else
  {
    size_t pos = strTime.find(":");
    // if there's no ":", the value must be in seconds only
    if (pos == std::string::npos)
      return IsPositiveInteger(strTime, NULL);

    std::string strMin = StringUtils2::Left(strTime, pos);
    std::string strSec = StringUtils2::Mid(strTime, pos + 1);
    return IsPositiveInteger(strMin, NULL) && IsPositiveInteger(strSec, NULL);
  }
  return false;
}
