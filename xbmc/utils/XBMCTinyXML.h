/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <tinyxml.h>
#include <string>

class CXBMCTinyXML : public TiXmlDocument
{
public:
  CXBMCTinyXML();
  explicit CXBMCTinyXML(const char*);
  explicit CXBMCTinyXML(const std::string& documentName);
  CXBMCTinyXML(const std::string& documentName, const std::string& documentCharset);
  bool LoadFile(TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
  bool LoadFile(const char*, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
  bool LoadFile(const std::string& _filename, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
  bool LoadFile(const std::string& _filename, const std::string& documentCharset);
  bool LoadFile(FILE*, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
  bool SaveFile(const char*) const;
  bool SaveFile(const std::string& filename) const;
  bool Parse(const std::string& data, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);
  bool Parse(const std::string& data, const std::string& dataCharset);
  inline std::string GetSuggestedCharset(void) const { return m_SuggestedCharset; }
  inline std::string GetUsedCharset(void) const      { return m_UsedCharset; }
  static bool Test();
protected:
  using TiXmlDocument::Parse;
  bool TryParse(const std::string& data, const std::string& tryDataCharset);
  bool InternalParse(const std::string& rawdata, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING);

  std::string m_SuggestedCharset;
  std::string m_UsedCharset;
};
