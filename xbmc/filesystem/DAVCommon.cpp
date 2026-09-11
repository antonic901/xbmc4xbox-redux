/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DAVCommon.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <tinyxml/tinyxml.h>

using namespace XFILE;

/*
 * Return true if pElement value is equal value without namespace.
 *
 * if pElement is <DAV:foo> and value is foo then ValueWithoutNamespace is true
 */
bool CDAVCommon::ValueWithoutNamespace(const TiXmlNode* node, const std::string& value)
{
  if (!node)
  {
    return false;
  }

  const TiXmlElement* element = node->ToElement();

  if (!element)
  {
    return false;
  }

  std::vector<std::string> tag = StringUtils::Split(element->Value(), ":", 2);

  if (tag.size() == 1 && tag[0] == value)
  {
    return true;
  }
  else if (tag.size() == 2 && tag[1] == value)
  {
    return true;
  }
  else if (tag.size() > 2)
  {
    CLog::Log(LOGERROR, "Splitting %s failed, size(): % "PRIuS", value: %s", element->Value(), tag.size(),
               value.c_str());
  }

  return false;
}

/*
 * Search for <status> and return its content
 */
std::string CDAVCommon::GetStatusTag(const TiXmlElement* element)
{
  for (const TiXmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
  {
    if (ValueWithoutNamespace(child, "status"))
    {
      return child->NoChildren() ? "" : child->FirstChild()->Value();
    }
  }

  return "";
}
