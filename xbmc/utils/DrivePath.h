/*
 *  Copyright (C) 2005-2018 Team Xodi
 *  This file is part of Xodi - https://xodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

// Shared by the application and FTP server. No XDK or application dependencies.
namespace DrivePath
{
inline bool IsLetter(char c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

inline bool IsName(const std::string& name)
{
  // Single letters are reserved for DVD, removable devices and application mounts.
  return (name.size() == 1 && std::string("DdHhIiJjKkLlMmNnOoQqTtUu").find(name[0]) != std::string::npos) ||
         (name.size() == 6 && (name[0] == 'H' || name[0] == 'h') &&
          (name[1] == 'D' || name[1] == 'd') &&
          (name[2] == 'D' || name[2] == 'd') &&
          (name[3] == '0' || name[3] == '1') && name[4] == '-' &&
          IsLetter(name[5]));
}

// Length excludes the colon; zero means this is not a local drive path.
inline std::string::size_type Length(const std::string& path)
{
  std::string::size_type colon = path.find(':');
  return colon != std::string::npos && IsName(path.substr(0, colon)) ? colon : 0;
}

inline bool IsRoot(const std::string& path)
{
  std::string::size_type length = Length(path);
  return length && (path.size() == length + 1 ||
         (path.size() == length + 2 && (path[length + 1] == '/' || path[length + 1] == '\\')));
}

inline char Upper(char c)
{
  return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
}

inline bool SameDrive(const std::string& a, const std::string& b)
{
  std::string::size_type length = Length(a);
  if (!length || length != Length(b))
    return false;
  for (std::string::size_type i = 0; i < length; ++i)
    if (Upper(a[i]) != Upper(b[i]))
      return false;
  return true;
}

// FTP exposes /HDD0-F/path while the permissions layer uses /HDD0-F:/path.
inline std::string FromFTP(const std::string& path)
{
  if (path.empty() || path[0] != '/')
    return path;
  std::string::size_type end = path.find('/', 1);
  std::string name = path.substr(1, end == std::string::npos ? end : end - 1);
  if (!IsName(name))
    return path;
  return "/" + name + ":" + (end == std::string::npos ? "" : path.substr(end));
}

inline std::string ToFTP(const std::string& path)
{
  if (path.empty() || path[0] != '/')
    return path;
  std::string::size_type length = Length(path.substr(1));
  if (!length)
    return path;
  std::string result = path.substr(0, length + 1);
  for (std::string::size_type i = 1; i < result.size(); ++i)
    result[i] = Upper(result[i]);
  return result + path.substr(length + 2);
}
}
