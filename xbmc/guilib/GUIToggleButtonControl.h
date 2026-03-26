/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIToggleButtonControl.h
\brief
*/

#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIToggleButtonControl : public CGUIButtonControl
{
public:
  CGUIToggleButtonControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus, const CTextureInfo& altTextureFocus, const CTextureInfo& altTextureNoFocus, const CLabelInfo &labelInfo, bool wrapMultiline = false);
  virtual ~CGUIToggleButtonControl(void);
  virtual CGUIToggleButtonControl* Clone() const { return new CGUIToggleButtonControl(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetPosition(float posX, float posY);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetMinWidth(float minWidth);
  virtual void SetLabel(const std::string& label);
  void SetAltLabel(const std::string& label);
  virtual std::string GetDescription() const;
  void SetToggleSelect(const std::string &toggleSelect);
  void SetAltClickActions(const CGUIAction &clickActions);

protected:
  virtual bool UpdateColors(const CGUIListItem* item);
  virtual void OnClick();
  CGUIButtonControl m_selectButton;
  INFO::InfoPtr m_toggleSelect;

private:
  void ProcessToggle(unsigned int currentTime);
  std::string m_altLabel;
};

