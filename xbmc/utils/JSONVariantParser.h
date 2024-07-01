#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "Variant.h"
#include "jsoncpp/json/json.h"

class IParseCallback
{
public:
  virtual ~IParseCallback() { }

  virtual void onParsed(CVariant *variant) = 0;
};

class CSimpleParseCallback : public IParseCallback
{
public:
  virtual void onParsed(CVariant *variant) { m_parsed = *variant; }
  CVariant &GetOutput() { return m_parsed; }

private:
  CVariant m_parsed;
};

class CJSONVariantParser
{
public:
  CJSONVariantParser(IParseCallback *callback);
  ~CJSONVariantParser();

  void push_buffer(const unsigned char *buffer, unsigned int length);

  static CVariant Parse(const unsigned char *json, unsigned int length);

  static CVariant Parse(const std::string& json);

private:
  static CVariant ConvertJsonValueToCVariant(const Json::Value &jsonValue);

  IParseCallback *m_callback;

  CVariant m_parsedObject;
  std::vector<CVariant *> m_parse;
  std::string m_key;

  Json::CharReaderBuilder readerBuilder;
  Json::CharReader *reader;
};
