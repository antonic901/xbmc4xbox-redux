/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/ColorUtils.h"
#include "utils/Geometry.h"

#include <boost/move/unique_ptr.hpp>
#include <string>

/*
 *   CRenderSystemBase interface allows us to create the rendering engine we use.
 *   We currently have two engines: OpenGL and DirectX
 *   This interface is very basic since a lot of the actual details will go in to the derived classes
 */

class CGUIImage;
class CGUITextLayout;

class CRenderSystemBase
{
public:
  CRenderSystemBase();
  virtual ~CRenderSystemBase();

  virtual bool InitRenderSystem() = 0;
  virtual bool DestroyRenderSystem() = 0;

  virtual bool BeginRender() = 0;
  virtual bool EndRender() = 0;
  virtual void PresentRender(bool rendered, bool videoLayer) = 0;

  virtual void ShowSplash(const std::string& message);

protected:
  bool                m_bRenderCreated;
  unsigned int        m_maxTextureSize;

  boost::movelib::unique_ptr<CGUIImage> m_splashImage;
  boost::movelib::unique_ptr<CGUITextLayout> m_splashMessageLayout;
};

