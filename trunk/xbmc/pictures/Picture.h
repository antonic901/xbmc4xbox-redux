#pragma once
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

#include "system.h"
#include "pictures/DllImageLib.h"
#include "gui3d.h"

class CPicture
{
public:
  CPicture(void);
  virtual ~CPicture(void);
  IDirect3DTexture8* Load(const CStdString& strFilename, int width = 128, int height = 128);

  static bool CreateThumbnailFromMemory(const unsigned char* buffer, int bufSize, const CStdString& extension, const CStdString& thumbFile);
  static bool CreateThumbnailFromSurface(const unsigned char* buffer, int width, int height, int stride, const CStdString &thumbFile);
  static bool CreateThumbnailFromSwizzledTexture(LPDIRECT3DTEXTURE8 &texture, int width, int height, const CStdString &thumbFile);
  static int ConvertFile(const CStdString& srcFile, const CStdString& destFile, float rotateDegrees, int width, int height, unsigned int quality, bool mirror=false);

  static void CreateFolderThumb(const CStdString *strThumbs, const CStdString &folderThumbnail);
  static bool CreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName, bool checkExistence = false);
  static bool CacheThumb(const CStdString& sourceUrl, const CStdString& destFileName);
  static bool CacheFanart(const CStdString& sourceUrl, const CStdString& destFileName);

  // caches a skin image as a thumbnail image
  bool CacheSkinImage(const CStdString &srcFile, const CStdString &destFile);

  ImageInfo GetInfo() const { return m_info; };
  unsigned int GetWidth() const { return m_info.width; };
  unsigned int GetHeight() const { return m_info.height; };
  unsigned int GetOriginalWidth() const { return m_info.originalwidth; };
  unsigned int GetOriginalHeight() const { return m_info.originalheight; };
  const EXIFINFO *GetExifInfo() const { return &m_info.exifInfo; };

protected:
  
private:
  static bool CacheImage(const CStdString& sourceUrl, const CStdString& destFileName, int width, int height);

  struct VERTEX
  {
    D3DXVECTOR4 p;
    D3DCOLOR col;
    FLOAT tu, tv;
  };
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

  ImageInfo m_info;
};
