/*!
\file gui3d.h
\brief
*/

#ifndef GUILIB_GUI3D_H
#define GUILIB_GUI3D_H
#pragma once

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

#define HAS_XBOX_D3D
#define GAMMA_RAMP_FLAG  D3DSGR_IMMEDIATE

#include <xgraphics.h>
#include <d3d8.h>
#include <d3dx8.h>

#define LPD3DXBUFFER XGBuffer*
#define D3DXAssembleShader(str, len, flags, constants, shader, errors) XGAssembleShader("UNKNOWN", str, len, flags, constants, shader, errors, NULL, NULL, NULL, NULL)

// sadly D3DXCreateTexture won't consider linear formats with non power of 2 textures as valid, thus we use standard instead
#define D3DXCreateTexture(device, width, height, levels, usage, format, pool, texture) (device)->CreateTexture(width, height, levels, usage, format, pool, texture)
#endif
