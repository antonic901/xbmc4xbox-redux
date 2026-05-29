/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DbUrl.h"

class CVariant;

class CVideoDbUrl : public CDbUrl
{
public:
  CVideoDbUrl();
  virtual ~CVideoDbUrl();

  const std::string& GetItemType() const { return m_itemType; }

protected:
  virtual bool parse();
  virtual bool validateOption(const std::string &key, const CVariant &value);

private:
  std::string m_itemType;
};
