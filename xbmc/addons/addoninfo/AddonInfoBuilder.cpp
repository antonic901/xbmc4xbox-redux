/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonInfoBuilder.h"

#include "LangInfo.h"
#include "addons/Repository.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <algorithm>
#include <boost/make_shared.hpp>

namespace
{
// Note that all of these characters are url-safe
const std::string VALID_ADDON_IDENTIFIER_CHARACTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_@!$";
}

namespace ADDON
{

CAddonInfoBuilderFromDB::CAddonInfoBuilderFromDB() : m_addonInfo(boost::make_shared<CAddonInfo>())
{
}

void CAddonInfoBuilderFromDB::SetId(std::string id)
{
  m_addonInfo->m_id = boost::move(id);
}

void CAddonInfoBuilderFromDB::SetName(std::string name)
{
  m_addonInfo->m_name = boost::move(name);
}

void CAddonInfoBuilderFromDB::SetLicense(std::string license)
{
  m_addonInfo->m_license = boost::move(license);
}

void CAddonInfoBuilderFromDB::SetSummary(std::string summary)
{
  m_addonInfo->m_summary.insert(std::pair<std::string, std::string>("unk", boost::move(summary)));
}

void CAddonInfoBuilderFromDB::SetDescription(std::string description)
{
  m_addonInfo->m_description.insert(
      std::pair<std::string, std::string>("unk", boost::move(description)));
}

void CAddonInfoBuilderFromDB::SetDisclaimer(std::string disclaimer)
{
  m_addonInfo->m_disclaimer.insert(
      std::pair<std::string, std::string>("unk", boost::move(disclaimer)));
}

void CAddonInfoBuilderFromDB::SetAuthor(std::string author)
{
  m_addonInfo->m_author = boost::move(author);
}

void CAddonInfoBuilderFromDB::SetSource(std::string source)
{
  m_addonInfo->m_source = boost::move(source);
}

void CAddonInfoBuilderFromDB::SetWebsite(std::string website)
{
  m_addonInfo->m_website = boost::move(website);
}

void CAddonInfoBuilderFromDB::SetForum(std::string forum)
{
  m_addonInfo->m_forum = boost::move(forum);
}

void CAddonInfoBuilderFromDB::SetEMail(std::string email)
{
  m_addonInfo->m_email = boost::move(email);
}

void CAddonInfoBuilderFromDB::SetIcon(std::string icon)
{
  m_addonInfo->m_icon = boost::move(icon);
}

void CAddonInfoBuilderFromDB::SetArt(const std::string& type, std::string value)
{
  m_addonInfo->m_art[type] = boost::move(value);
}

void CAddonInfoBuilderFromDB::SetArt(std::map<std::string, std::string> art)
{
  m_addonInfo->m_art = boost::move(art);
}

void CAddonInfoBuilderFromDB::SetScreenshots(std::vector<std::string> screenshots)
{
  m_addonInfo->m_screenshots = boost::move(screenshots);
}

void CAddonInfoBuilderFromDB::SetChangelog(std::string changelog)
{
  m_addonInfo->m_changelog.insert(std::pair<std::string, std::string>("unk", boost::move(changelog)));
}

void CAddonInfoBuilderFromDB::SetLifecycleState(AddonLifecycleState::Type state, std::string description)
{
  m_addonInfo->m_lifecycleState = state;
  m_addonInfo->m_lifecycleStateDescription.insert(std::make_pair("unk", boost::move(description)));
}

void CAddonInfoBuilderFromDB::SetPath(std::string path)
{
  m_addonInfo->m_path = boost::move(path);
}

void CAddonInfoBuilderFromDB::SetLibName(std::string libname)
{
  m_addonInfo->m_libname = boost::move(libname);
}

void CAddonInfoBuilderFromDB::SetVersion(CAddonVersion version)
{
  m_addonInfo->m_version = boost::move(version);
}

void CAddonInfoBuilderFromDB::SetDependencies(std::vector<DependencyInfo> dependencies)
{
  m_addonInfo->m_dependencies = boost::move(dependencies);
}

void CAddonInfoBuilderFromDB::SetExtrainfo(InfoMap extrainfo)
{
  m_addonInfo->m_extrainfo = boost::move(extrainfo);
}

void CAddonInfoBuilderFromDB::SetInstallDate(const CDateTime& installDate)
{
  m_addonInfo->m_installDate = installDate;
}

void CAddonInfoBuilderFromDB::SetLastUpdated(const CDateTime& lastUpdated)
{
  m_addonInfo->m_lastUpdated = lastUpdated;
}

void CAddonInfoBuilderFromDB::SetLastUsed(const CDateTime& lastUsed)
{
  m_addonInfo->m_lastUsed = lastUsed;
}

void CAddonInfoBuilderFromDB::SetOrigin(std::string origin)
{
  m_addonInfo->m_origin = boost::move(origin);
}

void CAddonInfoBuilderFromDB::SetPackageSize(uint64_t size)
{
  m_addonInfo->m_packageSize = size;
}

void CAddonInfoBuilderFromDB::SetExtensions(CAddonType addonType)
{
  if (!addonType.GetValue("provides").empty())
    addonType.SetProvides(addonType.GetValue("provides").asString());

  m_addonInfo->m_types.push_back(boost::move(addonType));
  m_addonInfo->m_mainType = addonType.m_type;
}

AddonInfoPtr CAddonInfoBuilder::Generate(const std::string& id, AddonType::Type type)
{
  // Check addon identifier for forbidden characters
  // The identifier is used e.g. in URLs so we shouldn't allow just
  // any character to go through.
  if (id.empty() || id.find_first_not_of(VALID_ADDON_IDENTIFIER_CHARACTERS) != std::string::npos)
  {
    CLog::Log(LOGERROR, "CAddonInfoBuilder::%s: identifier '%s' is invalid", __FUNCTION__, id.c_str());
    return AddonInfoPtr();
  }

  AddonInfoPtr addon = boost::make_shared<CAddonInfo>();
  addon->m_id = id;
  addon->m_mainType = type;
  return addon;
}

AddonInfoPtr CAddonInfoBuilder::Generate(const std::string& addonPath, bool platformCheck /*= true*/)
{
  std::string addonRealPath = CSpecialProtocol::TranslatePath(addonPath);

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(URIUtils::AddFileToFolder(addonRealPath, "addon.xml")))
  {
    CLog::Log(LOGERROR, "CAddonInfoBuilder::%s: Unable to load '%s', Line %i\n%s",
                                               __FUNCTION__,
                                               URIUtils::AddFileToFolder(addonRealPath, "addon.xml").c_str(),
                                               xmlDoc.ErrorRow(),
                                               xmlDoc.ErrorDesc());
    return AddonInfoPtr();
  }

