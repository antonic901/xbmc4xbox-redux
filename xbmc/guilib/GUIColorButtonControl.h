/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file CGUIColorButtonControl.h
\brief
*/
#include "GUIButtonControl.h"
#include "guilib/GUILabel.h"
#include "guilib/guiinfo/GUIInfoColor.h"
#include "utils/ColorUtils.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIColorButtonControl : public CGUIButtonControl
{
public:
  CGUIColorButtonControl(int parentID,
                         int controlID,
                         float posX,
                         float posY,
                         float width,
                         float height,
                         const CTextureInfo& textureFocus,
                         const CTextureInfo& textureNoFocus,
                         const CLabelInfo& labelInfo,
                         const CTextureInfo& colorMask,
                         const CTextureInfo& colorDisabledMask);

  virtual ~CGUIColorButtonControl() {}
  virtual CGUIColorButtonControl* Clone() const { return new CGUIColorButtonControl(*this); }
  CGUIColorButtonControl(const CGUIColorButtonControl& control);

  virtual void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction& action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetPosition(float posX, float posY);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual std::string GetDescription() const;
  void SetColorDimensions(float posX, float posY, float width, float height);
  bool IsSelected() const { return m_bSelected; }
  void SetImageBoxColor(const std::string& hexColor);
  void SetImageBoxColor(KODI::GUILIB::GUIINFO::CGUIInfoColor color);

protected:
  virtual bool UpdateColors(const CGUIListItem* item);
  void ProcessInfoText(unsigned int currentTime);
  void RenderInfoText();
  virtual CGUILabel::COLOR GetTextColor() const;
  boost::movelib::unique_ptr<CGUITexture> m_imgColorMask;
  boost::movelib::unique_ptr<CGUITexture> m_imgColorDisabledMask;
  float m_colorPosX;
  float m_colorPosY;
  KODI::GUILIB::GUIINFO::CGUIInfoColor m_imgBoxColor;
  CGUILabel m_labelInfo;
};
