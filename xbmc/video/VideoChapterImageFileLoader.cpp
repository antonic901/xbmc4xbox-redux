/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoChapterImageFileLoader.h"

#include "DVDFileInfo.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "guilib/Texture.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

bool VIDEO::CVideoChapterImageFileLoader::CanLoad(const std::string& specialType) const
{
  return specialType == "videochapter";
}

boost::movelib::unique_ptr<CTexture> VIDEO::CVideoChapterImageFileLoader::Load(
    const std::string& specialType,
    const std::string& goofyChapterPath,
    unsigned int,
    unsigned int) const
{
  // "goofy" chapter path because these paths don't yet conform to 'image://' path standard

  // 10 = length of "chapter://" string prefix from GUIDialogVideoBookmarks
  size_t lastSlashPos = goofyChapterPath.rfind('/');
  std::string cleanname = goofyChapterPath.substr(10, lastSlashPos - 10);

  int chapterNum = 0;
  try
  {
    chapterNum = atoi(goofyChapterPath.substr(lastSlashPos + 1).c_str());
  }
  catch (...)
  {
    // invalid_argument because these paths can come from anywhere
    // out_of_range mostly for the same reason - 32k+ seems high for a chapter count
    return boost::movelib::unique_ptr<CTexture>();
  }
  if (chapterNum < 1)
  {
    return boost::movelib::unique_ptr<CTexture>();
  }

  CFileItem item(cleanname, false);
  CTextureDetails details;
  details.file = CTextureCache::GetCacheFile(item.GetPath()) + ".jpg";
  if (CDVDFileInfo::ExtractThumb(item.GetPath(), details, NULL, chapterNum))
  {
    return CTexture::LoadFromFile(details.file);
  }
  return boost::movelib::unique_ptr<CTexture>();
}
