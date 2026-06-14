/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingsValueSerializer.h"
#include "utils/Variant.h"

#include "system.h" // <xtl.h>
#include <boost/shared_ptr.hpp>

class CSetting;
class CSettingCategory;
class CSettingGroup;
class CSettingSection;

class CSettingsValueFlatJsonSerializer : public ISettingsValueSerializer
{
public:
  explicit CSettingsValueFlatJsonSerializer(bool compact = true);
  ~CSettingsValueFlatJsonSerializer() {}

  void SetCompact(bool compact = true) { m_compact = compact; }

  // implementation of ISettingsValueSerializer
  virtual std::string SerializeValues(const CSettingsManager* settingsManager) const;

private:
  void SerializeSection(CVariant& parent, const boost::shared_ptr<CSettingSection>& section) const;
  void SerializeCategory(CVariant& parent, const boost::shared_ptr<CSettingCategory>& category) const;
  void SerializeGroup(CVariant& parent, const boost::shared_ptr<CSettingGroup>& group) const;
  void SerializeSetting(CVariant& parent, const boost::shared_ptr<CSetting>& setting) const;
  CVariant SerializeSettingValue(const boost::shared_ptr<CSetting>& setting) const;

  bool m_compact;
};
