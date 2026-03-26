/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIMoverControl.h
\brief
*/

#include "GUIControl.h"
#include "GUITexture.h"

#define ALLOWED_DIRECTIONS_ALL   0
#define ALLOWED_DIRECTIONS_UPDOWN  1
#define ALLOWED_DIRECTIONS_LEFTRIGHT 2

#define DIRECTION_NONE 0
#define DIRECTION_UP 1
#define DIRECTION_DOWN 2
#define DIRECTION_LEFT 3
#define DIRECTION_RIGHT 4

/*!
 \ingroup controls
 \brief
 */
class CGUIMoverControl : public CGUIControl
{
public:
  CGUIMoverControl(int parentID,
                   int controlID,
                   float posX,
                   float posY,
                   float width,
                   float height,
                   const CTextureInfo& textureFocus,
                   const CTextureInfo& textureNoFocus);

  virtual ~CGUIMoverControl(void) {}
  virtual CGUIMoverControl* Clone() const { return new CGUIMoverControl(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void OnUp();
  virtual void OnDown();
  virtual void OnLeft();
  virtual void OnRight();
#ifdef HAS_XBOX_D3D
  virtual void PreAllocResources();
#endif
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetPosition(float posX, float posY);
  void SetLimits(int iX1, int iY1, int iX2, int iY2);
  void SetLocation(int iLocX, int iLocY, bool bSetPosition = true);
  int GetXLocation() const { return m_iLocationX; }
  int GetYLocation() const { return m_iLocationY; }
  virtual bool CanFocus() const { return true; }

protected:
  virtual bool UpdateColors(const CGUIListItem* item);
  bool SetAlpha(unsigned char alpha);
  void UpdateSpeed(int nDirection);
  void Move(int iX, int iY);
  boost::movelib::unique_ptr<CGUITexture> m_imgFocus;
  boost::movelib::unique_ptr<CGUITexture> m_imgNoFocus;
  unsigned int m_frameCounter;
  unsigned int m_lastMoveTime;
  int m_nDirection;
  float m_fSpeed;
  float m_fAnalogSpeed;
  float m_fMaxSpeed;
  float m_fAcceleration;
  int m_iX1, m_iX2, m_iY1, m_iY2;
  int m_iLocationX, m_iLocationY;

private:
  CGUIMoverControl(const CGUIMoverControl& control);
};

