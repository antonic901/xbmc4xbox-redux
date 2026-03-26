/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIControl.h"
#include "GUIImage.h"
#include "TextureManager.h"

class CGUIBorderedImage : public CGUIImage
{
public:
  CGUIBorderedImage(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture, const CTextureInfo& borderTexture, const CRect &borderSize);
  virtual ~CGUIBorderedImage(void) {}
  virtual CGUIBorderedImage* Clone() const { return new CGUIBorderedImage(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
#ifdef HAS_XBOX_D3D
  virtual void PreAllocResources();
#endif
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);

  virtual CRect CalcRenderRegion() const;

protected:
  boost::movelib::unique_ptr<CGUITexture> m_borderImage;
  CRect m_borderSize;

private:
  CGUIBorderedImage(const CGUIBorderedImage& right);
};

