#pragma once
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
#include "IAddon.h"
#include "Service.h"

namespace ADDON
{

class CAddonBuilder
{
public:
  CAddonBuilder() : m_built(false), m_extPoint(nullptr) {}

  boost::shared_ptr<IAddon> Build();
  void SetId(std::string id) { m_props.id = boost::move(id); }
  void SetName(std::string name) { m_props.name = boost::move(name); }
  void SetLicense(std::string license) { m_props.license = boost::move(license); }
  void SetSummary(std::string summary) { m_props.summary = boost::move(summary); }
  void SetDescription(std::string description) { m_props.description = boost::move(description); }
  void SetDisclaimer(std::string disclaimer) { m_props.disclaimer = boost::move(disclaimer); }
  void SetAuthor(std::string author) { m_props.author = boost::move(author); }
  void SetSource(std::string source) { m_props.source = boost::move(source); }
  void SetIcon(std::string icon) { m_props.icon = boost::move(icon); }
  void SetFanart(std::string fanart) { m_props.fanart = boost::move(fanart); }
  void SetScreenshots(std::vector<std::string> screenshots) { m_props.screenshots = boost::move(screenshots); }
  void SetChangelog(std::string changelog) { m_props.changelog = boost::move(changelog); }
  void SetBroken(std::string broken) { m_props.broken = boost::move(broken); }
  void SetPath(std::string path) { m_props.path = boost::move(path); }
  void SetLibName(std::string libname) { m_props.libname = boost::move(libname); }
  void SetVersion(AddonVersion version) { m_props.version = boost::move(version); }
  void SetMinVersion(AddonVersion minversion) { m_props.minversion = boost::move(minversion); }
  void SetDependencies(ADDONDEPS dependencies) { m_props.dependencies = boost::move(dependencies); }
  void SetExtrainfo(InfoMap extrainfo) { m_props.extrainfo = boost::move(extrainfo); }
  void SetType(TYPE type) { m_props.type = type; }
  void SetExtPoint(cp_extension_t* ext) { m_extPoint = ext; }
  void SetInstallDate(CDateTime installDate) { m_props.installDate = installDate; }
  void SetLastUpdated(CDateTime lastUpdated) { m_props.lastUpdated = lastUpdated; }
  void SetLastUsed(CDateTime lastUsed) { m_props.lastUsed = lastUsed; }
  void SetOrigin(std::string origin) { m_props.origin = boost::move(origin); }
  void SetPackageSize(uint64_t size) { m_props.packageSize = size; }

  const std::string& GetId() const { return m_props.id; }
  const AddonVersion& GetVersion() const { return m_props.version; }

private:
  static boost::shared_ptr<IAddon> FromProps(AddonProps addonProps);

  bool m_built;
  AddonProps m_props;
  cp_extension_t* m_extPoint;
};

};