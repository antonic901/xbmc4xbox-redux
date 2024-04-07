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

#include "Picture.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "filesystem/CurlFile.h"
#include "DllImageLib.h"
#include "utils/JpegIO.h"
#include "utils/Crc32.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "guilib/GraphicContext.h"
#include "TextureManager.h"
#include "XBTF.h"

using namespace XFILE;

bool CPicture::CreateThumbnail(const CStdString& file, const CStdString& thumbFile, bool checkExistence /*= false*/)
{
  // don't create the thumb if it already exists
  if (checkExistence && CFile::Exists(thumbFile))
    return true;

  return CacheImage(file, thumbFile, g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize);
}

bool CPicture::CacheImage(const CStdString& sourceUrl, const CStdString& destFile, int width, int height)
{
  if (width > 0 && height > 0)
  {
    CLog::Log(LOGINFO, "Caching image from: %s to %s with width %i and height %i", sourceUrl.c_str(), destFile.c_str(), width, height);

    CJpegIO jpegImage;
    DllImageLib dll;
    bool ret;

    CStdString tempFile(sourceUrl);
    bool isTemp(false);
    if (URIUtils::IsInternetStream(sourceUrl, true))
    {
      Crc32 crc;
      crc.ComputeFromLowerCase(sourceUrl);
      tempFile.Format("special://temp/%08x%s", (unsigned __int32)crc, URIUtils::GetExtension(sourceUrl).c_str());
      CCurlFile stream;
      if (!stream.Download(sourceUrl, tempFile))
        return false;
      isTemp = true;
    }

    ret = false;
    if (URIUtils::GetExtension(sourceUrl).Equals(".jpg") || URIUtils::GetExtension(sourceUrl).Equals(".tbn"))
      ret = jpegImage.CreateThumbnail(tempFile, destFile, width, height);

    if (!ret)
    {
      if (!dll.Load()) return false;
      if (!dll.CreateThumbnail(tempFile.c_str(), destFile.c_str(), width, height, CSettings::Get().GetBool("pictures.useexifrotation")))
      {
        CLog::Log(LOGERROR, "%s Unable to create new image %s from image %s", __FUNCTION__, destFile.c_str(), sourceUrl.c_str());
        return false;
      }
    }
    if (isTemp)
      CFile::Delete(tempFile);
  }
  else
  {
    CLog::Log(LOGINFO, "Caching image from: %s to %s", sourceUrl.c_str(), destFile.c_str());
    if (URIUtils::IsInternetStream(sourceUrl, true))
    {
      CCurlFile stream;
      return stream.Download(sourceUrl, destFile);
    }
    else
      return CFile::Copy(sourceUrl, destFile);
  }
  return true;
}

bool CPicture::CacheThumb(const CStdString& sourceUrl, const CStdString& destFile)
{
  return CacheImage(sourceUrl, destFile, g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize);
}

bool CPicture::CacheFanart(const CStdString& sourceUrl, const CStdString& destFile)
{
  int height = g_advancedSettings.m_fanartHeight;
  // Assume 16:9 size
  int width = height * 16 / 9;

  return CacheImage(sourceUrl, destFile, width, height);
}

