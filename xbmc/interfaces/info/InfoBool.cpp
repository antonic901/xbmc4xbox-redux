/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoBool.h"

#include "utils/StringUtils.h"

namespace INFO
{
InfoBool::InfoBool(const std::string& expression, int context, const unsigned int& refreshCounter)
  : m_context(context), m_expression(expression), m_parentRefreshCounter(refreshCounter), m_value(false), m_listItemDependent(false), m_refreshCounter(0)
{
  StringUtils::ToLower(m_expression);
}
}
