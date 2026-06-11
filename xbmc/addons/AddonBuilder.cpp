/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/AddonBuilder.h"

#include "ServiceBroker.h"
#include "addons/ContextMenuAddon.h"
#include "addons/FontResource.h"
#include "addons/ImageResource.h"
#include "addons/LanguageResource.h"
#include "addons/PluginSource.h"
#include "addons/Repository.h"
#include "addons/Scraper.h"
#include "addons/Service.h"
#include "addons/Skin.h"
#include "addons/UISoundsResource.h"
#include "addons/Webinterface.h"
#include "addons/addoninfo/AddonInfo.h"
#include "utils/StringUtils.h"

#include <boost/make_shared.hpp>

using namespace KODI;

namespace ADDON
{

AddonPtr CAddonBuilder::Generate(const AddonInfoPtr& info, AddonType::Type type)
{
  if (!info || info->ID().empty())
    return AddonPtr();

  if (type == AddonType::UNKNOWN)
    type = info->MainType();
  if (type == AddonType::UNKNOWN)
    return boost::make_shared<CAddon>(info, AddonType::UNKNOWN);

  // Handle screensaver special cases
  if (type == AddonType::SCREENSAVER)
  {
    // built in screensaver or python screensaver
    if (StringUtils::StartsWithNoCase(info->ID(), "screensaver.xbmc.builtin.") ||
        URIUtils::HasExtension(info->LibName(), ".py"))
      return boost::make_shared<CAddon>(info, type);
  }

  switch (type)
  {
    case AddonType::VISUALIZATION:
    case AddonType::SCREENSAVER:
      return boost::make_shared<CAddonDll>(info, type);
    case AddonType::PLUGIN:
    case AddonType::SCRIPT:
      return boost::make_shared<CPluginSource>(info, type);
    case AddonType::SCRIPT_LIBRARY:
    case AddonType::SCRIPT_LYRICS:
    case AddonType::SCRIPT_MODULE:
    case AddonType::SUBTITLE_MODULE:
    case AddonType::SCRIPT_WEATHER:
      return boost::make_shared<CAddon>(info, type);
    case AddonType::WEB_INTERFACE:
      return boost::make_shared<CWebinterface>(info);
    case AddonType::SERVICE:
      return boost::make_shared<CService>(info);
    case AddonType::SCRAPER_ALBUMS:
    case AddonType::SCRAPER_ARTISTS:
    case AddonType::SCRAPER_MOVIES:
    case AddonType::SCRAPER_MUSICVIDEOS:
    case AddonType::SCRAPER_TVSHOWS:
    case AddonType::SCRAPER_LIBRARY:
      return boost::make_shared<CScraper>(info, type);
    case AddonType::SKIN:
      return boost::make_shared<CSkinInfo>(info);
    case AddonType::RESOURCE_FONT:
      return boost::make_shared<CFontResource>(info);
    case AddonType::RESOURCE_IMAGES:
      return boost::make_shared<CImageResource>(info);
    case AddonType::RESOURCE_LANGUAGE:
      return boost::make_shared<CLanguageResource>(info);
    case AddonType::RESOURCE_UISOUNDS:
      return boost::make_shared<CUISoundsResource>(info);
    case AddonType::REPOSITORY:
      return boost::make_shared<CRepository>(info);
    case AddonType::CONTEXTMENU_ITEM:
      return boost::make_shared<CContextMenuAddon>(info);
    default:
      break;
  }
  return AddonPtr();
}

}
