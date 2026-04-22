/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingPath.h"

#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <boost/make_shared.hpp>

#define XML_ELM_DEFAULT     "default"
#define XML_ELM_CONSTRAINTS "constraints"

CSettingPath::CSettingPath(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : CSettingString(id, settingsManager), m_writable(true), m_hideExtension(false)
{ }

CSettingPath::CSettingPath(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager /* = NULL */)
  : CSettingString(id, label, value, settingsManager), m_writable(true), m_hideExtension(false)
{ }

CSettingPath::CSettingPath(const std::string &id, const CSettingPath &setting)
  : CSettingString(id, setting)
{
  copy(setting);
}

SettingPtr CSettingPath::Clone(const std::string &id) const
{
  return boost::make_shared<CSettingPath>(id, *this);
}

bool CSettingPath::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  CExclusiveLock lock(m_critical);

  if (!CSettingString::Deserialize(node, update))
    return false;

  if (m_control != NULL &&
     (m_control->GetType() != "button" || (m_control->GetFormat() != "path" && m_control->GetFormat() != "file" && m_control->GetFormat() != "image")))
  {
    CLog::Log(LOGERROR, "CSettingPath: invalid <control> of \"{}\"", m_id);
    return false;
  }

  const TiXmlNode *constraints = node->FirstChild(XML_ELM_CONSTRAINTS);
  if (constraints != NULL)
  {
    // get writable
    XMLUtils::GetBoolean(constraints, "writable", m_writable);
    // get hide extensions
    XMLUtils::GetBoolean(constraints, "hideextensions", m_hideExtension);

    // get sources
    const TiXmlNode *sources = constraints->FirstChild("sources");
    if (sources != NULL)
    {
      m_sources.clear();
      const TiXmlNode *source = sources->FirstChild("source");
      while (source != NULL)
      {
        const TiXmlNode *child = source->FirstChild();
        if (child != NULL)
        {
          const std::string& strSource = child->ValueStr();
          if (!strSource.empty())
            m_sources.push_back(strSource);
        }

        source = source->NextSibling("source");
      }
    }

    // get masking
    const TiXmlNode *masking = constraints->FirstChild("masking");
    if (masking != NULL)
      m_masking = masking->FirstChild()->ValueStr();
  }

  return true;
}

bool CSettingPath::SetValue(const std::string &value)
{
  // for backwards compatibility to Frodo
  if (StringUtils::EqualsNoCase(value, "select folder") ||
      StringUtils::EqualsNoCase(value, "select writable folder"))
    return CSettingString::SetValue("");

  return CSettingString::SetValue(value);
}

std::string CSettingPath::GetMasking(const CFileExtensionProvider& fileExtensionProvider) const
{
  if (m_masking.empty())
    return m_masking;

  // setup masking
  std::string audioMask = fileExtensionProvider.GetMusicExtensions();
  std::string videoMask = fileExtensionProvider.GetVideoExtensions();
  std::string imageMask = fileExtensionProvider.GetPictureExtensions();
  const char *execMask = "";
#if defined(TARGET_WINDOWS)
  execMask = ".exe|.bat|.cmd|.py";
#elif defined(_XBOX)
  execMask = ".xbe|.py";
#endif // defined(TARGET_WINDOWS)

  std::string masking = m_masking;
  if (masking == "video")
    return videoMask;
  if (masking == "audio")
    return audioMask;
  if (masking == "image")
    return imageMask;
  if (masking == "executable")
    return execMask;

  // convert mask qualifiers
  StringUtils::Replace(masking, "$AUDIO", audioMask);
  StringUtils::Replace(masking, "$VIDEO", videoMask);
  StringUtils::Replace(masking, "$IMAGE", imageMask);
  StringUtils::Replace(masking, "$EXECUTABLE", execMask);

  return masking;
}

void CSettingPath::copy(const CSettingPath& setting)
{
  CSettingString::Copy(setting);

  CExclusiveLock lock(m_critical);
  m_writable = setting.m_writable;
  m_sources = setting.m_sources;
  m_hideExtension = setting.m_hideExtension;
  m_masking = setting.m_masking;
}
