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

#include "utils/StdString.h"

class CPicture
{
public:
  static bool CreateThumbnailFromMemory(const unsigned char* buffer, int bufSize, const CStdString& extension, const CStdString& thumbFile);
  static bool CreateThumbnailFromSurface(const unsigned char* buffer, int width, int height, int stride, const CStdString &thumbFile);
  static int ConvertFile(const CStdString& srcFile, const CStdString& destFile, float rotateDegrees, int width, int height, unsigned int quality, bool mirror=false);

  static void CreateFolderThumb(const CStdString *strThumbs, const CStdString &folderThumbnail);
  static bool CreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName, bool checkExistence = false);
  static bool CacheThumb(const CStdString& sourceUrl, const CStdString& destFileName);
  static bool CacheFanart(const CStdString& sourceUrl, const CStdString& destFileName);

  // caches a skin image as a thumbnail image
  static bool CacheSkinImage(const CStdString &srcFile, const CStdString &destFile);
  static bool CreateThumbnailFromSwizzledTexture(LPDIRECT3DTEXTURE8 &texture, int width, int height, const CStdString &thumbFile);

private:
  static bool CacheImage(const CStdString& sourceUrl, const CStdString& destFileName, int width, int height);

  struct VERTEX
  {
    D3DXVECTOR4 p;
    D3DCOLOR col;
    FLOAT tu, tv;
  };
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
};
