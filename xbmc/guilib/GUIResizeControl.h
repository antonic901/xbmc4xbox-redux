/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIRESIZEControl.h
\brief
*/

#include "GUIControl.h"
#include "GUITexture.h"
#include "utils/MovingSpeed.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIResizeControl : public CGUIControl
{
public:
  CGUIResizeControl(int parentID,
                    int controlID,
                    float posX,
                    float posY,
                    float width,
                    float height,
                    const CTextureInfo& textureFocus,
                    const CTextureInfo& textureNoFocus,
                    UTILS::MOVING_SPEED::MapEventConfig& movingSpeedCfg);

  virtual ~CGUIResizeControl() {}
  virtual CGUIResizeControl* Clone() const { return new CGUIResizeControl(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void OnUp();
  virtual void OnDown();
  virtual void OnLeft();
  virtual void OnRight();
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetPosition(float posX, float posY);
  void SetLimits(float x1, float y1, float x2, float y2);
  virtual bool CanFocus() const { return true; }

protected:
  virtual bool UpdateColors(const CGUIListItem* item);
  bool SetAlpha(unsigned char alpha);
  void Resize(float x, float y);
  boost::movelib::unique_ptr<CGUITexture> m_imgFocus;
  boost::movelib::unique_ptr<CGUITexture> m_imgNoFocus;
  unsigned int m_frameCounter;
  UTILS::MOVING_SPEED::CMovingSpeed m_movingSpeed;
  float m_fAnalogSpeed;
  float m_x1, m_x2, m_y1, m_y2;

private:
  CGUIResizeControl(const CGUIResizeControl& control);
};

