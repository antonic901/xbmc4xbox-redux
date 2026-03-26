/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIButtonControl.h
\brief
*/

#include "GUIAction.h"
#include "GUIControl.h"
#include "GUILabel.h"
#include "GUITexture.h"
#include "guilib/guiinfo/GUIInfoLabel.h"
#include "utils/ColorUtils.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIButtonControl : public CGUIControl
{
public:
  CGUIButtonControl(int parentID, int controlID,
                    float posX, float posY, float width, float height,
                    const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus,
                    const CLabelInfo &label, bool wrapMultiline = false);

  CGUIButtonControl(const CGUIButtonControl& control);

  virtual ~CGUIButtonControl() {}
  virtual CGUIButtonControl* Clone() const { return new CGUIButtonControl(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
#ifdef HAS_XBOX_D3D
  virtual void PreAllocResources();
#endif
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetPosition(float posX, float posY);
  virtual void SetLabel(const std::string & aLabel);
  virtual void SetLabel2(const std::string & aLabel2);
  void SetClickActions(const CGUIAction& clickActions) { m_clickActions = clickActions; }
  const CGUIAction& GetClickActions() const { return m_clickActions; }
  void SetFocusActions(const CGUIAction& focusActions) { m_focusActions = focusActions; }
  void SetUnFocusActions(const CGUIAction& unfocusActions) { m_unfocusActions = unfocusActions; }
  const CLabelInfo& GetLabelInfo() const { return m_label.GetLabelInfo(); }
  virtual std::string GetLabel() const { return GetDescription(); }
  virtual std::string GetLabel2() const;
  void SetSelected(bool bSelected);
  virtual std::string GetDescription() const;
  virtual float GetWidth() const;
  virtual void SetMinWidth(float minWidth);
  void SetAlpha(unsigned char alpha);

  void PythonSetLabel(const std::string& strFont,
                      const std::string& strText,
                      UTILS::COLOR::Color textColor,
                      UTILS::COLOR::Color shadowColor,
                      UTILS::COLOR::Color focusedColor);
  void PythonSetDisabledColor(UTILS::COLOR::Color disabledColor);

  virtual void OnClick();
  bool HasClickActions() const { return m_clickActions.HasActionsMeetingCondition(); }

  virtual bool UpdateColors(const CGUIListItem* item);

  virtual CRect CalcRenderRegion() const;

protected:
  friend class CGUISpinControlEx;
  virtual void OnFocus();
  virtual void OnUnFocus();
  virtual void ProcessText(unsigned int currentTime);
  virtual void RenderText();
  virtual CGUILabel::COLOR GetTextColor() const;

  /*!
   * \brief Set the maximum width for the left label
   */
  void SetMaxWidth(float labelMaxWidth) { m_labelMaxWidth = labelMaxWidth; }

  boost::movelib::unique_ptr<CGUITexture> m_imgFocus;
  boost::movelib::unique_ptr<CGUITexture> m_imgNoFocus;
  unsigned int  m_focusCounter;
  unsigned char m_alpha;

  float m_minWidth;
  float m_maxWidth;
  float m_labelMaxWidth;

  KODI::GUILIB::GUIINFO::CGUIInfoLabel  m_info;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel  m_info2;
  CGUILabel      m_label;
  CGUILabel      m_label2;

  CGUIAction m_clickActions;
  CGUIAction m_focusActions;
  CGUIAction m_unfocusActions;

  bool m_bSelected;
};