  AddonInfoPtr addon = boost::make_shared<CAddonInfo>();
  if (!ParseXML(addon, xmlDoc.RootElement(), addonRealPath))
    return AddonInfoPtr();

  if (!platformCheck || PlatformSupportsAddon(addon))
    return addon;

  CLog::Log(LOGERROR, "CAddonInfoBuilder::%s: No platform for add-on %s (supported platforms: %s)",
            __FUNCTION__, addon->ID().c_str(), StringUtils::Join(addon->m_platforms, ", ").c_str());

  return AddonInfoPtr();
}

AddonInfoPtr CAddonInfoBuilder::Generate(const TiXmlElement* baseElement,
                                         const RepositoryDirInfo& repo,
                                         bool platformCheck /*= true*/)
{
  AddonInfoPtr addon = boost::make_shared<CAddonInfo>();
  if (!ParseXML(addon, baseElement, repo.datadir, repo))
    return AddonInfoPtr();

  if (!platformCheck || PlatformSupportsAddon(addon))
    return addon;

  return AddonInfoPtr();
}

void CAddonInfoBuilder::SetInstallData(const AddonInfoPtr& addon, const CDateTime& installDate, const CDateTime& lastUpdated,
                                       const CDateTime& lastUsed, const std::string& origin)
{
  if (!addon)
    return;

  addon->m_installDate = installDate;
  addon->m_lastUpdated = lastUpdated;
  addon->m_lastUsed = lastUsed;
  addon->m_origin = origin;
}

