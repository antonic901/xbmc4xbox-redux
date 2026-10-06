/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CharsetConverter.h"

#include <xtl.h>

#include <boost/move/make_unique.hpp>
#include <boost/move/unique_ptr.hpp>

namespace KODI
{
namespace PLATFORM
{
namespace WINDOWS
{
std::string FromW(const wchar_t* str, size_t length)
{
  int result = WideCharToMultiByte(CP_UTF8, 0, str, length, NULL, 0, NULL, NULL);
  if (result == 0)
    return std::string();

  boost::movelib::unique_ptr<char[]> newStr = boost::movelib::make_unique<char[]>(result);
  result = WideCharToMultiByte(CP_UTF8, 0, str, length, newStr.get(), result, NULL, NULL);
  if (result == 0)
    return std::string();

  return std::string(newStr.get(), result);
}

std::string FromW(const std::wstring& str)
{
  return FromW(str.c_str(), str.length());
}

std::wstring ToW(const char* str, size_t length)
{
  int result = MultiByteToWideChar(CP_UTF8, 0, str, length, NULL, 0);
  if (result == 0)
    return std::wstring();

  boost::movelib::unique_ptr<wchar_t[]> newStr = boost::movelib::make_unique<wchar_t[]>(result);
  result = MultiByteToWideChar(CP_UTF8, 0, str, length, newStr.get(), result);

  if (result == 0)
    return std::wstring();

  return std::wstring(newStr.get(), result);
}

std::wstring ToW(const std::string& str)
{
  return ToW(str.c_str(), str.length());
}

}
}
}
