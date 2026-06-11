/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonExtensions.h"

#include "utils/StringUtils.h"

using namespace ADDON;

bool SExtValue::asBoolean() const
{
  return StringUtils::EqualsNoCase(str, "true");
}

const SExtValue CAddonExtensions::GetValue(const std::string& id) const
{
  for (EXT_VALUES::const_iterator values = m_values.begin(); values != m_values.end(); ++values)
  {
    for (CExtValues::const_iterator value = values->second.begin(); value != values->second.end(); ++value)
    {
      if (value->first == id)
        return value->second;
    }
  }
  return SExtValue("");
}

const EXT_VALUES& CAddonExtensions::GetValues() const
{
  return m_values;
}

const CAddonExtensions* CAddonExtensions::GetElement(const std::string& id) const
{
  for (EXT_ELEMENTS::const_iterator child = m_children.begin(); child != m_children.end(); ++child)
  {
    if (child->first == id)
      return &child->second;
  }

  return NULL;
}

const EXT_ELEMENTS CAddonExtensions::GetElements(const std::string& id) const
{
  if (id.empty())
    return m_children;

  EXT_ELEMENTS children;
  for (EXT_ELEMENTS::const_iterator child = m_children.begin(); child != m_children.end(); ++child)
  {
    if (child->first == id)
      children.push_back(std::make_pair(child->first, child->second));
  }
  return children;
}

void CAddonExtensions::Insert(const std::string& id, const std::string& value)
{
  EXT_VALUE extension;
  extension.push_back(std::make_pair(id, SExtValue(value)));
  m_values.push_back(std::make_pair(id, extension));
}
