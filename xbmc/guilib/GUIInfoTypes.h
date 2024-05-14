/*!
\file GUIInfoTypes.h
\brief
*/

#ifndef GUILIB_GUIINFOTYPES_H
#define GUILIB_GUIINFOTYPES_H

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/StdString.h"

class CGUIListItem;

class CGUIInfoBool
{
public:
  CGUIInfoBool(bool value = false);
  virtual ~CGUIInfoBool();

  operator bool() const { return m_value; };

  void Update(const CGUIListItem *item = NULL);
  void Parse(const CStdString &expression, int context);
private:
  unsigned int m_info;
  bool m_value;
};

typedef uint32_t color_t;

class CGUIInfoColor
{
public:
  CGUIInfoColor(color_t color = 0);

  const CGUIInfoColor &operator=(const CGUIInfoColor &color);
  const CGUIInfoColor &operator=(color_t color);
  operator color_t() const { return m_color; };

  void Update();
  void Parse(const CStdString &label, int context);

private:
  color_t GetColor() const;
  int      m_info;
  color_t m_color;
};

typedef CStdString (*StringReplacerFunc) (const CStdString &str);

class CGUIInfoLabel
{
public:
  CGUIInfoLabel();
  CGUIInfoLabel(const CStdString &label, const CStdString &fallback = "", int context = 0);

  void SetLabel(const CStdString &label, const CStdString &fallback, int context = 0);
  CStdString GetLabel(int contextWindow, bool preferImage = false, CStdString *fallback = NULL) const;

  /*!
   \brief Gets the label and returns it as an int value
   \param contextWindow the context in which to evaluate the expression.
   \return int value.
   \sa GetLabel
   */
  int GetIntValue(int contextWindow) const;

  CStdString GetItemLabel(const CGUIListItem *item, bool preferImage = false, CStdString *fallback = NULL) const;
  bool IsConstant() const;
  bool IsEmpty() const;

  const CStdString GetFallback() const { return m_fallback; };

  static CStdString GetLabel(const CStdString &label, int contextWindow = 0, bool preferImage = false);

  /*!
   \brief Replaces instances of $LOCALIZE[number] with the appropriate localized string
   \param label text to replace
   \return text with any localized strings filled in.
   */
  static CStdString ReplaceLocalize(const CStdString &label);
  static CStdString ReplaceAddonStrings(const CStdString &label);

  /*!
   \brief Replaces instances of $strKeyword[value] with the appropriate resolved string
   \param strInput text to replace
   \param strKeyword keyword to look for
   \param func function that does the actual replacement of each bracketed value found
   \param strOutput the output string
   \return whether anything has been replaced.
   */
  static bool ReplaceSpecialKeywordReferences(const CStdString &strInput, const CStdString &strKeyword, const StringReplacerFunc func, CStdString &strOutput);

  ///*!
  // \brief Replaces instances of $strKeyword[value] with the appropriate resolved string in-place
  // \param work text to replace in-place
  // \param strKeyword keyword to look for
  // \param func function that does the actual replacement of each bracketed value found
  // \return whether anything has been replaced.
  // */
  static bool ReplaceSpecialKeywordReferences(CStdString &work, const CStdString &strKeyword, const StringReplacerFunc func);

private:
  void Parse(const CStdString &label, int context);

  class CInfoPortion
  {
  public:
    CInfoPortion(int info, const CStdString &prefix, const CStdString &postfix, bool escaped = false);
    CStdString GetLabel(const CStdString &info) const;
    int m_info;
    CStdString m_prefix;
    CStdString m_postfix;
  private:
    bool m_escaped;
  };

  CStdString m_fallback;
  std::vector<CInfoPortion> m_info;
};

#endif