bool CPicture::CreateThumbnailFromMemory(const unsigned char* buffer, int bufSize, const CStdString& extension, const CStdString& thumbFile)
{
  CLog::Log(LOGINFO, "Creating album thumb from memory: %s", thumbFile.c_str());
  if (extension.Equals("jpg") || extension.Equals("tbn"))
  {
    CJpegIO jpegImage;
    if (jpegImage.CreateThumbnailFromMemory((unsigned char*)buffer, bufSize, thumbFile.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
      return true;
  }
  DllImageLib dll;
  if (!dll.Load()) return false;
  if (!dll.CreateThumbnailFromMemory((BYTE *)buffer, bufSize, extension.c_str(), thumbFile.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
  {
    CLog::Log(LOGERROR, "%s: exception with fileType: %s", __FUNCTION__, extension.c_str());
    return false;
  }
  return true;
}

void CPicture::CreateFolderThumb(const CStdString *thumbs, const CStdString &folderThumb)
{ // we want to mold the thumbs together into one single one
  DllImageLib dll;
  if (!dll.Load()) return;

  const char *szThumbs[4];
  for (int i=0; i < 4; i++)
    szThumbs[i] = thumbs[i].c_str();

  if (!dll.CreateFolderThumbnail(szThumbs, folderThumb.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
  {
    CLog::Log(LOGERROR, "%s failed for folder thumb %s", __FUNCTION__, folderThumb.c_str());
  }
}

bool CPicture::CreateThumbnailFromSurface(const unsigned char *buffer, int width, int height, int stride, const CStdString &thumbFile)
{
  if (URIUtils::GetExtension(thumbFile).Equals(".jpg"))
  {
    CJpegIO jpegImage;
    if (jpegImage.CreateThumbnailFromSurface((BYTE *)buffer, width, height, XB_FMT_A8R8G8B8, stride, thumbFile.c_str()))
      return true;
  }
  DllImageLib dll;
  if (!buffer || !dll.Load()) return false;
  return dll.CreateThumbnailFromSurface((BYTE *)buffer, width, height, stride, thumbFile.c_str());
}

int CPicture::ConvertFile(const CStdString &srcFile, const CStdString &destFile, float rotateDegrees, int width, int height, unsigned int quality, bool mirror)
{
  DllImageLib dll;
  if (!dll.Load()) return false;
  int ret = dll.ConvertFile(srcFile.c_str(), destFile.c_str(), rotateDegrees, width, height, quality, mirror);
  if (ret)
  {
    CLog::Log(LOGERROR, "PICTURE: Error %i converting image %s", ret, srcFile.c_str());
    return ret;
  }
  return ret;
}

// caches a skin image as a thumbnail image
bool CPicture::CacheSkinImage(const CStdString &srcFile, const CStdString &destFile)
{
  int iImages = g_TextureManager.Load(srcFile);
  if (iImages > 0)
  {
    CTextureArray texture = g_TextureManager.GetTexture(srcFile);
    if (texture.size())
    {
      bool success(false);
      if (!texture.m_texCoordsArePixels)
      { // damn, have to copy it to a linear texture first :(
        return CreateThumbnailFromSwizzledTexture(texture.m_textures[0], texture.m_width, texture.m_height, destFile);
      }
      else
      {
        D3DLOCKED_RECT lr;
        texture.m_textures[0]->LockRect(0, &lr, NULL, 0);
        success = CreateThumbnailFromSurface((BYTE *)lr.pBits, texture.m_width, texture.m_height, lr.Pitch, destFile);
        texture.m_textures[0]->UnlockRect(0);
      }
      g_TextureManager.ReleaseTexture(srcFile);
      return success;
    }
  }
  return false;
}

bool CPicture::CreateThumbnailFromSwizzledTexture(LPDIRECT3DTEXTURE8 &texture, int width, int height, const CStdString &thumbFile)
{
  LPDIRECT3DTEXTURE8 linTexture = NULL;
  if (D3D_OK == D3DXCreateTexture(g_graphicsContext.Get3DDevice(), width, height, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &linTexture))
  {
    LPDIRECT3DSURFACE8 source;
    LPDIRECT3DSURFACE8 dest;
    texture->GetSurfaceLevel(0, &source);
    linTexture->GetSurfaceLevel(0, &dest);
    D3DXLoadSurfaceFromSurface(dest, NULL, NULL, source, NULL, NULL, D3DX_FILTER_NONE, 0);
    D3DLOCKED_RECT lr;
    dest->LockRect(&lr, NULL, 0);
    bool success = CreateThumbnailFromSurface((BYTE *)lr.pBits, width, height, lr.Pitch, thumbFile);
    dest->UnlockRect();
    SAFE_RELEASE(source);
    SAFE_RELEASE(dest);
    SAFE_RELEASE(linTexture);
    return success;
  }
  return false;
}

