/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUISliderControl.h
\brief
*/

#include "GUIButtonControl.h"
#include "GUISliderControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUISettingsSliderControl :
      public CGUISliderControl
{
public:
  CGUISettingsSliderControl(int parentID,
                            int controlID,
                            float posX,
                            float posY,
                            float width,
                            float height,
                            float sliderWidth,
                            float sliderHeight,
                            const CTextureInfo& textureFocus,
                            const CTextureInfo& textureNoFocus,
                            const CTextureInfo& backGroundTexture,
                            const CTextureInfo& backGroundTextureDisabled,
                            const CTextureInfo& nibTexture,
                            const CTextureInfo& nibTextureFocus,
                            const CTextureInfo& nibTextureDisabled,
                            const CLabelInfo& labelInfo,
                            int iType);
  virtual ~CGUISettingsSliderControl() {}
  virtual CGUISettingsSliderControl *Clone() const { return new CGUISettingsSliderControl(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void OnUnFocus();
  void SetActive();
  virtual bool IsActive() const { return m_active; }
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetPosition(float posX, float posY);
  virtual float GetWidth() const { return m_buttonControl.GetWidth(); }
  virtual void SetWidth(float width);
  virtual float GetHeight() const { return m_buttonControl.GetHeight(); }
  virtual void SetHeight(float height);
  virtual void SetEnabled(bool bEnable);

  void SetText(const std::string& label) { m_buttonControl.SetLabel(label); }
  virtual float GetXPosition() const { return m_buttonControl.GetXPosition(); }
  virtual float GetYPosition() const { return m_buttonControl.GetYPosition(); }
  virtual std::string GetDescription() const;
  virtual bool HitTest(const CPoint& point) const { return m_buttonControl.HitTest(point); }

protected:
  virtual bool UpdateColors(const CGUIListItem* item);
  virtual void ProcessText();

private:
  CGUISettingsSliderControl(const CGUISettingsSliderControl& control);

  CGUIButtonControl m_buttonControl;
  CGUILabel m_label;
  bool m_active; ///< Whether the slider has been activated by a click.
};

