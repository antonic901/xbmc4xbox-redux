/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ISettingControl.h"

#include "ServiceBroker.h"
#include "SettingDefinitions.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

ISettingControl::ISettingControl()
{
  m_delayed = false;
}

bool ISettingControl::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  const char *strTmp = elem->Attribute(SETTING_XML_ATTR_FORMAT);
  std::string format;
  if (strTmp != NULL)
    format = strTmp;
  if (!SetFormat(format))
  {
    CLog::Log(LOGERROR, "error reading \"{}\" attribute of <control>", SETTING_XML_ATTR_FORMAT);
    return false;
  }

  if ((strTmp = elem->Attribute(SETTING_XML_ATTR_DELAYED)) != NULL)
  {
    if (!StringUtils::EqualsNoCase(strTmp, "false") && !StringUtils::EqualsNoCase(strTmp, "true"))
    {
      CLog::Log(LOGERROR, "error reading \"{}\" attribute of <control>", SETTING_XML_ATTR_DELAYED);
      return false;
    }
    else
      m_delayed = StringUtils::EqualsNoCase(strTmp, "true");
  }

  return true;
}
