/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "JSONVariantWriter.h"

std::string CJSONVariantWriter::Write(const CVariant &value, bool compact)
{
  std::string output;

  Json::Value root;
  if(InternalWrite(root, value))
  {
    Json::StreamWriterBuilder writerBuilder;
    writerBuilder.settings_["indentation"] = compact ? "" : "\t";

    output = Json::writeString(writerBuilder, root);
  }

  return output;
}

bool CJSONVariantWriter::InternalWrite(Json::Value &jsonValue, const CVariant &value)
{
  switch (value.type())
  {
    case CVariant::VariantTypeInteger:
      jsonValue = Json::Value(value.asInteger());
      break;
    case CVariant::VariantTypeUnsignedInteger:
      jsonValue = Json::Value(static_cast<unsigned int>(value.asUnsignedInteger()));
      break;
    case CVariant::VariantTypeDouble:
      jsonValue = Json::Value(value.asDouble());
      break;
    case CVariant::VariantTypeBoolean:
      jsonValue = Json::Value(value.asBoolean());
      break;
    case CVariant::VariantTypeString:
      jsonValue = Json::Value(value.asString());
      break;
    case CVariant::VariantTypeArray:
      for (CVariant::const_iterator_array it = value.begin_array(); it != value.end_array(); ++it)
      {
        Json::Value subJsonValue;
        if(!InternalWrite(subJsonValue, *it))
          return false;
        jsonValue.append(subJsonValue);
      }
      break;
    case CVariant::VariantTypeObject:
      for (CVariant::const_iterator_map it = value.begin_map(); it != value.end_map(); ++it)
      {
        Json::Value subJsonValue;
        if(!InternalWrite(subJsonValue, it->second))
          return false;
        jsonValue[it->first] = subJsonValue;
      }
      break;
    case CVariant::VariantTypeConstNull:
    case CVariant::VariantTypeNull:
    default:
      jsonValue = Json::Value(Json::nullValue);
      break;
  }
  return true;
}
