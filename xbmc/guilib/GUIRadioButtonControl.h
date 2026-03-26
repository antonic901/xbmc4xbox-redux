/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIRadioButtonControl.h
\brief
*/

#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIRadioButtonControl :
      public CGUIButtonControl
{
public:
  CGUIRadioButtonControl(int parentID, int controlID,
                         float posX, float posY, float width, float height,
                         const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus,
                         const CLabelInfo& labelInfo,
                         const CTextureInfo& radioOnFocus, const CTextureInfo& radioOnNoFocus,
                         const CTextureInfo& radioOffFocus, const CTextureInfo& radioOffNoFocus,
                         const CTextureInfo& radioOnDisabled, const CTextureInfo& radioOffDisabled);

  virtual ~CGUIRadioButtonControl() {}
  virtual CGUIRadioButtonControl* Clone() const { return new CGUIRadioButtonControl(*this); }

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
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual std::string GetDescription() const;
  void SetRadioDimensions(float posX, float posY, float width, float height);
  void SetToggleSelect(const std::string &toggleSelect);
  bool IsSelected() const { return m_bSelected; }

protected:
  virtual bool UpdateColors(const CGUIListItem* item);
  boost::movelib::unique_ptr<CGUITexture> m_imgRadioOnFocus;
  boost::movelib::unique_ptr<CGUITexture> m_imgRadioOnNoFocus;
  boost::movelib::unique_ptr<CGUITexture> m_imgRadioOffFocus;
  boost::movelib::unique_ptr<CGUITexture> m_imgRadioOffNoFocus;
  boost::movelib::unique_ptr<CGUITexture> m_imgRadioOnDisabled;
  boost::movelib::unique_ptr<CGUITexture> m_imgRadioOffDisabled;
  float m_radioPosX;
  float m_radioPosY;
  INFO::InfoPtr m_toggleSelect;
  bool m_useLabel2;

private:
  CGUIRadioButtonControl(const CGUIRadioButtonControl& control);
};
