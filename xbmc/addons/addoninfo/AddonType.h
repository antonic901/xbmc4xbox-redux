/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/addoninfo/AddonExtensions.h"

#include <set>
#include <string>

class TiXmlElement;

namespace ADDON
{

namespace AddonType
{
enum Type
{
  UNKNOWN = 0,
  VISUALIZATION,
  SKIN,
  SCRIPT,
  SCRIPT_WEATHER,
  SUBTITLE_MODULE,
  SCRIPT_LYRICS,
  SCRAPER_ALBUMS,
  SCRAPER_ARTISTS,
  SCRAPER_MOVIES,
  SCRAPER_MUSICVIDEOS,
  SCRAPER_TVSHOWS,
  SCREENSAVER,
  PLUGIN,
  REPOSITORY,
  WEB_INTERFACE,
  SERVICE,
  CONTEXTMENU_ITEM,
  RESOURCE_IMAGES,
  RESOURCE_LANGUAGE,
  RESOURCE_UISOUNDS,
  RESOURCE_FONT,
  SCRAPER_LIBRARY,
  SCRIPT_LIBRARY,
  SCRIPT_MODULE,
  VIDEOCODEC,

  /**
    * @brief virtual addon types
    */
  //@{
  VIDEO,
  AUDIO,
  IMAGE,
  EXECUTABLE,
  GAME,
  //@}

  MAX_TYPES
};
}

class CAddonInfoBuilder;
class CAddonDatabaseSerializer;

class CAddonType : public CAddonExtensions
{
public:
  CAddonType(AddonType::Type type = AddonType::UNKNOWN) : m_type(type) {}

  AddonType::Type Type() const { return m_type; }
  std::string LibPath() const;
  const std::string& LibName() const { return m_libname; }

  bool ProvidesSubContent(const AddonType::Type& content) const
  {
    return content == AddonType::UNKNOWN
               ? false
               : m_type == content || m_providedSubContent.count(content) > 0;
  }

  bool ProvidesSeveralSubContents() const
  {
    return m_providedSubContent.size() > 1;
  }

  size_t ProvidedSubContents() const
  {
    return m_providedSubContent.size();
  }

  /*!
   * @brief Indicates whether a given type is a dependency type (e.g. addons which the main type is
   * a script.module)
   *
   * @param[in] type the provided type
   * @return true if type is one of the dependency types
   */
  static bool IsDependencyType(AddonType::Type type);

private:
  friend class CAddonInfoBuilder;
  friend class CAddonInfoBuilderFromDB;
  friend class CAddonDatabaseSerializer;

  void SetProvides(const std::string& content);

  AddonType::Type m_type;
  std::string m_path;
  std::string m_libname;
  std::set<AddonType::Type> m_providedSubContent;
};

} /* namespace ADDON */
