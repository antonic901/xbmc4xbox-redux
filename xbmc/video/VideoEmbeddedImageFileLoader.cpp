/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoEmbeddedImageFileLoader.h"

#include "FileItem.h"
#include "guilib/Texture.h"
#include "utils/EmbeddedArt.h"
#include "utils/StringUtils.h"
#include "video/VideoInfoTag.h"
#include "video/tags/IVideoInfoTagLoader.h"
#include "video/tags/VideoInfoTagLoaderFactory.h"

using namespace VIDEO;

bool CVideoEmbeddedImageFileLoader::CanLoad(const std::string& specialType) const
{
  return StringUtils::StartsWith(specialType, "video_");
}

namespace
{
bool GetEmbeddedThumb(const std::string& path, const std::string& type, EmbeddedArt& art)
{
  CFileItem item(path, false);
  boost::movelib::unique_ptr<IVideoInfoTagLoader> loader(
      CVideoInfoTagLoaderFactory::CreateLoader(item, ADDON::ScraperPtr(), false));
  CVideoInfoTag tag;
  std::vector<EmbeddedArt> artv;
  if (loader)
    loader->Load(tag, false, &artv);

  for (std::vector<EmbeddedArt>::const_iterator it = artv.begin(); it != artv.end(); ++it)
  {
    if (it->m_type == type)
    {
      art = *it;
      break;
    }
  }
  return !art.Empty();
}
} // namespace

boost::movelib::unique_ptr<CTexture> CVideoEmbeddedImageFileLoader::Load(const std::string& specialType,
                                                              const std::string& filePath,
                                                              unsigned int preferredWidth,
                                                              unsigned int preferredHeight) const
{
  EmbeddedArt art;
  if (GetEmbeddedThumb(filePath, specialType.substr(6), art))
    return CTexture::LoadFromFileInMemory(&art.m_data[0], art.m_size, art.m_mime, preferredWidth,
                                          preferredHeight);
  return boost::movelib::unique_ptr<CTexture>();
}
