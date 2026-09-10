/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIContentUtils.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "addons/gui/GUIDialogAddonInfo.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "programs/dialogs/GUIDialogProgramInfo.h"
#include "video/VideoInfoTag.h"
#include "video/dialogs/GUIDialogVideoInfo.h"

#include <boost/make_shared.hpp>

using namespace UTILS::GUILIB;

bool CGUIContentUtils::HasInfoForItem(const CFileItem& item)
{
  if (item.HasVideoInfoTag())
  {
    MediaType mediaType = item.GetVideoInfoTag()->m_type;
    return (mediaType == MediaTypeMovie || mediaType == MediaTypeTvShow ||
            mediaType == MediaTypeSeason || mediaType == MediaTypeEpisode ||
            mediaType == MediaTypeVideo || mediaType == MediaTypeVideoCollection ||
            mediaType == MediaTypeMusicVideo);
  }

  return (item.HasMusicInfoTag() || item.HasAddonInfo() || item.HasProgramInfoTag());
}

bool CGUIContentUtils::ShowInfoForItem(const CFileItem& item)
{
  if (item.HasAddonInfo())
  {
    return CGUIDialogAddonInfo::ShowForItem(boost::make_shared<CFileItem>(item));
  }
  else if (item.HasVideoInfoTag())
  {
    CGUIDialogVideoInfo::ShowFor(item);
    return true;
  }
  else if (item.HasMusicInfoTag())
  {
    CGUIDialogMusicInfo::ShowFor(boost::make_shared<CFileItem>(item).get());
    return true;
  }
  else if (item.HasProgramInfoTag())
  {
    CGUIDialogProgramInfo::ShowFor(boost::make_shared<CFileItem>(item));
    return true;
  }
  return false;
}
