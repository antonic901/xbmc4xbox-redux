/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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

#include "ServiceBroker.h"
#include "addons/AddonBuilder.h"
#include "addons/ContextMenuAddon.h"
#include "addons/ImageResource.h"
#include "addons/LanguageResource.h"
#include "addons/PluginSource.h"
#include "addons/Repository.h"
#include "addons/Scraper.h"
#include "addons/ScreenSaver.h"
#include "addons/Service.h"
#include "addons/Skin.h"
#include "addons/UISoundsResource.h"
#include "addons/Visualisation.h"
#include "addons/Webinterface.h"
#include "utils/StringUtils.h"

// Casting from boost::movelib::unique_ptr to boost::shared_ptr is not supported.
// In order to achive same behavior just like casting from std::unique_ptr to std::shared_ptr
// we need this custom deleter class which will ensure the unique pointer's ownership is correctly transferred.
template<typename T>
void deleter(T* ptr) {
    boost::movelib::unique_ptr<T> tmp(ptr);
}

namespace ADDON
{

boost::shared_ptr<IAddon> CAddonBuilder::Build()
{
  if (m_built)
    throw std::logic_error("Already built");

  if (m_props.id.empty())
    return boost::shared_ptr<IAddon>();

  m_built = true;

  if (m_props.type == ADDON_UNKNOWN)
    return boost::make_shared<CAddon>(boost::move(m_props));

  if (m_extPoint == nullptr)
    return FromProps(boost::move(m_props));

  const TYPE type(m_props.type);

  // Handle screensaver special cases
  if (type == ADDON_SCREENSAVER)
  {
    // built in screensaver
    if (StringUtils::StartsWithNoCase(m_extPoint->plugin->identifier, "screensaver.xbmc.builtin."))
      return boost::make_shared<CAddon>(boost::move(m_props));
    // python screensaver
    if (URIUtils::HasExtension(CServiceBroker::GetAddonMgr().GetExtValue(m_extPoint->configuration, "@library"), ".py"))
      return boost::make_shared<CScreenSaver>(boost::move(m_props));
  }

  // Ensure binary types have a valid library for the platform
  if (type == ADDON_VIZ ||
      type == ADDON_SCREENSAVER ||
      type == ADDON_PVRDLL ||
      type == ADDON_ADSPDLL ||
      type == ADDON_AUDIOENCODER ||
      type == ADDON_AUDIODECODER ||
      type == ADDON_INPUTSTREAM ||
      type == ADDON_PERIPHERALDLL)
  {
    std::string value = CServiceBroker::GetAddonMgr().GetPlatformLibraryName(m_extPoint->plugin->extensions->configuration);
    if (value.empty())
      return AddonPtr();
  }

  switch (type)
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
      return boost::shared_ptr<CPluginSource>(CPluginSource::FromExtension(boost::move(m_props), m_extPoint).release(), deleter<CPluginSource>);
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_MODULE:
    case ADDON_SUBTITLE_MODULE:
    case ADDON_SCRIPT_WEATHER:
      return boost::make_shared<CAddon>(boost::move(m_props));
    case ADDON_WEB_INTERFACE:
      return boost::shared_ptr<CWebinterface>(CWebinterface::FromExtension(boost::move(m_props), m_extPoint).release(), deleter<CWebinterface>);
    case ADDON_SERVICE:
      return boost::shared_ptr<CService>(CService::FromExtension(boost::move(m_props), m_extPoint).release(), deleter<CService>);
    case ADDON_SCRAPER_ALBUMS:
    case ADDON_SCRAPER_ARTISTS:
    case ADDON_SCRAPER_MOVIES:
    case ADDON_SCRAPER_MUSICVIDEOS:
    case ADDON_SCRAPER_TVSHOWS:
    case ADDON_SCRAPER_LIBRARY:
    case ADDON_SCRAPER_PROGRAMS:
      return boost::shared_ptr<CScraper>(CScraper::FromExtension(boost::move(m_props), m_extPoint).release(), deleter<CScraper>);
#if defined(HAS_VISUALISATION)
    case ADDON_VIZ:
      return boost::make_shared<CVisualisation>(boost::move(m_props));
#endif
    case ADDON_SCREENSAVER:
      return boost::make_shared<CScreenSaver>(boost::move(m_props));
    case ADDON_SKIN:
      return boost::shared_ptr<CSkinInfo>(CSkinInfo::FromExtension(boost::move(m_props), m_extPoint).release(), deleter<CSkinInfo>);
    case ADDON_RESOURCE_IMAGES:
      return boost::shared_ptr<CImageResource>(CImageResource::FromExtension(boost::move(m_props), m_extPoint).release(), deleter<CImageResource>);
    case ADDON_RESOURCE_LANGUAGE:
      return boost::shared_ptr<CLanguageResource>(CLanguageResource::FromExtension(boost::move(m_props), m_extPoint).release(), deleter<CLanguageResource>);
    case ADDON_RESOURCE_UISOUNDS:
      return boost::make_shared<CUISoundsResource>(boost::move(m_props));
    case ADDON_REPOSITORY:
      return boost::shared_ptr<CRepository>(CRepository::FromExtension(boost::move(m_props), m_extPoint).release(), deleter<CRepository>);
    case ADDON_CONTEXT_ITEM:
      return boost::shared_ptr<CContextMenuAddon>(CContextMenuAddon::FromExtension(boost::move(m_props), m_extPoint).release(), deleter<CContextMenuAddon>);
    default:
      break;
  }
  return AddonPtr();
}