bool CAddonInfoBuilder::ParseXML(const AddonInfoPtr& addon,
                                 const TiXmlElement* element,
                                 const std::string& addonPath)
{
  return ParseXML(addon, element, addonPath, RepositoryDirInfo());
}

static bool RemoveEmptyPlatform(const std::string& platform) { return platform.empty(); }

bool CAddonInfoBuilder::ParseXML(const AddonInfoPtr& addon,
                                 const TiXmlElement* element,
                                 const std::string& addonPath,
                                 const RepositoryDirInfo& repo)
{
  /*
   * Following values currently not set from creator:
   * - CDateTime installDate;
   * - CDateTime lastUpdated;
   * - CDateTime lastUsed;
   * - std::string origin;
   */

  if (!StringUtils::EqualsNoCase(element->Value(), "addon"))
  {
    CLog::Log(LOGERROR, "CAddonInfoBuilder::%s: file from '%s' doesn't contain <addon>", __FUNCTION__, addonPath.c_str());
    return false;
  }

  /*
   * The function variable "repo" is only used when reading data stored on the internet.
   * A boolean value is then set here for easier identification.
   */
  const bool isRepoXMLContent = !repo.datadir.empty();

  /*
   * Parse addon.xml:
   * <addon id="???"
   *        name="???"
   *        version="???"
   *        provider-name="???">
   */
  addon->m_id = StringUtils::CreateFromCString(element->Attribute("id"));
  addon->m_name = StringUtils::CreateFromCString(element->Attribute("name"));
  addon->m_author = StringUtils::CreateFromCString(element->Attribute("provider-name"));

  const std::string version = StringUtils::CreateFromCString(element->Attribute("version"));
  addon->m_version = CAddonVersion(version);

  if (addon->m_id.empty() || addon->m_version.empty())
  {
    CLog::Log(LOGERROR, "CAddonInfoBuilder::%s: file '%s' doesn't contain required values on <addon ... > id='%s', version='%s'",
              __FUNCTION__,
              addonPath.c_str(),
              addon->m_id.empty() ? "missing" : addon->m_id.c_str(),
              addon->m_version.empty() ? "missing" : addon->m_version.asString().c_str());
    return false;
  }

  // Check addon identifier for forbidden characters
  // The identifier is used e.g. in URLs so we shouldn't allow just
  // any character to go through.
  if (addon->m_id.find_first_not_of(VALID_ADDON_IDENTIFIER_CHARACTERS) != std::string::npos)
  {
    CLog::Log(LOGERROR, "CAddonInfoBuilder::%s: identifier %s is invalid", __FUNCTION__, addon->m_id.c_str());
    return false;
  }

  /*
   * Parse addon.xml:
   * <backwards-compatibility abi="???"/>
   */
  const TiXmlElement* backwards = element->FirstChildElement("backwards-compatibility");
  if (backwards)
  {
    const std::string minVersion = StringUtils::CreateFromCString(backwards->Attribute("abi"));
    addon->m_minversion = CAddonVersion(minVersion);
  }

  /*
   * Parse addon.xml:
   * <requires>
   *   <import addon="???" minversion="???" version="???" optional="???"/>
   * </requires>
   */
  const TiXmlElement* _requires = element->FirstChildElement("requires");
  if (_requires)
  {
    for (const TiXmlElement* child = _requires->FirstChildElement("import"); child != NULL;
         child = child->NextSiblingElement("import"))
    {
      if (child->Attribute("addon"))
      {
        const std::string minVersion =
            StringUtils::CreateFromCString(child->Attribute("minversion"));
        const std::string version = StringUtils::CreateFromCString(child->Attribute("version"));

        bool optional = false;
        child->QueryBoolAttribute("optional", &optional);

        addon->m_dependencies.push_back(DependencyInfo(child->Attribute("addon"), CAddonVersion(minVersion),
                                           CAddonVersion(version), optional));
      }
    }
  }

  std::string assetBasePath;
  if (!isRepoXMLContent && !addonPath.empty())
  {
    // Default for add-on information not loaded from repository
    assetBasePath = addonPath;
    addon->m_path = addonPath;
  }
  else
  {
    assetBasePath = URIUtils::AddFileToFolder(repo.artdir, addon->m_id);
    addon->m_path = URIUtils::AddFileToFolder(repo.datadir, addon->m_id, StringUtils::Format("%s-%s.zip", addon->m_id.c_str(), addon->m_version.asString().c_str()));
  }

  addon->m_profilePath = StringUtils::Format("special://profile/addon_data/%s/", addon->m_id.c_str());

  /*
   * Parse addon.xml:
   * <extension>
   *   ...
   * </extension>
   */
  for (const TiXmlElement* child = element->FirstChildElement("extension"); child != NULL; child = child->NextSiblingElement("extension"))
  {
    const std::string point = StringUtils::CreateFromCString(child->Attribute("point"));

    if (point == "kodi.addon.metadata" || point == "xbmc.addon.metadata")
    {
      /*
       * Parse addon.xml "<path">...</path>" (special related to repository path),
       * do first and if present override the default. Also set assetBasePath to
       * find screenshots and icons.
       */
      element = child->FirstChildElement("path");
      if (element && element->GetText() != NULL && !repo.datadir.empty())
      {
        addon->m_path = URIUtils::AddFileToFolder(repo.datadir, element->GetText());
        assetBasePath = URIUtils::GetDirectory(URIUtils::AddFileToFolder(repo.artdir, element->GetText()));
      }

      /*
       * Parse addon.xml "<summary lang="..">...</summary>"
       */
      GetTextList(child, "summary", addon->m_summary);

      /*
       * Parse addon.xml "<description lang="..">...</description>"
       */
      GetTextList(child, "description", addon->m_description);

      /*
       * Parse addon.xml "<disclaimer lang="..">...</disclaimer>"
       */
      GetTextList(child, "disclaimer", addon->m_disclaimer);

      /*
       * Parse addon.xml "<assets>...</assets>"
       */
      const TiXmlElement* element = child->FirstChildElement("assets");
      if (element)
      {
        for (const TiXmlElement* elementsAssets = element->FirstChildElement(); elementsAssets != NULL; elementsAssets = elementsAssets->NextSiblingElement())
        {
          std::string value = elementsAssets->Value();
          if (value == "icon")
          {
            if (elementsAssets->GetText() != NULL)
              addon->m_icon = URIUtils::AddFileToFolder(assetBasePath, elementsAssets->GetText());
          }
          else if (value == "screenshot")
          {
            if (elementsAssets->GetText() != NULL)
              addon->m_screenshots.push_back(URIUtils::AddFileToFolder(assetBasePath, elementsAssets->GetText()));
          }
          else if (value == "fanart")
          {
            if (elementsAssets->GetText() != NULL)
              addon->m_art[value] = URIUtils::AddFileToFolder(assetBasePath, elementsAssets->GetText());
          }
          else if (value == "banner")
          {
            if (elementsAssets->GetText() != NULL)
              addon->m_art[value] = URIUtils::AddFileToFolder(assetBasePath, elementsAssets->GetText());
          }
          else if (value == "clearlogo")
          {
            if (elementsAssets->GetText() != NULL)
              addon->m_art[value] = URIUtils::AddFileToFolder(assetBasePath, elementsAssets->GetText());
          }
          else if (value == "thumb")
          {
            if (elementsAssets->GetText() != NULL)
              addon->m_art[value] =
                  URIUtils::AddFileToFolder(assetBasePath, elementsAssets->GetText());
          }
        }
      }

      /* Parse addon.xml "<platform">...</platform>" */
      element = child->FirstChildElement("platform");
      if (element && element->GetText() != NULL)
      {
        std::vector<std::string> temp;
        temp.push_back(" ");
        temp.push_back("\t");
        temp.push_back("\n");
        temp.push_back("\r");
        std::vector<std::string> platforms = StringUtils::Split(element->GetText(), temp);
        platforms.erase(std::remove_if(platforms.begin(), platforms.end(), RemoveEmptyPlatform),
                        platforms.end());
        addon->m_platforms = platforms;
      }

      /* Parse addon.xml "<license">...</license>" */
      element = child->FirstChildElement("license");
      if (element && element->GetText() != NULL)
        addon->m_license = element->GetText();

      /* Parse addon.xml "<source">...</source>" */
      element = child->FirstChildElement("source");
      if (element && element->GetText() != NULL)
        addon->m_source = element->GetText();

      /* Parse addon.xml "<email">...</email>" */
      element = child->FirstChildElement("email");
      if (element && element->GetText() != NULL)
        addon->m_email = element->GetText();

      /* Parse addon.xml "<website">...</website>" */
      element = child->FirstChildElement("website");
      if (element && element->GetText() != NULL)
        addon->m_website = element->GetText();

      /* Parse addon.xml "<forum">...</forum>" */
      element = child->FirstChildElement("forum");
      if (element && element->GetText() != NULL)
        addon->m_forum = element->GetText();

      /* Parse addon.xml "<broken">...</broken>"
       * NOTE: Replaced with <lifecyclestate>, available for backward compatibility */
      element = child->FirstChildElement("broken");
      if (element && element->GetText() != NULL)
      {
        addon->m_lifecycleState = AddonLifecycleState::BROKEN;
        addon->m_lifecycleStateDescription.emplace(KODI_ADDON_DEFAULT_LANGUAGE_CODE,
                                                   element->GetText());
      }

      /* Parse addon.xml "<lifecyclestate">...</lifecyclestate>" */
      element = child->FirstChildElement("lifecyclestate");
      if (element && element->GetText() != NULL)
      {
        const char* lang = element->Attribute("type");
        if (lang)
        {
          if (strcmp(lang, "broken") == 0)
            addon->m_lifecycleState = AddonLifecycleState::BROKEN;
          else if (strcmp(lang, "deprecated") == 0)
            addon->m_lifecycleState = AddonLifecycleState::DEPRECATED;
          else
            addon->m_lifecycleState = AddonLifecycleState::NORMAL;

          GetTextList(child, "lifecyclestate", addon->m_lifecycleStateDescription);
        }
      }

      /* Parse addon.xml "<language">...</language>" */
      element = child->FirstChildElement("language");
      if (element && element->GetText() != NULL)
        addon->AddExtraInfo("language", element->GetText());

      /* Parse addon.xml "<reuselanguageinvoker">...</reuselanguageinvoker>" */
      element = child->FirstChildElement("reuselanguageinvoker");
      if (element && element->GetText() != NULL)
        addon->AddExtraInfo("reuselanguageinvoker", element->GetText());

      /* Parse addon.xml "<size">...</size>" */
      element = child->FirstChildElement("size");
      if (element && element->GetText() != NULL)
        addon->m_packageSize = StringUtils::ToUint64(element->GetText(), 0);

      /* Parse addon.xml "<news lang="..">...</news>"
       *
       * In the event that the changelog (news) in addon.xml is empty, check
       * whether it is an installed addon and read a changelog.txt as a
       * replacement, if available. */
      GetTextList(child, "news", addon->m_changelog);
      if (addon->m_changelog.empty() && !isRepoXMLContent && !addonPath.empty())
      {
        using XFILE::CFile;

        const std::string changelog = URIUtils::AddFileToFolder(addonPath, "changelog.txt");
        if (CFile::Exists(changelog))
        {
          CFile file;
          XFILE::auto_buffer buf;
          if (file.LoadFile(changelog, buf) > 0)
            addon->m_changelog[KODI_ADDON_DEFAULT_LANGUAGE_CODE].assign(
                reinterpret_cast<char*>(buf.get(), buf.length()));
        }
      }
    }
    else
    {
      AddonType::Type type = CAddonInfo::TranslateType(point);
      if (type == AddonType::UNKNOWN || type >= AddonType::MAX_TYPES)
      {
        CLog::Log(LOGERROR, "CAddonInfoBuilder::%s: file '%s' doesn't contain a valid add-on type name (%s)", __FUNCTION__, addon->m_path.c_str(), point.c_str());
        return false;
      }

      CAddonType addonType(type);
      if (ParseXMLTypes(addonType, addon, child))
        addon->m_types.push_back(boost::move(addonType));
    }
  }

  /*
   * If nothing is defined in addon.xml set addon as unknown to have minimum one
   * instance type present.
   */
  if (addon->m_types.empty())
  {
    CAddonType addonType(AddonType::UNKNOWN);
    addon->m_types.push_back(boost::move(addonType));
  }

  addon->m_mainType = addon->m_types[0].Type();
  addon->m_libname = addon->m_types[0].m_libname;
  if (!addon->m_types[0].GetValue("provides").empty())
    addon->AddExtraInfo("provides", addon->m_types[0].GetValue("provides").asString());

  // Ensure binary types have a valid library for the platform
  if (addon->m_mainType == AddonType::VISUALIZATION ||
      addon->m_mainType == AddonType::SCREENSAVER)
  {
    if (addon->m_libname.empty())
    {
      // Prevent log file entry if data is from repository, there normal on
      // addons for other OS's
      if (!isRepoXMLContent)
        CLog::Log(LOGERROR, "CAddonInfoBuilder::%s: addon.xml from '%s' for binary type '%s' doesn't contain library and addon becomes ignored",
                      __FUNCTION__, addon->ID().c_str(), CAddonInfo::TranslateType(addon->m_mainType).c_str());
      return false;
    }
  }

  if (!isRepoXMLContent)
  {
    using XFILE::CFile;
    if (CFile::Exists(URIUtils::AddFileToFolder(addonPath, "resources", "settings.xml")))
      addon->m_supportsAddonSettings = true;
    if (CFile::Exists(URIUtils::AddFileToFolder(addonPath, "resources", "instance-settings.xml")))
      addon->m_supportsInstanceSettings = true;
  }

  addon->m_addonInstanceSupportType = CAddonInfo::InstanceSupportType(addon->m_mainType);

  return true;
}

