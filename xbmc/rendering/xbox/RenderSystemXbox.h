/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "rendering/RenderSystem.h"

#include <xtl.h>
#include <xgraphics.h>
#include <d3d8.h>
#include <d3dx8.h>

class CRenderSystemXbox : public CRenderSystemBase
{
public:
  CRenderSystemXbox();
  virtual ~CRenderSystemXbox();
  virtual bool InitRenderSystem();
  virtual bool DestroyRenderSystem();

  virtual bool BeginRender();
  virtual bool EndRender();
  virtual void PresentRender(bool rendered, bool videoLayer);

protected:
  virtual void PresentRenderImpl(bool rendered) = 0;

  D3DPRESENT_PARAMETERS m_d3dpp;
  LPDIRECT3D8 m_pD3D;
  LPDIRECT3DDEVICE8 m_pd3dDevice;
  LPDIRECT3DSURFACE8 m_pBackBuffer;
};
