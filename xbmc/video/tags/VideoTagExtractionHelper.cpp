/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoTagExtractionHelper.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "TextureDatabase.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"
#include "video/tags/VideoTagLoaderFFmpeg.h"

using namespace VIDEO::TAGS;

bool CVideoTagExtractionHelper::IsExtractionSupportedFor(const CFileItem& item)
{
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
             CSettings::SETTING_MYVIDEOS_USETAGS) &&
         URIUtils::HasExtension(item.GetDynPath(), ".mkv|.mp4|.avi|.m4v");
}

std::string CVideoTagExtractionHelper::ExtractEmbeddedArtFor(const CFileItem& item,
                                                             const std::string& artType)
{
  CVideoTagLoaderFFmpeg loader(item, ADDON::ScraperPtr(), false);
  CVideoInfoTag tag;
  loader.Load(tag, false, NULL);
  for (std::vector<EmbeddedArtInfo>::const_iterator it = tag.m_coverArt.begin(); it != tag.m_coverArt.end(); ++it)
  {
    if (it->m_type == artType)
      return CTextureUtils::GetWrappedImageURL(item.GetDynPath(), "video_" + artType);
  }
  return std::string();
}
