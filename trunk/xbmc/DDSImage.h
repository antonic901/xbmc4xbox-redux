/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#pragma once

#include <string>
#include <stdint.h>

class CDDSImage
{
public:
  CDDSImage();
  CDDSImage(unsigned int width, unsigned int height, unsigned int format);
  ~CDDSImage();

  unsigned int GetOrgWidth() const;
  unsigned int GetOrgHeight() const;
  unsigned int GetWidth() const;
  unsigned int GetHeight() const;
  unsigned int GetFormat() const;
  unsigned int GetSize() const;
  unsigned char *GetData() const;

  bool ReadFile(const std::string &file);

private:
  void Allocate(unsigned int width, unsigned int height, unsigned int format);
  const char *GetFourCC(unsigned int format) const;
  bool WriteFile(const std::string &file) const;
  unsigned int GetStorageRequirements(unsigned int width, unsigned int height, unsigned int format) const;
  enum {
    ddsd_caps        = 0x00000001,
    ddsd_height      = 0x00000002,
    ddsd_width       = 0x00000004,
    ddsd_pitch       = 0x00000008,
    ddsd_pixelformat = 0x00001000,
    ddsd_mipmapcount = 0x00020000,
    ddsd_linearsize  = 0x00080000,
    ddsd_depth       = 0x00800000
  };

  enum {
    ddpf_alphapixels = 0x00000001,
    ddpf_fourcc      = 0x00000004,
    ddpf_rgb         = 0x00000040
  };

  enum {
    ddscaps_complex = 0x00000008,
    ddscaps_texture = 0x00001000,
    ddscaps_mipmap  = 0x00400000
  };

  #pragma pack(push, 2)
  typedef struct
  {
    uint32_t size;
    uint32_t flags;
    uint32_t fourcc;
    uint32_t rgbBitCount;
    uint32_t rBitMask;
    uint32_t gBitMask;
    uint32_t bBitMask;
    uint32_t aBitMask;
  } ddpixelformat;

#define DDPF_ALPHAPIXELS 0x00000001
#define DDPF_ALPHA       0x00000002
#define DDPF_FOURCC      0x00000004
#define DDPF_RGB         0x00000040
#define DDPF_YUV         0x00000200
#define DDPF_LUMINANCE   0x00020000

//XBMC Magic Mark 0x434D4258 = 'XBMC'
#define DD_XBMC_MAGIC    0x434D4258

  typedef struct
  {
    uint32_t flags1;
    uint32_t flags2;
    uint32_t reserved[2];
  } ddcaps2;

  typedef struct
  {
    uint32_t      size;
    uint32_t      flags;
    uint32_t      height;
    uint32_t      width;
    uint32_t      linearSize;
    uint32_t      depth;
    uint32_t      mipmapcount;
    /*
      DDS FILE FORMAT MODIFICATION FOR PRE-PADDED TEXTURES

      Original reserved area: 
      uint32_t      reserved[11];
      
      This modification uses the last 3 dwords of space in the 1st reserved area to store
      the actual size of the image data (as opposed to the texture size). The DDS file
      format was not designed to be used pre-padded - this modification was necessary
      in order to support it. Having the textures pre-padded on disk saves quite a bit of
      work, since otherwise we would have to manually pad the texture to POT procedurally
      and then clamp the edges off - this allows us to avoid that. See GetOrgWidth() and
      GetOrgHeight() in DDSImage.cpp
    */
    uint32_t      reserved[8];
    uint32_t      xbmcMagic;
    uint32_t      orgWidth;
    uint32_t      orgHeight;
    /*
    END DDS FILE FORMAT MODIFICATION
    */
    ddpixelformat pixelFormat;
    ddcaps2       caps;
    uint32_t      reserved2;
  } ddsurfacedesc2;
  #pragma pack(pop)

  ddsurfacedesc2 m_desc;
  unsigned char *m_data;
};
