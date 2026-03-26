/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/info/InfoBool.h"
#include "utils/log.h" // for ExpressionFlattener

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

// forward definitions
class TiXmlElement;
namespace INFO
{
  class CSkinVariableString;
}

class CGUIIncludes
{
public:
  CGUIIncludes();
  ~CGUIIncludes();

  /*!
   \brief Clear all include components (defaults, constants, variables, expressions and includes)
  */
  void Clear();

  /*!
   \brief Load all include components(defaults, constants, variables, expressions and includes)
   from the main entrypoint \code{file}. Flattens nested expressions and expressions in variable
   conditions after loading all other included files.

   \param file the file to load
  */
  void Load(const std::string &file);

  /*!
   \brief Resolve all include components (defaults, constants, variables, expressions and includes)
   for the given \code{node}. Place the conditions specified for <include> elements in \code{includeConditions}.

   \param node the node from where we start to resolve the include components
   \param includeConditions a map that holds the conditions for resolved includes
   */
  void Resolve(TiXmlElement *node, std::map<INFO::InfoPtr, bool>* includeConditions = NULL);

  /*!
   \brief Create a skin variable for the given \code{name} within the given \code{context}.

   \param name the name of the skin variable
   \param context the context where the variable is created in
   \return skin variable
   */
  const INFO::CSkinVariableString* CreateSkinVariable(const std::string& name, int context);

private:
  enum ResolveParamsResult
  {
    NO_PARAMS_FOUND,
    PARAMS_RESOLVED,
    SINGLE_UNDEFINED_PARAM_RESOLVED
  };

  /*!
   \brief Load all include components (defaults, constants, variables, expressions and includes)
   from the given \code{file}.

   \param file the file to load
   \return true if the file was loaded otherwise false
  */
  bool Load_Internal(const std::string &file);

  bool HasLoaded(const std::string &file) const;

  void LoadDefaults(const TiXmlElement *node);
  void LoadIncludes(const TiXmlElement *node);
  void LoadVariables(const TiXmlElement *node);
  void LoadConstants(const TiXmlElement *node);
  void LoadExpressions(const TiXmlElement *node);

  /*!
   \brief Resolve all expressions containing other expressions to a single evaluatable expression.
  */
  void FlattenExpressions();

  /*!
   \brief Expand any expressions nested in this expression.

   \param expression the expression to flatten
   \param resolved list of already evaluated expression names, to avoid expanding circular references
  */
  void FlattenExpression(std::string &expression, const std::vector<std::string> &resolved);

  /*!
   \brief Resolve all variable conditions containing expressions to a single evaluatable condition.
  */
  void FlattenSkinVariableConditions();

  void SetDefaults(TiXmlElement *node);
  void ResolveIncludes(TiXmlElement *node, std::map<INFO::InfoPtr, bool>* xmlIncludeConditions = NULL);
  void ResolveConstants(TiXmlElement *node);
  void ResolveExpressions(TiXmlElement *node);

  typedef std::map<std::string, std::string> Params;
  static void InsertNested(TiXmlElement* controls, TiXmlElement* include, TiXmlElement* node);
  static bool GetParameters(const TiXmlElement *include, const char *valueAttribute, Params& params);
  static void ResolveParametersForNode(TiXmlElement *node, const Params& params);
  static ResolveParamsResult ResolveParameters(const std::string& strInput, std::string& strOutput, const Params& params);

  std::string ResolveConstant(const std::string &constant) const;
  std::string ResolveExpressions(const std::string &expression) const;

  std::vector<std::string> m_files;
  std::map<std::string, std::pair<TiXmlElement, Params> > m_includes;
  std::map<std::string, TiXmlElement> m_defaults;
  std::map<std::string, TiXmlElement> m_skinvariables;
  std::map<std::string, std::string> m_constants;
  std::map<std::string, std::string> m_expressions;

  std::set<std::string> m_constantAttributes;
  std::set<std::string> m_constantNodes;

  std::set<std::string> m_expressionAttributes;
  std::set<std::string> m_expressionNodes;

  // because C++98 doesn't support lambda functions we need this class to pass
  // our replacer function to CGUIInfoLabel::ReplaceSpecialKeywordReferences
  class ExpressionReplacer
  {
  public:
    ExpressionReplacer(const std::map<std::string, std::string>& expressions)
      : m_expressions(expressions) {}

    std::string operator()(const std::string &str) const
    {
      std::map<std::string, std::string>::const_iterator it = m_expressions.find(str);
      if (it != m_expressions.end())
        return it->second;
      return "";
    }

  private:
    const std::map<std::string, std::string>& m_expressions;
  };

  class ExpressionFlattener
  {
  public:
    ExpressionFlattener(CGUIIncludes* includes,
                        const std::string& original,
                        const std::vector<std::string>& resolved)
      : m_includes(includes), m_original(original), m_resolved(resolved)
    {}

    std::string operator()(const std::string& expressionName) const
    {
      if (std::find(m_resolved.begin(), m_resolved.end(), expressionName) != m_resolved.end())
      {
        CLog::Log(LOGERROR, "Skin has a circular expression \"%s\": %s", m_resolved.back().c_str(), m_original.c_str());
        return std::string();
      }

      std::map<std::string, std::string>::iterator it = m_includes->m_expressions.find(expressionName);
      if (it == m_includes->m_expressions.end())
        return std::string();

      std::vector<std::string> rescopy = m_resolved;
      rescopy.push_back(expressionName);
      m_includes->FlattenExpression(it->second, rescopy);

      return it->second;
    }

  private:
    CGUIIncludes* m_includes;
    std::string m_original;
    std::vector<std::string> m_resolved;
  };
};
