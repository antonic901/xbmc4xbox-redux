/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUISpinControl.h
\brief
*/

#include "GUIControl.h"
#include "GUILabel.h"
#include "GUITexture.h"

#include <vector>

#define SPIN_CONTROL_TYPE_INT    1
#define SPIN_CONTROL_TYPE_FLOAT  2
#define SPIN_CONTROL_TYPE_TEXT   3
#define SPIN_CONTROL_TYPE_PAGE   4

/*!
 \ingroup controls
 \brief
 */
class CGUISpinControl : public CGUIControl
{
public:
  CGUISpinControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureUp, const CTextureInfo& textureDown, const CTextureInfo& textureUpFocus, const CTextureInfo& textureDownFocus, const CTextureInfo& textureUpDisabled, const CTextureInfo& textureDownDisabled, const CLabelInfo& labelInfo, int iType);
  virtual ~CGUISpinControl() {}
  virtual CGUISpinControl* Clone() const { return new CGUISpinControl(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool HitTest(const CPoint &point) const;
  virtual bool OnMessage(CGUIMessage& message);
#ifdef HAS_XBOX_D3D
  virtual void PreAllocResources();
#endif
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetPosition(float posX, float posY);
  virtual float GetWidth() const;
  void SetRange(int iStart, int iEnd);
  void SetFloatRange(float fStart, float fEnd);
  void SetValue(int iValue);
  void SetValueFromLabel(const std::string &label);
  void SetFloatValue(float fValue);
  void SetStringValue(const std::string& strValue);
  int GetValue() const;
  float GetFloatValue() const;
  std::string GetStringValue() const;
  void AddLabel(const std::string& strLabel, int iValue);
  void AddLabel(const std::string& strLabel, const std::string& strValue);
  const std::string GetLabel() const;
  void SetReverse(bool bOnOff);
  int GetMaximum() const;
  int GetMinimum() const;
  void SetSpinAlign(uint32_t align, float offsetX)
  {
    m_label.GetLabelInfo().align = align;
    m_label.GetLabelInfo().offsetX = offsetX;
  }
  void SetType(int iType) { m_iType = iType; }
  float GetSpinWidth() const { return m_imgspinUp->GetWidth(); }
  float GetSpinHeight() const { return m_imgspinUp->GetHeight(); }
  void SetFloatInterval(float fInterval);
  void SetShowRange(bool bOnoff) ;
  void SetShowOnePage(bool showOnePage) { m_showOnePage = showOnePage; }
  void Clear();
  virtual std::string GetDescription() const;
  bool IsFocusedOnUp() const;

  virtual bool IsVisible() const;

protected:
  CGUISpinControl(const CGUISpinControl& control);

  virtual bool UpdateColors(const CGUIListItem* item);
  /*! \brief Render the spinner text
   \param posX position of the left edge of the text
   \param posY positing of the top edge of the text
   \param width width of the text
   \param height height of the text
   */
  virtual void RenderText(float posX, float posY, float width, float height);
  CGUILabel::COLOR GetTextColor() const;
  void PageUp();
  void PageDown();
  bool CanMoveDown(bool bTestReverse = true);
  bool CanMoveUp(bool bTestReverse = true);
  void MoveUp(bool bTestReverse = true);
  void MoveDown(bool bTestReverse = true);
  void ChangePage(int amount);
  int m_iStart;
  int m_iEnd;
  float m_fStart;
  float m_fEnd;
  int m_iValue;
  float m_fValue;
  int m_iType;
  int m_iSelect;
  bool m_bReverse;
  float m_fInterval;
  std::vector<std::string> m_vecLabels;
  std::vector<int> m_vecValues;
  std::vector<std::string> m_vecStrValues;
  boost::movelib::unique_ptr<CGUITexture> m_imgspinUp;
  boost::movelib::unique_ptr<CGUITexture> m_imgspinDown;
  boost::movelib::unique_ptr<CGUITexture> m_imgspinUpFocus;
  boost::movelib::unique_ptr<CGUITexture> m_imgspinDownFocus;
  boost::movelib::unique_ptr<CGUITexture> m_imgspinUpDisabled;
  boost::movelib::unique_ptr<CGUITexture> m_imgspinDownDisabled;
  CGUILabel   m_label;
  bool m_bShowRange;
  char m_szTyped[10];
  int m_iTypedPos;

  int m_currentItem;
  int m_itemsPerPage;
  int m_numItems;
  bool m_showOnePage;
};