bool CAddonInfoBuilder::ParseXMLTypes(CAddonType& addonType,
                                      const AddonInfoPtr& info,
                                      const TiXmlElement* child)
{
  if (child)
  {
    addonType.m_path = info->Path();

    // Get add-on library file name (if present)
    const char* library = child->Attribute("library");
    if (library == NULL)
      library = GetPlatformLibraryName(child);
    if (library != NULL)
    {
      addonType.m_libname = library;

      // linux is different and has the version number after the suffix
      const std::string libRegex("^.*.dll\\.?[0-9]*\\.?[0-9]*\\.?[0-9]*$");
      CRegExp re(true, CRegExp::autoUtf8);
      if (re.RegComp(libRegex) && re.RegFind(library))
      {
        info->SetBinary(true);
        CLog::Log(LOGDEBUG, "CAddonInfoBuilder::%s: Binary addon found: %s", __FUNCTION__,
                  info->ID().c_str());
      }
    }

    if (!ParseXMLExtension(addonType, child))
    {
      CLog::Log(LOGERROR, "CAddonInfoBuilder::%s: addon.xml file doesn't contain a valid add-on extensions (%s)", __FUNCTION__, info->ID().c_str());
      return false;
    }
    if (!addonType.GetValue("provides").empty())
      addonType.SetProvides(addonType.GetValue("provides").asString());
    return true;
  }
  return false;
}

