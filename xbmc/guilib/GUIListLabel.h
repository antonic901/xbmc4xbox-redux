/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIListLabel.h
\brief
*/

#include "GUIControl.h"
#include "GUILabel.h"
#include "guilib/guiinfo/GUIInfoLabel.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIListLabel :
      public CGUIControl
{
public:
  CGUIListLabel(int parentID, int controlID, float posX, float posY, float width, float height,
                const CLabelInfo& labelInfo, const KODI::GUILIB::GUIINFO::CGUIInfoLabel &label, CGUIControl::GUISCROLLVALUE scroll);
  virtual ~CGUIListLabel(void);
  virtual CGUIListLabel* Clone() const { return new CGUIListLabel(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool CanFocus() const { return false; }
  virtual void UpdateInfo(const CGUIListItem *item = NULL);
  virtual void SetFocus(bool focus);
  virtual void SetInvalid();
  virtual void SetWidth(float width);

  void SetLabel(const std::string &label);
  void SetSelected(bool selected);

  static void CheckAndCorrectOverlap(CGUIListLabel &label1, CGUIListLabel &label2)
  {
    CGUILabel::CheckAndCorrectOverlap(label1.m_label, label2.m_label);
  }

  virtual CRect CalcRenderRegion() const;

protected:
  virtual bool UpdateColors(const CGUIListItem* item);

  CGUILabel     m_label;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_info;
  CGUIControl::GUISCROLLVALUE m_scroll;
};
