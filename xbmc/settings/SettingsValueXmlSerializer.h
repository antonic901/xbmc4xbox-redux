/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingsValueSerializer.h"

#include "system.h" // <xtl.h>
#include <boost/shared_ptr.hpp>

class CSetting;
class CSettingCategory;
class CSettingGroup;
class CSettingSection;
class TiXmlNode;

class CSettingsValueXmlSerializer : public ISettingsValueSerializer
{
public:
  CSettingsValueXmlSerializer() {};
  ~CSettingsValueXmlSerializer() {};

  // implementation of ISettingsValueSerializer
  virtual std::string SerializeValues(const CSettingsManager* settingsManager) const;

private:
  void SerializeSection(TiXmlNode* parent, const boost::shared_ptr<CSettingSection>& section) const;
  void SerializeCategory(TiXmlNode* parent,
                         const boost::shared_ptr<CSettingCategory>& category) const;
  void SerializeGroup(TiXmlNode* parent, const boost::shared_ptr<CSettingGroup>& group) const;
  void SerializeSetting(TiXmlNode* parent, const boost::shared_ptr<CSetting>& setting) const;
};