bool CAddonInfoBuilder::ParseXMLExtension(CAddonExtensions& addonExt, const TiXmlElement* element)
{
  addonExt.m_point = StringUtils::CreateFromCString(element->Attribute("point"));

  EXT_VALUE extension;
  const TiXmlAttribute* attribute = element->FirstAttribute();
  while (attribute)
  {
    std::string name = attribute->Name();
    if (name != "point")
    {
      const std::string value = StringUtils::CreateFromCString(attribute->Value());
      if (!value.empty())
      {
        name = "@" + name;
        extension.push_back(std::make_pair(name, SExtValue(value)));
      }
    }
    attribute = attribute->Next();
  }
  if (!extension.empty())
    addonExt.m_values.push_back(std::pair<std::string, EXT_VALUE>("", boost::move(extension)));

  const TiXmlElement* childElement = element->FirstChildElement();
  while (childElement)
  {
    const std::string id = StringUtils::CreateFromCString(childElement->Value());
    if (!id.empty())
    {
      EXT_VALUE extension;
      const TiXmlAttribute* attribute = childElement->FirstAttribute();
      while (attribute)
      {
        std::string name = attribute->Name();
        if (name != "point")
        {
          const std::string value = StringUtils::CreateFromCString(attribute->Value());
          if (!value.empty())
          {
            name = id + "@" + name;
            extension.push_back(std::make_pair(name, SExtValue(value)));
          }
        }
        attribute = attribute->Next();
      }

      const std::string childElementText = StringUtils::CreateFromCString(childElement->GetText());

      if (!childElementText.empty())
      {
        extension.push_back(std::make_pair(id, SExtValue(childElementText)));
      }

      if (!extension.empty())
        addonExt.m_values.push_back(std::make_pair(id, boost::move(extension)));

      if (childElementText.empty())
      {
        const TiXmlElement* childSubElement = childElement->FirstChildElement();
        if (childSubElement)
        {
          CAddonExtensions subElement;
          if (ParseXMLExtension(subElement, childElement))
            addonExt.m_children.push_back(std::make_pair(id, boost::move(subElement)));
        }
      }
    }
    childElement = childElement->NextSiblingElement();
  }

  return true;
}

