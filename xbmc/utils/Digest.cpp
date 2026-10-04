/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Digest.h"

#include "StringUtils.h"
#include "log.h"

#include <stdexcept>

namespace KODI
{
namespace UTILITY
{

std::ostream& operator<<(std::ostream& os, TypedDigest const& digest)
{
  return os << "{" << CDigest::TypeToString(digest.type) << "}" << digest.value;
}

std::string CDigest::TypeToString(Type type)
{
  switch (type)
  {
    case Type::MD5:
      return "md5";
    case Type::SHA1:
      return "sha1";
    case Type::SHA256:
      return "sha256";
    case Type::SHA512:
      return "sha512";
    case Type::INVALID:
      return "invalid";
    default:
      throw std::invalid_argument("Unknown digest type");
  }
}

CDigest::Type CDigest::TypeFromString(std::string const& type)
{
  std::string typeLower(type);
  StringUtils::ToLower(typeLower);
  if (type == "md5")
  {
    return Type::MD5;
  }
  else if (type == "sha1")
  {
    return Type::SHA1;
  }
  else if (type == "sha256")
  {
    return Type::SHA256;
  }
  else if (type == "sha512")
  {
    return Type::SHA512;
  }
  else
  {
    throw std::invalid_argument(std::string("Unknown digest type \"") + type + "\"");
  }
}

CDigest::CDigest(Type type) : m_finalized(false), m_type(type)
{
  if (type == Type::SHA256)
    XBMC::SHA256Init(&m_context.sha256);
  else
  {
    if (type != Type::MD5)
      CLog::Log(LOGWARNING, "Digest of type %s is not supported. Falling back to MD5", TypeToString(type).c_str());
    m_type = Type::MD5;
    MD5Init(&m_context.md5);
  }
}

void CDigest::Update(std::string const& data)
{
  Update(data.c_str(), data.size());
}

void CDigest::Update(void const* data, std::size_t size)
{
  if (m_finalized)
  {
    throw std::logic_error("Finalized digest cannot be updated any more");
  }

  if (m_type == Type::SHA256)
    XBMC::SHA256Update(&m_context.sha256, static_cast<const unsigned char*>(data), size);
  else
    MD5Update(&m_context.md5, static_cast<const md5byte*>(data), size);
}

std::string CDigest::FinalizeRaw()
{
  if (m_finalized)
  {
    throw std::logic_error("Digest can only be finalized once");
  }

  m_finalized = true;

  unsigned char digest[32];
  if (m_type == Type::SHA256)
  {
    XBMC::SHA256Final(&m_context.sha256, digest);
    return std::string(reinterpret_cast<const char*>(digest), 32);
  }

  MD5Final(digest, &m_context.md5);
  return std::string(reinterpret_cast<const char*>(digest), 16);
}

std::string CDigest::Finalize()
{
  return StringUtils::ToHexadecimal(FinalizeRaw());
}

std::string CDigest::Calculate(Type type, std::string const& data)
{
  CDigest digest(type);
  digest.Update(data);
  return digest.Finalize();
}

std::string CDigest::Calculate(Type type, void const* data, std::size_t size)
{
  CDigest digest(type);
  digest.Update(data, size);
  return digest.Finalize();
}

}
}
