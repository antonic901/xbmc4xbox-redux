/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "InfoBool.h"

#include <list>
#include <stack>
#include <utility>
#include <vector>

class CGUIListItem;

namespace INFO
{
/*! \brief Class to wrap active boolean conditions
 */
class InfoSingle : public InfoBool
{
public:
  InfoSingle(const std::string& expression, int context, const unsigned int& refreshCounter)
    : InfoBool(expression, context, refreshCounter)
  {
  }
  virtual void Initialize(CGUIInfoManager* infoMgr);

  virtual void Update(int contextWindow, const CGUIListItem* item);

private:
  int m_condition;             ///< actual condition this represents
};

/*! \brief Class to wrap active boolean expressions
 */
class InfoExpression : public InfoBool
{
public:
  InfoExpression(const std::string& expression, int context, const unsigned int& refreshCounter)
    : InfoBool(expression, context, refreshCounter)
  {
  }
  virtual ~InfoExpression() {}

  virtual void Initialize(CGUIInfoManager* infoMgr);

  virtual void Update(int contextWindow, const CGUIListItem* item);

private:
  typedef enum
  {
    OPERATOR_NONE  = 0,
    OPERATOR_LB,  // 1
    OPERATOR_RB,  // 2
    OPERATOR_OR,  // 3
    OPERATOR_AND, // 4
    OPERATOR_NOT, // 5
  } operator_t;

  typedef enum
  {
    NODE_LEAF,
    NODE_AND,
    NODE_OR,
  } node_type_t;

  // An abstract base class for nodes in the expression tree
  class InfoSubexpression
  {
  public:
    virtual ~InfoSubexpression(void) {}; // so we can destruct derived classes using a pointer to their base class
    virtual bool Evaluate(int contextWindow, const CGUIListItem* item) = 0;
    virtual node_type_t Type() const=0;
  };

  typedef boost::shared_ptr<InfoSubexpression> InfoSubexpressionPtr;

  // A leaf node in the expression tree
  class InfoLeaf : public InfoSubexpression
  {
  public:
    InfoLeaf(InfoPtr info, bool invert) : m_info(boost::move(info)), m_invert(invert) {}
    virtual bool Evaluate(int contextWindow, const CGUIListItem* item);
    virtual node_type_t Type() const { return NODE_LEAF; }

  private:
    InfoPtr m_info;
    bool m_invert;
  };

  // A branch node in the expression tree
  class InfoAssociativeGroup : public InfoSubexpression
  {
  public:
    InfoAssociativeGroup(node_type_t type, const InfoSubexpressionPtr &left, const InfoSubexpressionPtr &right);
    void AddChild(const InfoSubexpressionPtr &child);
    void Merge(const boost::shared_ptr<InfoAssociativeGroup>& other);
    virtual bool Evaluate(int contextWindow, const CGUIListItem* item);
    virtual node_type_t Type() const { return m_type; }

  private:
    node_type_t m_type;
    std::list<InfoSubexpressionPtr> m_children;
  };

  static operator_t GetOperator(char ch);
  static void OperatorPop(std::stack<operator_t> &operator_stack, bool &invert, std::stack<InfoSubexpressionPtr> &nodes);
  bool Parse(const std::string &expression);
  InfoSubexpressionPtr m_expression_tree;
};

};
