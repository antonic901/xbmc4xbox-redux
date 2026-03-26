/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIProgressControl.h
\brief
*/

#include "GUIControl.h"
#include "GUITexture.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIProgressControl :
      public CGUIControl
{
public:
  CGUIProgressControl(int parentID, int controlID, float posX, float posY,
                      float width, float height, const CTextureInfo& backGroundTexture,
                      const CTextureInfo& leftTexture, const CTextureInfo& midTexture,
                      const CTextureInfo& rightTexture, const CTextureInfo& overlayTexture,
                      bool reveal=false);
  virtual ~CGUIProgressControl() {}
  virtual CGUIProgressControl* Clone() const { return new CGUIProgressControl(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool CanFocus() const;
#ifdef HAS_XBOX_D3D
  virtual void PreAllocResources();
#endif
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetPosition(float posX, float posY);
  void SetPercentage(float fPercent);
  void SetInfo(int iInfo, int iInfo2 = 0);
  int GetInfo() const { return m_iInfoCode; }

  float GetPercentage() const;
  virtual std::string GetDescription() const;
  virtual void UpdateInfo(const CGUIListItem *item = NULL);
  bool UpdateLayout(void);
protected:
  virtual bool UpdateColors(const CGUIListItem* item);
  boost::movelib::unique_ptr<CGUITexture> m_guiBackground;
  boost::movelib::unique_ptr<CGUITexture> m_guiLeft;
  boost::movelib::unique_ptr<CGUITexture> m_guiMid;
  boost::movelib::unique_ptr<CGUITexture> m_guiRight;
  boost::movelib::unique_ptr<CGUITexture> m_guiOverlay;
  CRect m_guiMidClipRect;

  int m_iInfoCode;
  int m_iInfoCode2;
  float m_fPercent;
  float m_fPercent2;
  bool m_bReveal;
  bool m_bChanged;

private:
  CGUIProgressControl(const CGUIProgressControl& control);
};