bool CAddonInfoBuilder::GetTextList(const TiXmlElement* element, const std::string& tag, boost::unordered_map<std::string, std::string>& translatedValues)
{
  if (!element)
    return false;

  translatedValues.clear();

  for (const TiXmlElement* child = element->FirstChildElement(tag); child != NULL; child = child->NextSiblingElement(tag))
  {
    const char* lang = child->Attribute("lang");
    const char* text = child->GetText();
    if (lang != NULL)
    {
      if (strcmp(lang, "no") == 0)
        translatedValues.insert(std::make_pair("nb_NO", text != NULL ? text : ""));
      else
        translatedValues.insert(std::make_pair(lang, text != NULL ? text : ""));
    }
    else
      translatedValues.insert(
          std::make_pair(KODI_ADDON_DEFAULT_LANGUAGE_CODE, text != NULL ? text : ""));
  }

  return !translatedValues.empty();
}

const char* CAddonInfoBuilder::GetPlatformLibraryName(const TiXmlElement* element)
{
  const char* libraryName;
#if defined(TARGET_ANDROID)
  libraryName = element->Attribute("library_android");
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
#if defined(TARGET_FREEBSD)
  libraryName = element->Attribute("library_freebsd");
  if (libraryName == NULL)
#endif
  libraryName = element->Attribute("library_linux");
#elif defined(TARGET_WINDOWS_DESKTOP)
  libraryName = element->Attribute("library_windx");
  if (libraryName == NULL)
    libraryName = element->Attribute("library_windows");
#elif defined(TARGET_WINDOWS_STORE)
  libraryName = element->Attribute("library_windowsstore");
#elif defined(TARGET_DARWIN)
#if defined(TARGET_DARWIN_EMBEDDED)
  libraryName = element->Attribute("library_darwin_embedded");
#else
  libraryName = element->Attribute("library_osx");
#endif
#elif defined(_XBOX)
  libraryName = element->Attribute("library_xbox");
#endif

  return libraryName;
}

bool CAddonInfoBuilder::PlatformSupportsAddon(const AddonInfoPtr& addon)
{
  if (addon->m_platforms.empty())
    return true;

  std::vector<std::string> supportedPlatforms;
  supportedPlatforms.push_back("all");
  supportedPlatforms.push_back("xbox");

  return std::find_first_of(addon->m_platforms.begin(), addon->m_platforms.end(),
      supportedPlatforms.begin(), supportedPlatforms.end()) != addon->m_platforms.end();
}

}
