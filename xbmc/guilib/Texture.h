/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/*!
\file Texture.h
\brief
*/

#ifndef GUILIB_TEXTURE_H
#define GUILIB_TEXTURE_H

#include "utils/StdString.h"
#include "XBTF.h"

#pragma once

struct ImageInfo;

/*!
\ingroup textures
\brief Base texture class, subclasses of which depend on the render spec (DX, GL etc.)
This class is not real backport from Kodi/XBMC. This class is abstraction of current way how textures are loaded by XBMC4Xbox
and it's DirectX dependant. This class export same methods just like the one from Kodi. For example CBaseTexture::LoadFromFile
before was known as CPicture::Load.
*/
class CBaseTexture
{

public:
  CBaseTexture(unsigned int width = 0, unsigned int height = 0, unsigned int format = XB_FMT_A8R8G8B8,
               IDirect3DTexture8* texture = NULL, IDirect3DPalette8* palette = NULL, bool packed = false);
  virtual ~CBaseTexture();

  /*! \brief Load a texture from a file
   Loads a texture from a file, restricting in size if needed based on maxHeight and maxWidth.
   Note that these are the ideal size to load at - the returned texture may be smaller or larger than these.
   \param texturePath the path of the texture to load.
   \param idealWidth the ideal width of the texture (defaults to 0, no ideal width).
   \param idealHeight the ideal height of the texture (defaults to 0, no ideal height).
   \param autoRotate whether the textures should be autorotated based on EXIF information (defaults to false).
   \return a CBaseTexture pointer to the created texture - NULL if the texture failed to load.
   */
  static CBaseTexture *LoadFromFile(const CStdString& texturePath, unsigned int idealWidth = 0, unsigned int idealHeight = 0,
                                    bool autoRotate = false);

  /*! \brief Load a texture from a file in memory
   Loads a texture from a file in memory, restricting in size if needed based on maxHeight and maxWidth.
   Note that these are the ideal size to load at - the returned texture may be smaller or larger than these.
   \param buffer the memory buffer holding the file.
   \param bufferSize the size of buffer.
   \param mimeType the mime type of the file in buffer.
   \param idealWidth the ideal width of the texture (defaults to 0, no ideal width).
   \param idealHeight the ideal height of the texture (defaults to 0, no ideal height).
   \return a CBaseTexture pointer to the created texture - NULL if the texture failed to load.
   */
  static CBaseTexture *LoadFromFileInMemory(unsigned char* buffer, size_t bufferSize, const std::string& mimeType,
                                            unsigned int idealWidth = 0, unsigned int idealHeight = 0);                                  

  bool LoadPaletted(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, IDirect3DPalette8 *palette);

  bool HasAlpha() const;

  IDirect3DTexture8* GetTextureObject() const { return m_texture; }
  IDirect3DPalette8* GetPaletteObject() const { return m_palette; }
  void SetPaletteObject(IDirect3DPalette8* palette) { m_palette = palette; }

  unsigned char* GetPixels() const { return m_pixels; }
  unsigned int GetPitch() const { return m_pitch; }
  unsigned int GetRows() const { return GetRows(m_textureHeight); }
  unsigned int GetTextureWidth() const { return m_textureWidth; }
  unsigned int GetTextureHeight() const { return m_textureHeight; }
  unsigned int GetWidth() const { return m_imageWidth; }
  unsigned int GetHeight() const { return m_imageHeight; }
  /*! \brief return the original width of the image, before scaling/cropping */
  unsigned int GetOriginalWidth() const { return m_originalWidth; }
  /*! \brief return the original height of the image, before scaling/cropping */
  unsigned int GetOriginalHeight() const { return m_originalHeight; }

  bool GetTexCoordsArePixels() const { return m_texCoordsArePixels; }
  int GetOrientation() const { return m_orientation; }
  void SetOrientation(int orientation) { m_orientation = orientation; }

  void Allocate(unsigned int width, unsigned int height, unsigned int format);
  // populates some general info about loaded texture (width, height, pitch etc.)
  bool GetTextureInfo();

  static unsigned int PadPow2(unsigned int x);

private:
  // no copy constructor
  CBaseTexture(const CBaseTexture &copy);

protected:
  bool LoadFromFileInMem(unsigned char* buffer, size_t size, const std::string& mimeType,
                         unsigned int maxWidth, unsigned int maxHeight);
  bool LoadFromFileInternal(const CStdString& texturePath, unsigned int maxWidth, unsigned int maxHeight, bool autoRotate);
  void LoadFromImage(ImageInfo &image, bool autoRotate = false);
  // helpers for computation of texture parameters for compressed textures
  unsigned int GetRows(unsigned int height) const;

  unsigned int m_imageWidth;
  unsigned int m_imageHeight;
  unsigned int m_textureWidth;
  unsigned int m_textureHeight;
  unsigned int m_originalWidth;   ///< original image width before scaling or cropping
  unsigned int m_originalHeight;  ///< original image height before scaling or cropping

  IDirect3DTexture8* m_texture;
  /* NOTICE for future:
    Note that in SDL and Win32 we already convert the paletted textures into normal textures,
    so there's no chance of having m_palette as a real palette */
  IDirect3DPalette8* m_palette;

  // this variable should hold Data which represents loaded Texture
  unsigned char* m_pixels;
  unsigned int m_format;
  unsigned int m_pitch;
  int m_orientation;
  bool m_hasAlpha;
  // What is this? On Kodi it's always false
  bool m_texCoordsArePixels;
  // true if texture is loaded from .XPR
  bool m_packed;
};

#define CTexture CBaseTexture

#endif
