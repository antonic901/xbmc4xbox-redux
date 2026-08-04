/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SpecialImageLoaderFactory.h"

#include "guilib/Texture.h"
#include "music/MusicEmbeddedImageFileLoader.h"
#include "pictures/PictureFolderImageFileLoader.h"
#include "video/VideoChapterImageFileLoader.h"
#include "video/VideoEmbeddedImageFileLoader.h"
#include "video/VideoGeneratedImageFileLoader.h"

using namespace IMAGE_FILES;

CSpecialImageLoaderFactory::CSpecialImageLoaderFactory()
{
  m_specialImageLoaders[0] = boost::movelib::unique_ptr<ISpecialImageFileLoader>(new VIDEO::CVideoEmbeddedImageFileLoader);
  m_specialImageLoaders[1] = boost::movelib::unique_ptr<ISpecialImageFileLoader>(new MUSIC_INFO::CMusicEmbeddedImageFileLoader);
  m_specialImageLoaders[2] = boost::movelib::unique_ptr<ISpecialImageFileLoader>(new VIDEO::CVideoGeneratedImageFileLoader);
  m_specialImageLoaders[3] = boost::movelib::unique_ptr<ISpecialImageFileLoader>(new CPictureFolderImageFileLoader);
  m_specialImageLoaders[4] = boost::movelib::unique_ptr<ISpecialImageFileLoader>(new VIDEO::CVideoChapterImageFileLoader);
}

boost::movelib::unique_ptr<CTexture> CSpecialImageLoaderFactory::Load(const std::string& specialType,
                                                           const std::string& filePath,
                                                           unsigned int preferredWidth,
                                                           unsigned int preferredHeight) const
{
  if (specialType.empty())
    return boost::movelib::unique_ptr<CTexture>();
  for (boost::array<boost::movelib::unique_ptr<ISpecialImageFileLoader>, 5>::const_iterator loader = m_specialImageLoaders.begin(); loader != m_specialImageLoaders.end(); ++loader)
  {
    if ((*loader)->CanLoad(specialType))
    {
      boost::movelib::unique_ptr<CTexture> val = (*loader)->Load(specialType, filePath, preferredWidth, preferredHeight);
      if (val)
        return boost::move(val);
    }
  }
  return boost::movelib::unique_ptr<CTexture>();
}
