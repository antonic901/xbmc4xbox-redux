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

#include "JSONVariantParser.h"

CJSONVariantParser::CJSONVariantParser(IParseCallback *callback)
{
  m_callback = callback;
  reader = readerBuilder.newCharReader();
}

CJSONVariantParser::~CJSONVariantParser()
{
  delete reader;
}

void CJSONVariantParser::push_buffer(const unsigned char *buffer, unsigned int length)
{
  Json::Value root;
  std::string errs;
  bool parsedSuccessfully = reader->parse(reinterpret_cast<const char *>(buffer), reinterpret_cast<const char *>(buffer) + length, &root, &errs);
  
  if (parsedSuccessfully && m_callback)
  {
    CVariant variant = ConvertJsonValueToCVariant(root);
    m_callback->onParsed(&variant);
  }
}

CVariant CJSONVariantParser::Parse(const std::string& json)
{
  return Parse(reinterpret_cast<const unsigned char*>(json.c_str()), json.length());
}

CVariant CJSONVariantParser::Parse(const unsigned char *json, unsigned int length)
{
  CSimpleParseCallback callback;
  CJSONVariantParser parser(&callback);

  parser.push_buffer(json, length);

  return callback.GetOutput();
}

CVariant CJSONVariantParser::ConvertJsonValueToCVariant(const Json::Value &jsonValue)
{
  if (jsonValue.isNull())
    return CVariant::VariantTypeNull;
  else if (jsonValue.isBool())
    return CVariant(jsonValue.asBool());
  else if (jsonValue.isInt())
    return CVariant(jsonValue.asInt());
  else if (jsonValue.isDouble())
    return CVariant(jsonValue.asDouble());
  else if (jsonValue.isString())
    return CVariant(jsonValue.asCString());
  else if (jsonValue.isObject())
  {
    CVariant variant = CVariant::VariantTypeObject;
    Json::Value::Members names = jsonValue.getMemberNames();
    for (Json::Value::Members::const_iterator it = names.begin(); it != names.end(); ++it)
    {
      const std::string &key = *it;
      variant[key.c_str()] = ConvertJsonValueToCVariant(jsonValue[key]);
    }
    return variant;
  }
  else if (jsonValue.isArray())
  {
      CVariant variant = CVariant::VariantTypeArray;
      for (Json::Value::iterator it = jsonValue.begin(); it != jsonValue.end(); ++it)
      {
        const Json::Value &element = *it;
        variant.push_back(ConvertJsonValueToCVariant(element));
      }
      return variant;
  }
  return CVariant();
}
