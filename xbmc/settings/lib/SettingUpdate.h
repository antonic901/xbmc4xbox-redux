/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class TiXmlNode;

namespace SettingUpdateType
{
  enum Type {
    Unknown = 0,
    Rename,
    Change
  };
}

class CSettingUpdate
{
public:
  CSettingUpdate();
  virtual ~CSettingUpdate() {}

  inline bool operator<(const CSettingUpdate& rhs) const
  {
    return m_type < rhs.m_type && m_value < rhs.m_value;
  }

  virtual bool Deserialize(const TiXmlNode *node);

  SettingUpdateType::Type GetType() const { return m_type; }
  const std::string& GetValue() const { return m_value; }

private:
  bool setType(const std::string &type);

  SettingUpdateType::Type m_type;
  std::string m_value;
};
