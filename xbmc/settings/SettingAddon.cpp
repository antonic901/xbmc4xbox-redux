/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingAddon.h"

#include "addons/Addon.h"
#include "settings/lib/SettingsManager.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

CSettingAddon::CSettingAddon(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : CSettingString(id, settingsManager), m_addonType(ADDON::ADDON_UNKNOWN)
{ }

CSettingAddon::CSettingAddon(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager /* = NULL */)
  : CSettingString(id, label, value, settingsManager), m_addonType(ADDON::ADDON_UNKNOWN)
{ }

CSettingAddon::CSettingAddon(const std::string &id, const CSettingAddon &setting)
  : CSettingString(id, setting)
{
  copyaddontype(setting);
}

SettingPtr CSettingAddon::Clone(const std::string &id) const
{
  return boost::make_shared<CSettingAddon>(id, *this);
}

bool CSettingAddon::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (!CSettingString::Deserialize(node, update))
    return false;

  if (m_control != NULL &&
     (m_control->GetType() != "button" || m_control->GetFormat() != "addon"))
  {
    CLog::Log(LOGERROR, "CSettingAddon: invalid <control> of \"{}\"", m_id);
    return false;
  }

  bool ok = false;
  std::string strAddonType;
  const TiXmlNode *constraints = node->FirstChild("constraints");
  if (constraints != NULL)
  {
    // get the addon type
    if (XMLUtils::GetString(constraints, "addontype", strAddonType) && !strAddonType.empty())
    {
      m_addonType = ADDON::TranslateType(strAddonType);
      if (m_addonType != ADDON::ADDON_UNKNOWN)
        ok = true;
    }
  }

  if (!ok && !update)
  {
    CLog::Log(LOGERROR, "CSettingAddon: error reading the addontype value \"{}\" of \"{}\"",
              strAddonType, m_id);
    return false;
  }

  return true;
}

void CSettingAddon::copyaddontype(const CSettingAddon &setting)
{
  CSettingString::Copy(setting);

  CExclusiveLock lock(m_critical);
  m_addonType = setting.m_addonType;
}
