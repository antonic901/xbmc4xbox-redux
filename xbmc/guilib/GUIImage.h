/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file guiImage.h
\brief
*/

#include "GUIControl.h"
#include "GUITexture.h"
#include "guilib/guiinfo/GUIInfoLabel.h"

#include <vector>

/*!
 \ingroup controls
 \brief
 */

class CGUIImage : public CGUIControl
{
public:
  class CFadingTexture
  {
  public:
    CFadingTexture(const CGUITexture* texture, unsigned int fadeTime)
    {
      // create a copy of our texture, and allocate resources
      m_texture.reset(texture->Clone());
      m_texture->AllocResources();
      m_fadeTime = fadeTime;
      m_fading = false;
    };
    ~CFadingTexture()
    {
      m_texture->FreeResources();
    };

    boost::movelib::unique_ptr<CGUITexture> m_texture; ///< texture to fade out
    unsigned int m_fadeTime; ///< time to fade out (ms)
    bool         m_fading;   ///< whether we're fading out

  private:
    CFadingTexture(const CFadingTexture&);
    CFadingTexture& operator=(const CFadingTexture&);
  };

  CGUIImage(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture);
  CGUIImage(const CGUIImage &left);
  virtual ~CGUIImage(void);
  virtual CGUIImage* Clone() const { return new CGUIImage(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
#ifdef HAS_XBOX_D3D
  virtual void PreAllocResources();
#endif
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; }
  virtual void SetInvalid();
  virtual bool CanFocus() const;
  virtual void UpdateInfo(const CGUIListItem *item = NULL);

  virtual void SetInfo(const KODI::GUILIB::GUIINFO::CGUIInfoLabel &info);
  virtual void SetFileName(const std::string& strFileName, bool setConstant = false, const bool useCache = true);
  virtual void SetAspectRatio(const CAspectRatio &aspect);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetPosition(float posX, float posY);
  virtual std::string GetDescription() const;
  void SetCrossFade(unsigned int time);

  const std::string& GetFileName() const;
  float GetTextureWidth() const;
  float GetTextureHeight() const;

  virtual CRect CalcRenderRegion() const;

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  virtual void AllocateOnDemand();
  virtual void FreeTextures(bool immediately = false);
  void FreeResourcesButNotAnims();
  unsigned char GetFadeLevel(unsigned int time) const;
  bool ProcessFading(CFadingTexture *texture, unsigned int frameTime, unsigned int currentTime);

  /*!
   * \brief Update the diffuse color based on the current item infos
   * \param item the item to for info resolution
  */
  void UpdateDiffuseColor(const CGUIListItem* item);

  bool m_bDynamicResourceAlloc;

  // border + conditional info
  CTextureInfo m_image;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_info;

  boost::movelib::unique_ptr<CGUITexture> m_texture;
  std::vector<CFadingTexture *> m_fadingTextures;
  std::string m_currentTexture;
  std::string m_currentFallback;

  unsigned int m_crossFadeTime;
  unsigned int m_currentFadeTime;
  unsigned int m_lastRenderTime;
};

