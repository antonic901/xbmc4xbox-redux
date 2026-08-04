/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "imagefiles/SpecialImageFileLoader.h"

namespace MUSIC_INFO
{
/*!
 * @brief Generates a texture for an image embedded in a music file.
*/
class CMusicEmbeddedImageFileLoader : public IMAGE_FILES::ISpecialImageFileLoader
{
public:
  CMusicEmbeddedImageFileLoader() {}
  virtual ~CMusicEmbeddedImageFileLoader() {}

  virtual bool CanLoad(const std::string& specialType) const;
  virtual boost::movelib::unique_ptr<CTexture> Load(const std::string& specialType,
                                 const std::string& filePath,
                                 unsigned int preferredWidth,
                                 unsigned int preferredHeight) const;
};
} // namespace MUSIC_INFO
