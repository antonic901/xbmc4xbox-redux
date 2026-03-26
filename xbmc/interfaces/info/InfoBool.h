/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system.h" // <xtl.h>
#include "boost/shared_ptr.hpp"
#include "boost/move/move.hpp"
#include <string>

class CGUIListItem;
class CGUIInfoManager;

namespace INFO
{
/*!
 \ingroup info
 \brief Base class, wrapping boolean conditions and expressions
 */
class InfoBool
{
public:
  InfoBool(const std::string &expression, int context, const unsigned int &refreshCounter);
  virtual ~InfoBool() {};

  virtual void Initialize(CGUIInfoManager* infoMgr) { m_infoMgr = infoMgr; }

  /*! \brief Get the value of this info bool
   This is called to update (if dirty) and fetch the value of the info bool
   \param contextWindow the context (window id) where this condition is being evaluated
   \param item the item used to evaluate the bool
   */
  inline bool Get(int contextWindow, const CGUIListItem* item = NULL)
  {
    if (item && m_listItemDependent)
      Update(contextWindow, item);
    else if (m_refreshCounter != m_parentRefreshCounter || m_refreshCounter == 0)
    {
      Update(contextWindow, nullptr);
      m_refreshCounter = m_parentRefreshCounter;
    }
    return m_value;
  }

  bool operator==(const InfoBool &right) const
  {
    return (m_context == right.m_context &&
            m_expression == right.m_expression);
  }

  bool operator<(const InfoBool &right) const
  {
    if (m_context < right.m_context)
      return true;
    else if (m_context == right.m_context)
      return m_expression < right.m_expression;
    else
      return false;
  }

  /*! \brief Update the value of this info bool
   This is called if and only if the info bool is dirty, allowing it to update it's current value
   */
  virtual void Update(int contextWindow, const CGUIListItem* item) {}

  const std::string &GetExpression() const { return m_expression; }
  bool ListItemDependent() const { return m_listItemDependent; }
protected:
  bool m_value; ///< current value
  int m_context;               ///< contextual information to go with the condition
  bool m_listItemDependent; ///< do not cache if a listitem pointer is given
  std::string  m_expression;   ///< original expression
  CGUIInfoManager* m_infoMgr;

private:
  unsigned int m_refreshCounter;
  const unsigned int &m_parentRefreshCounter;
};

typedef boost::shared_ptr<InfoBool> InfoPtr;
};
