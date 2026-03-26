/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file
\brief
*/

#include "GUIControl.h"
#include "GUITexture.h"

/*!
 \ingroup controls
 \brief
 */
class GUIScrollBarControl :
      public CGUIControl
{
public:
  GUIScrollBarControl(int parentID, int controlID, float posX, float posY,
                       float width, float height,
                       const CTextureInfo& backGroundTexture,
                       const CTextureInfo& barTexture, const CTextureInfo& barTextureFocus,
                       const CTextureInfo& nibTexture, const CTextureInfo& nibTextureFocus,
                       ORIENTATION orientation, bool showOnePage);
  virtual ~GUIScrollBarControl() {}
  virtual GUIScrollBarControl* Clone() const { return new GUIScrollBarControl(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetRange(int pageSize, int numItems);
  virtual bool OnMessage(CGUIMessage& message);
  void SetValue(int value);
  int GetValue() const;
  virtual std::string GetDescription() const;
  virtual bool IsVisible() const;
protected:
  virtual bool UpdateColors(const CGUIListItem* item);
  bool UpdateBarSize();
  bool Move(int iNumSteps);
  virtual void SetFromPosition(const CPoint &point);

  boost::movelib::unique_ptr<CGUITexture> m_guiBackground;
  boost::movelib::unique_ptr<CGUITexture> m_guiBarNoFocus;
  boost::movelib::unique_ptr<CGUITexture> m_guiBarFocus;
  boost::movelib::unique_ptr<CGUITexture> m_guiNibNoFocus;
  boost::movelib::unique_ptr<CGUITexture> m_guiNibFocus;

  int m_numItems;
  int m_pageSize;
  int m_offset;

  bool m_showOnePage;
  ORIENTATION m_orientation;

private:
  GUIScrollBarControl(const GUIScrollBarControl& control);
};