AddonPtr CAddonBuilder::FromProps(AddonProps addonProps)
{
  // FIXME: there is no need for this as none of the derived classes will contain any useful
  // information. We should return CAddon instances only, however there are several places that
  // down casts, which need to fixed first.
  switch (addonProps.type)
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
      return AddonPtr(new CPluginSource(boost::move(addonProps)));
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_WEATHER:
    case ADDON_SCRIPT_MODULE:
    case ADDON_SUBTITLE_MODULE:
      return AddonPtr(new CAddon(boost::move(addonProps)));
    case ADDON_WEB_INTERFACE:
      return AddonPtr(new CWebinterface(boost::move(addonProps)));
    case ADDON_SERVICE:
      return AddonPtr(new CService(boost::move(addonProps)));
    case ADDON_SCRAPER_ALBUMS:
    case ADDON_SCRAPER_ARTISTS:
    case ADDON_SCRAPER_MOVIES:
    case ADDON_SCRAPER_MUSICVIDEOS:
    case ADDON_SCRAPER_TVSHOWS:
    case ADDON_SCRAPER_LIBRARY:
    case ADDON_SCRAPER_PROGRAMS:
      return AddonPtr(new CScraper(boost::move(addonProps)));
    case ADDON_SKIN:
      return AddonPtr(new CSkinInfo(boost::move(addonProps)));
#if defined(HAS_VISUALISATION)
    case ADDON_VIZ:
      return AddonPtr(new CVisualisation(boost::move(addonProps)));
#endif
    case ADDON_SCREENSAVER:
      return AddonPtr(new CScreenSaver(boost::move(addonProps)));
    case ADDON_RESOURCE_IMAGES:
      return AddonPtr(new CImageResource(boost::move(addonProps)));
    case ADDON_RESOURCE_LANGUAGE:
      return AddonPtr(new CLanguageResource(boost::move(addonProps)));
    case ADDON_RESOURCE_UISOUNDS:
      return AddonPtr(new CUISoundsResource(boost::move(addonProps)));
    case ADDON_REPOSITORY:
      return AddonPtr(new CRepository(boost::move(addonProps)));
    case ADDON_CONTEXT_ITEM:
      return AddonPtr(new CContextMenuAddon(boost::move(addonProps)));
    default:
      break;
  }
  return AddonPtr();
}
}
