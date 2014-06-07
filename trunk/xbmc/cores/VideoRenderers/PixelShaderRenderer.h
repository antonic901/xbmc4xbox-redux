#ifndef PIXELSHADER_RENDERER
#define PIXELSHADER_RENDERER

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "XBoxRenderer.h"

class CPixelShaderRenderer : public CXBoxRenderer
{
public:
  CPixelShaderRenderer(LPDIRECT3DDEVICE8 pDevice);
  //~CPixelShaderRenderer();

  // Functions called from mplayer
  // virtual void     WaitForFlip();
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);
  virtual bool IsConfigured() { return m_bConfigured; } 

protected:
  virtual void Render(DWORD flags);
};

#endif
