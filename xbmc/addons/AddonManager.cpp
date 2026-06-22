/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonManager.h"

#include "FileItem.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "addons/AddonBuilder.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonEvents.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonRepos.h"
#include "addons/AddonSystemSettings.h"
#include "addons/AddonUpdateRules.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <boost/bind.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/algorithm/cxx11/copy_if.hpp>
#include <boost/algorithm/cxx11/none_of.hpp>
#include <boost/move/make_unique.hpp>
#include <set>
#include <utility>

using namespace XFILE;

namespace ADDON
{

/**********************************************************
 * CAddonMgr
 *
 */

  std::map<AddonType::Type, IAddonMgrCallback*> CAddonMgr::m_managers;

static bool LoadManifest(std::set<std::string>& system, std::set<std::string>& optional)
{
  CXBMCTinyXML doc;
  if (!doc.LoadFile("special://xbmc/system/addon-manifest.xml"))
  {
    CLog::Log(LOGERROR, "ADDONS: manifest missing");
    return false;
  }

  TiXmlElement *root = doc.RootElement();
  if (!root || root->ValueStr() != "addons")
  {
    CLog::Log(LOGERROR, "ADDONS: malformed manifest");
    return false;
  }

  TiXmlElement *elem = root->FirstChildElement("addon");
  while (elem)
  {
    if (elem->FirstChild())
    {
      if (XMLUtils::GetAttribute(elem, "optional") == "true")
        optional.insert(elem->FirstChild()->ValueStr());
      else
        system.insert(elem->FirstChild()->ValueStr());
    }
    elem = elem->NextSiblingElement("addon");
  }
  return true;
}

CAddonMgr::CAddonMgr()
  : m_database(boost::movelib::make_unique<CAddonDatabase>()),
    m_updateRules(boost::movelib::make_unique<CAddonUpdateRules>()),
    m_tempAddonBasePath("special://temp/addons")
{
}

CAddonMgr::~CAddonMgr()
{
  DeInit();
}

IAddonMgrCallback* CAddonMgr::GetCallbackForType(AddonType::Type type)
{
  if (m_managers.find(type) == m_managers.end())
    return NULL;
  else
    return m_managers[type];
}

bool CAddonMgr::RegisterAddonMgrCallback(AddonType::Type type, IAddonMgrCallback* cb)
{
  if (cb == NULL)
    return false;

  m_managers.erase(type);
  m_managers[type] = cb;

  return true;
}

void CAddonMgr::UnregisterAddonMgrCallback(AddonType::Type type)
{
  m_managers.erase(type);
}

bool CAddonMgr::Init()
{
  CSingleLock lock(m_critSection);

  if (!LoadManifest(m_systemAddons, m_optionalSystemAddons))
  {
    CLog::Log(LOGERROR, "ADDONS: Failed to read manifest");
    return false;
  }

  if (!m_database->Open())
    CLog::Log(LOGFATAL, "ADDONS: Failed to open database");

  FindAddons();

  //Ensure required add-ons are installed and enabled
  for (std::set<std::string>::const_iterator id = m_systemAddons.begin(); id != m_systemAddons.end(); ++id)
  {
    AddonPtr addon;
    if (!GetAddon(*id, addon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_YES))
    {
      CLog::Log(LOGFATAL, "addon '%s' not installed or not enabled.", id->c_str());
      return false;
    }
  }

  return true;
}

void CAddonMgr::DeInit()
{
  m_database->Close();

  /* If temporary directory was used from add-on, delete it */
  if (XFILE::CDirectory::Exists(m_tempAddonBasePath))
    XFILE::CDirectory::RemoveRecursive(CSpecialProtocol::TranslatePath(m_tempAddonBasePath));
}

bool CAddonMgr::HasAddons(AddonType::Type type)
{
  CSingleLock lock(m_critSection);

  for (ADDON_INFO_LIST::const_iterator addonInfo = m_installedAddons.begin(); addonInfo != m_installedAddons.end(); ++addonInfo)
  {
    if (addonInfo->second->HasType(type) && !IsAddonDisabled(addonInfo->second->ID()))
      return true;
  }
  return false;
}

bool CAddonMgr::HasInstalledAddons(AddonType::Type type)
{
  CSingleLock lock(m_critSection);

  for (ADDON_INFO_LIST::const_iterator addonInfo = m_installedAddons.begin(); addonInfo != m_installedAddons.end(); ++addonInfo)
  {
    if (addonInfo->second->HasType(type))
      return true;
  }
  return false;
}

void CAddonMgr::AddToUpdateableAddons(AddonPtr &pAddon)
{
  CSingleLock lock(m_critSection);
  m_updateableAddons.push_back(pAddon);
}

void CAddonMgr::RemoveFromUpdateableAddons(AddonPtr &pAddon)
{
  CSingleLock lock(m_critSection);
  VECADDONS::iterator it = std::find(m_updateableAddons.begin(), m_updateableAddons.end(), pAddon);

  if(it != m_updateableAddons.end())
  {
    m_updateableAddons.erase(it);
  }
}

struct AddonIdFinder
{
    explicit AddonIdFinder(const std::string& id)
      : m_id(id)
    {}

    bool operator()(const AddonPtr& addon)
    {
      return m_id == addon->ID();
    }
    private:
    std::string m_id;
};

bool CAddonMgr::ReloadSettings(const std::string& addonId, AddonInstanceId instanceId)
{
  CSingleLock lock(m_critSection);
  VECADDONS::iterator it =
      std::find_if(m_updateableAddons.begin(), m_updateableAddons.end(), AddonIdFinder(addonId));

  if( it != m_updateableAddons.end())
  {
    return (*it)->ReloadSettings(instanceId);
  }
  return false;
}

std::vector<boost::shared_ptr<IAddon> > CAddonMgr::GetAvailableUpdates() const
{
  std::vector<boost::shared_ptr<IAddon> > availableUpdates =
      GetAvailableUpdatesOrOutdatedAddons(AddonCheckType::AVAILABLE_UPDATES);

  CSingleLock lock(m_lastAvailableUpdatesCountMutex);
  m_lastAvailableUpdatesCountAsString = std::to_string(availableUpdates.size());

  return availableUpdates;
}

const std::string& CAddonMgr::GetLastAvailableUpdatesCountAsString() const
{
  CSingleLock lock(m_lastAvailableUpdatesCountMutex);
  return m_lastAvailableUpdatesCountAsString;
};

std::vector<boost::shared_ptr<IAddon> > CAddonMgr::GetOutdatedAddons() const
{
  return GetAvailableUpdatesOrOutdatedAddons(AddonCheckType::OUTDATED_ADDONS);
}

std::vector<boost::shared_ptr<IAddon> > CAddonMgr::GetAvailableUpdatesOrOutdatedAddons(
    AddonCheckType::Type addonCheckType) const
{
  unsigned int start = XbmcThreads::SystemClockMillis();

  std::vector<boost::shared_ptr<IAddon> > result;
  std::vector<boost::shared_ptr<IAddon> > installed;
  CAddonRepos addonRepos;

  if (addonRepos.IsValid())
  {
    GetAddonsForUpdate(installed);
    addonRepos.BuildUpdateOrOutdatedList(installed, result, addonCheckType);
  }

  CLog::Log(LOGDEBUG, "CAddonMgr::GetAvailableUpdatesOrOutdatedAddons took %u ms",
            XbmcThreads::SystemClockMillis() - start);

  return result;
}

std::map<std::string, AddonWithUpdate> CAddonMgr::GetAddonsWithAvailableUpdate() const
{
  CSingleLock lock(m_critSection);
  unsigned int start = XbmcThreads::SystemClockMillis();

  std::vector<boost::shared_ptr<IAddon> > installed;
  std::map<std::string, AddonWithUpdate> result;
  CAddonRepos addonRepos;

  if (addonRepos.IsValid())
  {
    GetAddonsForUpdate(installed);
    addonRepos.BuildAddonsWithUpdateList(installed, result);
  }

  CLog::Log(LOGDEBUG, "CAddonMgr::%s took %u ms", __FUNCTION__, XbmcThreads::SystemClockMillis() - start);

  return result;
}

std::vector<boost::shared_ptr<IAddon> > CAddonMgr::GetCompatibleVersions(
    const std::string& addonId) const
{
  CSingleLock lock(m_critSection);
  unsigned int start = XbmcThreads::SystemClockMillis();

  CAddonRepos addonRepos(addonId);
  std::vector<boost::shared_ptr<IAddon> > result;

  if (addonRepos.IsValid())
    addonRepos.BuildCompatibleVersionsList(result);

  CLog::Log(LOGDEBUG, "CAddonMgr::%s took %u ms", __FUNCTION__, XbmcThreads::SystemClockMillis() - start);

  return result;
}

bool CAddonMgr::HasAvailableUpdates()
{
  return !GetAvailableUpdates().empty();
}

std::vector<boost::shared_ptr<IAddon> > CAddonMgr::GetOrphanedDependencies() const
{
  std::vector<boost::shared_ptr<IAddon> > allAddons;
  GetAddonsInternal(AddonType::UNKNOWN, allAddons, OnlyEnabled::CHOICE_NO,
                    CheckIncompatible::CHOICE_YES);

  std::vector<boost::shared_ptr<IAddon> > orphanedDependencies;
  for (std::vector<boost::shared_ptr<IAddon> >::const_iterator addon = allAddons.begin(); addon != allAddons.end(); ++addon)
  {
    if (IsOrphaned(*addon, allAddons))
    {
      orphanedDependencies.push_back(*addon);
    }
  }

  return orphanedDependencies;
}

static bool isSameAddonID(const DependencyInfo& dep, const ADDON::AddonPtr &addon) { return dep.id == addon->ID(); }
static bool dependsOnCapturedAddon(const boost::shared_ptr<IAddon>& _, const ADDON::AddonPtr &addon)
{
  const std::vector<ADDON::DependencyInfo> &deps = _->GetDependencies();
  return boost::algorithm::any_of(deps.begin(), deps.end(), boost::bind(&isSameAddonID, _1, addon));
}

bool CAddonMgr::IsOrphaned(const boost::shared_ptr<IAddon>& addon,
                           const std::vector<boost::shared_ptr<IAddon> >& allAddons) const
{
  if (CServiceBroker::GetAddonMgr().IsSystemAddon(addon->ID()) ||
      !CAddonType::IsDependencyType(addon->MainType()))
    return false;

  return boost::algorithm::none_of(allAddons.begin(), allAddons.end(), boost::bind(&dependsOnCapturedAddon, _1, addon));
}

bool CAddonMgr::GetAddonsForUpdate(VECADDONS& addons) const
{
  return GetAddonsInternal(AddonType::UNKNOWN, addons, OnlyEnabled::CHOICE_YES,
                           CheckIncompatible::CHOICE_YES);
}

bool CAddonMgr::GetAddons(VECADDONS& addons) const
{
  return GetAddonsInternal(AddonType::UNKNOWN, addons, OnlyEnabled::CHOICE_YES,
                           CheckIncompatible::CHOICE_NO);
}

bool CAddonMgr::GetAddons(VECADDONS& addons, AddonType::Type type)
{
  return GetAddonsInternal(type, addons, OnlyEnabled::CHOICE_YES, CheckIncompatible::CHOICE_NO);
}

bool CAddonMgr::GetInstalledAddons(VECADDONS& addons)
{
  return GetAddonsInternal(AddonType::UNKNOWN, addons, OnlyEnabled::CHOICE_NO,
                           CheckIncompatible::CHOICE_NO);
}

bool CAddonMgr::GetInstalledAddons(VECADDONS& addons, AddonType::Type type)
{
  return GetAddonsInternal(type, addons, OnlyEnabled::CHOICE_NO, CheckIncompatible::CHOICE_NO);
}

bool CAddonMgr::GetDisabledAddons(VECADDONS& addons)
{
  return CAddonMgr::GetDisabledAddons(addons, AddonType::UNKNOWN);
}

static bool isAddonDisabled(const AddonPtr& addon, const CAddonMgr *manager) { return manager->IsAddonDisabled(addon->ID()); }

bool CAddonMgr::GetDisabledAddons(VECADDONS& addons, AddonType::Type type)
{
  VECADDONS all;
  if (GetInstalledAddons(all, type))
  {
    boost::algorithm::copy_if(all.begin(), all.end(), std::back_inserter(addons), boost::bind(&isAddonDisabled, _1, this));
    return true;
  }
  return false;
}

bool CAddonMgr::GetInstallableAddons(VECADDONS& addons)
{
  return GetInstallableAddons(addons, AddonType::UNKNOWN);
}

static bool isAddonInstalled(const AddonPtr& addon, const ADDON::AddonType::Type& type, CAddonMgr *manager)
{
  bool bErase = false;

  // check if the addon matches the provided addon type
  if (type != AddonType::UNKNOWN && addon->Type() != type && !addon->HasType(type))
    bErase = true;

  if (!manager->CanAddonBeInstalled(addon))
    bErase = true;

  return bErase;
}

bool CAddonMgr::GetInstallableAddons(VECADDONS& addons, AddonType::Type type)
{
  CSingleLock lock(m_critSection);
  CAddonRepos addonRepos;

  if (!addonRepos.IsValid())
    return false;

  // get all addons
  addonRepos.GetLatestAddonVersions(addons);

  // go through all addons and remove all that are already installed

  addons.erase(std::remove_if(addons.begin(), addons.end(), boost::bind(&isAddonInstalled, _1, type, this)), addons.end());

  return true;
}

bool CAddonMgr::FindInstallableById(const std::string& addonId, AddonPtr& result)
{
  CSingleLock lock(m_critSection);

  CAddonRepos addonRepos(addonId);
  if (!addonRepos.IsValid())
    return false;

  AddonPtr addonToUpdate;

  // check for an update if addon is installed already

  if (GetAddon(addonId, addonToUpdate, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO))
  {
    if (addonRepos.DoAddonUpdateCheck(addonToUpdate, result))
      return true;
  }

  // get the latest version from all repos if the
  // addon is up-to-date or not installed yet

  CLog::Log(
      LOGDEBUG,
      "addon {} is up-to-date or not installed. falling back to get latest version from all repos",
      addonId);

  return addonRepos.GetLatestAddonVersionFromAllRepos(addonId, result);
}

bool CAddonMgr::GetAddonsInternal(AddonType::Type type,
                                  VECADDONS& addons,
                                  OnlyEnabled::Type onlyEnabled,
                                  CheckIncompatible::Type checkIncompatible) const
{
  CSingleLock lock(m_critSection);

  for (ADDON_INFO_LIST::const_iterator addonInfo = m_installedAddons.begin(); addonInfo != m_installedAddons.end(); ++addonInfo)
  {
    if (type != AddonType::UNKNOWN && !addonInfo->second->HasType(type))
      continue;

    if (onlyEnabled == OnlyEnabled::CHOICE_YES &&
        ((checkIncompatible == CheckIncompatible::CHOICE_NO &&
          IsAddonDisabled(addonInfo->second->ID())) ||
        (checkIncompatible == CheckIncompatible::CHOICE_YES &&
          IsAddonDisabledExcept(addonInfo->second->ID(), AddonDisabledReason::INCOMPATIBLE))))
      continue;

    //FIXME: hack for skipping special dependency addons (xbmc.python etc.).
    //Will break if any extension point is added to them
    if (addonInfo->second->MainType() == AddonType::UNKNOWN)
      continue;

    AddonPtr addon = CAddonBuilder::Generate(addonInfo->second, type);
    if (addon)
    {
      // if the addon has a running instance, grab that
      AddonPtr runningAddon = addon->GetRunningInstance();
      if (runningAddon)
        addon = runningAddon;
      addons.push_back(boost::move(addon));
    }
  }
  return addons.size() > 0;
}

bool CAddonMgr::GetIncompatibleEnabledAddonInfos(std::vector<AddonInfoPtr>& incompatible) const
{
  return GetIncompatibleAddonInfos(incompatible, false);
}

static bool isCompatibleAddon(const AddonInfoPtr& a, const CAddonMgr *manager) { return manager->IsCompatible(a); }

bool CAddonMgr::GetIncompatibleAddonInfos(std::vector<AddonInfoPtr>& incompatible,
                                          bool includeDisabled) const
{
  GetAddonInfos(incompatible, true, AddonType::UNKNOWN);
  if (includeDisabled)
    GetDisabledAddonInfos(incompatible, AddonType::UNKNOWN, AddonDisabledReason::INCOMPATIBLE);
  incompatible.erase(std::remove_if(incompatible.begin(), incompatible.end(), boost::bind(&isCompatibleAddon, _1, this)),
                     incompatible.end());
  return !incompatible.empty();
}

std::vector<AddonInfoPtr> CAddonMgr::MigrateAddons()
{
  // install all addon updates
  CSingleLock lock(m_installAddonsMutex);
  CLog::Log(LOGINFO, "ADDON: waiting for add-ons to update...");
  VECADDONS updates;
  GetAddonUpdateCandidates(updates);
  InstallAddonUpdates(updates, true, AllowCheckForUpdates::CHOICE_NO);

  // get addons that became incompatible and disable them
  std::vector<AddonInfoPtr> incompatible;
  GetIncompatibleAddonInfos(incompatible, true);

  return DisableIncompatibleAddons(incompatible);
}

std::vector<AddonInfoPtr> CAddonMgr::DisableIncompatibleAddons(
    const std::vector<AddonInfoPtr>& incompatible)
{
  std::vector<AddonInfoPtr> changed;
  for (std::vector<AddonInfoPtr>::const_iterator addon = incompatible.begin(); addon != incompatible.end(); ++addon)
  {
    CLog::Log(LOGINFO, "ADDON: %s version %s is incompatible", (*addon)->ID().c_str(),
              (*addon)->Version().asString().c_str());

    if (!CAddonSystemSettings::GetInstance().UnsetActive(*addon))
    {
      CLog::Log(LOGWARNING, "ADDON: failed to unset %s", (*addon)->ID().c_str());
      continue;
    }
    if (!DisableAddon((*addon)->ID(), AddonDisabledReason::INCOMPATIBLE))
    {
      CLog::Log(LOGWARNING, "ADDON: failed to disable %s", (*addon)->ID().c_str());
    }

    changed.push_back(*addon);
  }

  return changed;
}

void CAddonMgr::CheckAndInstallAddonUpdates(bool wait) const
{
  CSingleLock lock(m_installAddonsMutex);
  VECADDONS updates;
  GetAddonUpdateCandidates(updates);
  InstallAddonUpdates(updates, wait, AllowCheckForUpdates::CHOICE_YES);
}

static bool isNotAutoUpdateable(const AddonPtr& addon, const ADDON::CAddonMgr *manager) { return !manager->IsAutoUpdateable(addon->ID()); }

bool CAddonMgr::GetAddonUpdateCandidates(VECADDONS& updates) const
{
  // Get Addons in need of an update and remove all the blacklisted ones
  updates = GetAvailableUpdates();
  updates.erase(
      std::remove_if(updates.begin(), updates.end(), boost::bind(&isNotAutoUpdateable, _1, this)),
      updates.end());
  return updates.empty();
}

void CAddonMgr::SortByDependencies(VECADDONS& updates) const
{
  std::vector<boost::shared_ptr<ADDON::IAddon> > sorted;
  while (!updates.empty())
  {
    for (ADDON::VECADDONS::iterator it = updates.begin(); it != updates.end();)
    {
      const ADDON::AddonPtr &addon = *it;

      const std::vector<ADDON::DependencyInfo> &dependencies = addon->GetDependencies();
      bool addToSortedList = true;
      // if the addon has dependencies we need to check for each dependency if it also has
      // an update to be installed (and in that case, if it is already in the sorted vector).
      // if all dependency match the said conditions, the addon doesn't depend on other addons
      // waiting to be updated. Hence, the addon being processed can be installed (i.e. added to
      // the end of the sorted vector of addon updates)
      for (std::vector<ADDON::DependencyInfo>::const_iterator dep = dependencies.begin(); dep != dependencies.end(); ++dep)
      {
        if ((boost::algorithm::any_of(updates.begin(), updates.end(), boost::bind(&isSameAddonID, *dep, _1))) &&
            (!boost::algorithm::any_of(sorted.begin(), sorted.end(), boost::bind(&isSameAddonID, *dep, _1))))
        {
          addToSortedList = false;
          break;
        }
      }

      // add to the end of sorted list of addons
      if (addToSortedList)
      {
        sorted.push_back(addon);
        it = updates.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }
  updates = sorted;
}

void CAddonMgr::InstallAddonUpdates(VECADDONS& updates,
                                    bool wait,
                                    AllowCheckForUpdates::Type allowCheckForUpdates) const
{
  // sort addons by dependencies (ensure install order) and install all
  SortByDependencies(updates);
  CAddonInstaller::GetInstance().InstallAddons(updates, wait, allowCheckForUpdates);
}

bool CAddonMgr::GetAddon(const std::string& str,
                         AddonPtr& addon,
                         AddonType::Type type,
                         OnlyEnabled::Type onlyEnabled) const
{
  CSingleLock lock(m_critSection);

  AddonInfoPtr addonInfo = GetAddonInfo(str, type);
  if (addonInfo)
  {
    addon = CAddonBuilder::Generate(addonInfo, type);
    if (addon)
    {
      if (onlyEnabled == OnlyEnabled::CHOICE_YES && IsAddonDisabled(addonInfo->ID()))
        return false;

      // if the addon has a running instance, grab that
      AddonPtr runningAddon = addon->GetRunningInstance();
      if (runningAddon)
        addon = runningAddon;
    }
    return NULL != addon.get();
  }

  return false;
}

bool CAddonMgr::GetAddon(const std::string& str, AddonPtr& addon, OnlyEnabled::Type onlyEnabled) const
{
  return GetAddon(str, addon, AddonType::UNKNOWN, onlyEnabled);
}

bool CAddonMgr::HasType(const std::string& id, AddonType::Type type)
{
  AddonPtr addon;
  return GetAddon(id, addon, type, OnlyEnabled::CHOICE_NO);
}

bool CAddonMgr::FindAddon(const std::string& addonId,
                          const std::string& origin,
                          const CAddonVersion& addonVersion)
{
  std::map<std::string, boost::shared_ptr<CAddonInfo> > installedAddons;

  FindAddons(installedAddons, "special://xbmcbin/addons");
  // Confirm special://xbmcbin/addons and special://xbmc/addons are not the same
  if (!CSpecialProtocol::ComparePath("special://xbmcbin/addons", "special://xbmc/addons"))
    FindAddons(installedAddons, "special://xbmc/addons");
  FindAddons(installedAddons, "special://home/addons");

  const ADDON::ADDON_INFO_LIST::iterator it = installedAddons.find(addonId);
  if (it == installedAddons.end() || it->second->Version() != addonVersion)
    return false;

  CSingleLock lock(m_critSection);

  m_database->GetInstallData(it->second);
  CLog::Log(LOGINFO, "CAddonMgr::%s: %s v%s installed", __FUNCTION__, addonId.c_str(),
            addonVersion.asString().c_str());

  m_installedAddons[addonId] = it->second; // insert/replace entry
  m_database->AddInstalledAddon(it->second, origin);

  // Reload caches
  std::map<std::string, AddonDisabledReason::Type> tmpDisabled;
  m_database->GetDisabled(tmpDisabled);
  m_disabled = boost::move(tmpDisabled);

  m_updateRules->RefreshRulesMap(*m_database);
  return true;
}

bool CAddonMgr::FindAddons()
{
  ADDON_INFO_LIST installedAddons;

  FindAddons(installedAddons, "special://xbmcbin/addons");
  // Confirm special://xbmcbin/addons and special://xbmc/addons are not the same
  if (!CSpecialProtocol::ComparePath("special://xbmcbin/addons", "special://xbmc/addons"))
    FindAddons(installedAddons, "special://xbmc/addons");
  FindAddons(installedAddons, "special://home/addons");

  std::set<std::string> installed;
  for (ADDON_INFO_LIST::const_iterator addon = installedAddons.begin(); addon != installedAddons.end(); ++addon)
    installed.insert(addon->second->ID());

  CSingleLock lock(m_critSection);

  // Sync with db
  m_database->SyncInstalled(installed, m_systemAddons, m_optionalSystemAddons);
  for (ADDON_INFO_LIST::const_iterator addon = installedAddons.begin(); addon != installedAddons.end(); ++addon)
  {
    m_database->GetInstallData(addon->second);
    CLog::Log(LOGINFO, "CAddonMgr::%s: %s v%s installed", __FUNCTION__, addon->second->ID().c_str(),
              addon->second->Version().asString().c_str());
  }

  m_installedAddons = boost::move(installedAddons);

  // Reload caches
  std::map<std::string, AddonDisabledReason::Type> tmpDisabled;
  m_database->GetDisabled(tmpDisabled);
  m_disabled = boost::move(tmpDisabled);

  m_updateRules->RefreshRulesMap(*m_database);

  return true;
}

bool CAddonMgr::UnloadAddon(const std::string& addonId)
{
  CSingleLock lock(m_critSection);

  if (!IsAddonInstalled(addonId))
    return true;

  AddonPtr localAddon;
  // can't unload an binary addon that is in use
  if (GetAddon(addonId, localAddon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO) &&
      localAddon->IsBinary() && localAddon->IsInUse())
  {
    CLog::Log(LOGERROR, "CAddonMgr::%s: could not unload binary add-on %s, as is in use", __FUNCTION__,
              addonId.c_str());
    return false;
  }

  m_installedAddons.erase(addonId);
  CLog::Log(LOGDEBUG, "CAddonMgr::%s: %s unloaded", __FUNCTION__, addonId.c_str());

  lock.unlock();
  AddonEvents::Unload event(addonId);
  m_unloadEvents.HandleEvent(event);

  return true;
}

bool CAddonMgr::LoadAddon(const std::string& addonId,
                          const std::string& origin,
                          const CAddonVersion& addonVersion)
{
  CSingleLock lock(m_critSection);

  AddonPtr addon;
  if (GetAddon(addonId, addon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO))
  {
    return true;
  }

  if (!FindAddon(addonId, origin, addonVersion))
  {
    CLog::Log(LOGERROR, "CAddonMgr: could not reload add-on %s. FindAddon failed.", addonId.c_str());
    return false;
  }

  if (!GetAddon(addonId, addon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO))
  {
    CLog::Log(LOGERROR, "CAddonMgr: could not load add-on %s. No add-on with that ID is installed.",
              addonId.c_str());
    return false;
  }

  lock.unlock();

  AddonEvents::Load event(addon->ID());
  m_unloadEvents.HandleEvent(event);

  if (IsAddonDisabled(addon->ID()))
  {
    EnableAddon(addon->ID());
    return true;
  }

  m_events.Publish(AddonEvents::ReInstalled(addon->ID()));
  CLog::Log(LOGDEBUG, "CAddonMgr: %s successfully loaded", addon->ID().c_str());
  return true;
}

void CAddonMgr::OnPostUnInstall(const std::string& id)
{
  CSingleLock lock(m_critSection);
  m_disabled.erase(id);
  RemoveAllUpdateRulesFromList(id);
  m_events.Publish(AddonEvents::UnInstalled(id));
}

void CAddonMgr::OnEventSubmit(const std::string& id, const CDateTime& time)
{
  {
    CSingleLock lock(m_critSection);
    m_database->SetLastUsed(id, time);
    ADDON::AddonInfoPtr addonInfo = GetAddonInfo(id, AddonType::UNKNOWN);
    if (addonInfo)
      addonInfo->SetLastUsed(time);
  }
  m_events.Publish(AddonEvents::MetadataChanged(id));
}

void CAddonMgr::UpdateLastUsed(const std::string& id)
{
  CDateTime time = CDateTime::GetCurrentDateTime();
  CServiceBroker::GetJobManager()->Submit(boost::bind(&CAddonMgr::OnEventSubmit, this, id, time));
}

static void ResolveDependencies(const std::string& addonId, std::vector<std::string>& needed, std::vector<std::string>& missing)
{
  if (std::find(needed.begin(), needed.end(), addonId) != needed.end())
    return;

  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(addonId, addon, AddonType::UNKNOWN,
                                              OnlyEnabled::CHOICE_NO))
    missing.push_back(addonId);
  else
  {
    needed.push_back(addonId);
    const std::vector<ADDON::DependencyInfo> &dependencies = addon->GetDependencies();
    for (std::vector<ADDON::DependencyInfo>::const_iterator dep = dependencies.begin(); dep != dependencies.end(); ++dep)
      if (!dep->optional)
        ResolveDependencies(dep->id, needed, missing);
  }
}

bool CAddonMgr::DisableAddon(const std::string& id, AddonDisabledReason::Type disabledReason)
{
  CSingleLock lock(m_critSection);
  if (!CanAddonBeDisabled(id))
    return false;
  if (m_disabled.find(id) != m_disabled.end())
    return true; //already disabled
  if (!m_database->DisableAddon(id, disabledReason))
    return false;
  if (!m_disabled.insert(std::make_pair(id, disabledReason)).second)
    return false;

  //success
  CLog::Log(LOGDEBUG, "CAddonMgr: %s disabled", id.c_str());
  AddonPtr addon;
  if (GetAddon(id, addon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO) && addon != NULL)
  {
  }

  m_events.Publish(AddonEvents::Disabled(id));
  return true;
}

bool CAddonMgr::UpdateDisabledReason(const std::string& id, AddonDisabledReason::Type newDisabledReason)
{
  CSingleLock lock(m_critSection);
  if (!IsAddonDisabled(id))
    return false;
  if (!m_database->DisableAddon(id, newDisabledReason))
    return false;

  m_disabled[id] = newDisabledReason;

  // success
  CLog::Log(LOGDEBUG, "CAddonMgr: DisabledReason for %s updated to %i", id.c_str(),
            static_cast<int>(newDisabledReason));
  return true;
}

bool CAddonMgr::EnableSingle(const std::string& id)
{
  CSingleLock lock(m_critSection);

  if (m_disabled.find(id) == m_disabled.end())
    return true; //already enabled

  AddonPtr addon;
  if (!GetAddon(id, addon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO) || addon == NULL)
    return false;

  if (!IsCompatible(addon))
  {
    CLog::Log(LOGERROR, "Add-on '%s' is not compatible with Kodi", addon->ID().c_str());
    UpdateDisabledReason(addon->ID(), AddonDisabledReason::INCOMPATIBLE);
    return false;
  }

  if (!m_database->EnableAddon(id))
    return false;
  m_disabled.erase(id);

  // If enabling a repo add-on without an origin, set its origin to its own id
  if (addon->HasType(AddonType::REPOSITORY) && addon->Origin().empty())
    SetAddonOrigin(id, id, false);

  CLog::Log(LOGDEBUG, "CAddonMgr: enabled %s", addon->ID().c_str());
  m_events.Publish(AddonEvents::Enabled(id));
  return true;
}

bool CAddonMgr::EnableAddon(const std::string& id)
{
  if (id.empty() || !IsAddonInstalled(id))
    return false;
  std::vector<std::string> needed;
  std::vector<std::string> missing;
  ResolveDependencies(id, needed, missing);
  for (std::vector<std::string>::const_iterator dep = missing.begin(); dep != missing.end(); ++dep)
    CLog::Log(LOGWARNING,
              "CAddonMgr: '{}' required by '{}' is missing. Add-on may not function "
              "correctly",
              *dep, id);
  for (std::reverse_iterator<std::vector<std::string>::iterator> it = needed.rbegin(); it != needed.rend(); ++it)
    EnableSingle(*it);

  return true;
}

bool CAddonMgr::IsAddonDisabled(const std::string& ID) const
{
  CSingleLock lock(m_critSection);
  return m_disabled.find(ID) != m_disabled.end();
}

bool CAddonMgr::IsAddonDisabledExcept(const std::string& ID,
                                      AddonDisabledReason::Type disabledReason) const
{
  CSingleLock lock(m_critSection);
  const std::map<std::string, ADDON::AddonDisabledReason::Type>::const_iterator disabledAddon = m_disabled.find(ID);
  return disabledAddon != m_disabled.end() && disabledAddon->second != disabledReason;
}

bool CAddonMgr::CanAddonBeDisabled(const std::string& ID)
{
  if (ID.empty())
    return false;

  CSingleLock lock(m_critSection);
  // Required system add-ons can not be disabled
  if (IsRequiredSystemAddon(ID))
    return false;

  AddonPtr localAddon;
  // can't disable an addon that isn't installed
  if (!GetAddon(ID, localAddon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO))
    return false;

  // can't disable an addon that is in use
  if (localAddon->IsInUse())
    return false;

  return true;
}

bool CAddonMgr::CanAddonBeEnabled(const std::string& id)
{
  return !id.empty() && IsAddonInstalled(id);
}

bool CAddonMgr::IsAddonInstalled(const std::string& ID)
{
  AddonPtr tmp;
  return GetAddon(ID, tmp, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO);
}

bool CAddonMgr::IsAddonInstalled(const std::string& ID, const std::string& origin) const
{
  AddonPtr tmp;

  if (GetAddon(ID, tmp, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO) && tmp)
  {
    if (tmp->Origin() == ORIGIN_SYSTEM)
    {
      return CAddonRepos::IsOfficialRepo(origin);
    }
    else
    {
      return tmp->Origin() == origin;
    }
  }
  return false;
}

bool CAddonMgr::IsAddonInstalled(const std::string& ID,
                                 const std::string& origin,
                                 const CAddonVersion& version)
{
  AddonPtr tmp;

  if (GetAddon(ID, tmp, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO) && tmp)
  {
    if (tmp->Origin() == ORIGIN_SYSTEM)
    {
      return CAddonRepos::IsOfficialRepo(origin) && tmp->Version() == version;
    }
    else
    {
      return tmp->Origin() == origin && tmp->Version() == version;
    }
  }
  return false;
}

bool CAddonMgr::CanAddonBeInstalled(const AddonPtr& addon)
{
  return addon != NULL && addon->LifecycleState() != AddonLifecycleState::BROKEN &&
         !IsAddonInstalled(addon->ID());
}

bool CAddonMgr::CanUninstall(const AddonPtr& addon)
{
  return addon && CanAddonBeDisabled(addon->ID()) && !IsBundledAddon(addon->ID());
}

bool CAddonMgr::IsBundledAddon(const std::string& id)
{
  return XFILE::CDirectory::Exists(
             CSpecialProtocol::TranslatePath("special://xbmc/addons/" + id + "/")) ||
         XFILE::CDirectory::Exists(
             CSpecialProtocol::TranslatePath("special://xbmcbin/addons/" + id + "/"));
}

bool CAddonMgr::IsSystemAddon(const std::string& id)
{
  return IsOptionalSystemAddon(id) || IsRequiredSystemAddon(id);
}

bool CAddonMgr::IsRequiredSystemAddon(const std::string& id)
{
  CSingleLock lock(m_critSection);
  return std::find(m_systemAddons.begin(), m_systemAddons.end(), id) != m_systemAddons.end();
}

bool CAddonMgr::IsOptionalSystemAddon(const std::string& id)
{
  CSingleLock lock(m_critSection);
  return std::find(m_optionalSystemAddons.begin(), m_optionalSystemAddons.end(), id) !=
         m_optionalSystemAddons.end();
}

bool CAddonMgr::LoadAddonDescription(const std::string &directory, AddonPtr &addon)
{
  ADDON::AddonInfoPtr addonInfo = CAddonInfoBuilder::Generate(directory);
  if (addonInfo)
    addon = CAddonBuilder::Generate(addonInfo, AddonType::UNKNOWN);

  return addon != NULL;
}

bool CAddonMgr::AddUpdateRuleToList(const std::string& id, AddonUpdateRule::Type updateRule)
{
  return m_updateRules->AddUpdateRuleToList(*m_database, id, updateRule);
}

bool CAddonMgr::RemoveAllUpdateRulesFromList(const std::string& id)
{
  return m_updateRules->RemoveAllUpdateRulesFromList(*m_database, id);
}

bool CAddonMgr::RemoveUpdateRuleFromList(const std::string& id, AddonUpdateRule::Type updateRule)
{
  return m_updateRules->RemoveUpdateRuleFromList(*m_database, id, updateRule);
}

bool CAddonMgr::IsAutoUpdateable(const std::string& id) const
{
  return m_updateRules->IsAutoUpdateable(id);
}

void CAddonMgr::PublishEventAutoUpdateStateChanged(const std::string& id)
{
  m_events.Publish(AddonEvents::AutoUpdateStateChanged(id));
}

void CAddonMgr::PublishInstanceAdded(const std::string& addonId, AddonInstanceId instanceId)
{
  m_events.Publish(AddonEvents::InstanceAdded(addonId, instanceId));
}

void CAddonMgr::PublishInstanceRemoved(const std::string& addonId, AddonInstanceId instanceId)
{
  m_events.Publish(AddonEvents::InstanceRemoved(addonId, instanceId));
}

bool CAddonMgr::IsCompatible(const boost::shared_ptr<const IAddon>& addon) const
{
  const std::vector<DependencyInfo> &dependencies = addon->GetDependencies();
  for (std::vector<ADDON::DependencyInfo>::const_iterator dependency = dependencies.begin(); dependency != dependencies.end(); ++dependency)
  {
    if (!dependency->optional)
    {
      // Intentionally only check the xbmc.* and kodi.* magic dependencies. Everything else will
      // not be missing anyway, unless addon was installed in an unsupported way.
      if (StringUtils::StartsWith(dependency->id, "xbmc.") ||
          StringUtils::StartsWith(dependency->id, "kodi."))
      {
        boost::shared_ptr<IAddon> dep;
        const bool haveDependency =
            GetAddon(dependency->id, dep, AddonType::UNKNOWN, OnlyEnabled::CHOICE_YES);
        if (!haveDependency || !dep->MeetsVersion(dependency->versionMin, dependency->version))
          return false;
      }
    }
  }
  return true;
}

bool CAddonMgr::IsCompatible(const AddonInfoPtr& addonInfo) const
{
  const std::vector<ADDON::DependencyInfo> &dependencies = addonInfo->GetDependencies();
  for (std::vector<ADDON::DependencyInfo>::const_iterator dependency = dependencies.begin(); dependency != dependencies.end(); ++dependency)
  {
    if (!dependency->optional)
    {
      // Intentionally only check the xbmc.* and kodi.* magic dependencies. Everything else will
      // not be missing anyway, unless addon was installed in an unsupported way.
      if (StringUtils::StartsWith(dependency->id, "xbmc.") ||
          StringUtils::StartsWith(dependency->id, "kodi."))
      {
        AddonInfoPtr addonInfo = GetAddonInfo(dependency->id, AddonType::UNKNOWN);
        if (!addonInfo || !addonInfo->MeetsVersion(dependency->versionMin, dependency->version))
          return false;
      }
    }
  }
  return true;
}

static bool isSameDependencyID(const DependencyInfo& d, const DependencyInfo& current_dep) { return d.id == current_dep.id; }

std::vector<DependencyInfo> CAddonMgr::GetDepsRecursive(const std::string& id,
                                                        OnlyEnabledRootAddon::Type onlyEnabledRootAddon)
{
  std::vector<DependencyInfo> added;
  AddonPtr root_addon;
  if (!FindInstallableById(id, root_addon) &&
      !GetAddon(id, root_addon, AddonType::UNKNOWN, static_cast<OnlyEnabled::Type>(onlyEnabledRootAddon)))
  {
    return added;
  }

  std::vector<DependencyInfo> toProcess;
  const std::vector<DependencyInfo> &dependencies = root_addon->GetDependencies();
  for (std::vector<ADDON::DependencyInfo>::const_iterator dep = dependencies.begin(); dep != dependencies.end(); ++dep)
    toProcess.push_back(*dep);

  while (!toProcess.empty())
  {
    ADDON::DependencyInfo current_dep = *toProcess.begin();
    toProcess.erase(toProcess.begin());
    if (StringUtils::StartsWith(current_dep.id, "xbmc.") ||
        StringUtils::StartsWith(current_dep.id, "kodi."))
      continue;

    std::vector<ADDON::DependencyInfo>::iterator added_it = std::find_if(added.begin(), added.end(), boost::bind(&isSameDependencyID, _1, current_dep));
    if (added_it != added.end())
    {
      if (current_dep.version < added_it->version)
        continue;

      bool aopt = added_it->optional;
      added.erase(added_it);
      added.push_back(current_dep);
      if (!current_dep.optional && aopt)
        continue;
    }
    else
      added.push_back(current_dep);

    AddonPtr current_addon;
    if (FindInstallableById(current_dep.id, current_addon))
    {
      const std::vector<DependencyInfo> &vecDependencies = current_addon->GetDependencies();
      for (std::vector<ADDON::DependencyInfo>::const_iterator item = vecDependencies.begin(); item != vecDependencies.end(); ++item)
        toProcess.push_back(*item);
    }
  }

  return added;
}

bool CAddonMgr::GetAddonInfos(AddonInfos& addonInfos, bool onlyEnabled, AddonType::Type type) const
{
  CSingleLock lock(m_critSection);

  bool forUnknown = type == AddonType::UNKNOWN;
  for (ADDON_INFO_LIST::const_iterator info = m_installedAddons.begin(); info != m_installedAddons.end(); ++info)
  {
    if (onlyEnabled && m_disabled.find(info->first) != m_disabled.end())
      continue;

    if (info->second->MainType() != AddonType::UNKNOWN && (forUnknown || info->second->HasType(type)))
      addonInfos.push_back(info->second);
  }

  return !addonInfos.empty();
}

static bool isSameType(AddonType::Type t, const std::pair<const std::string, ADDON::AddonInfoPtr> &info) { return info.second->HasType(t); }

std::vector<AddonInfoPtr> CAddonMgr::GetAddonInfos(bool onlyEnabled,
                                                   const std::vector<AddonType::Type>& types) const
{
  std::vector<AddonInfoPtr> infos;
  if (types.empty())
    return infos;

  CSingleLock lock(m_critSection);

  for (ADDON_INFO_LIST::const_iterator info = m_installedAddons.begin(); info != m_installedAddons.end(); ++info)
  {
    if (onlyEnabled && m_disabled.find(info->first) != m_disabled.end())
      continue;

    if (info->second->MainType() == AddonType::UNKNOWN)
      continue;

    const std::vector<ADDON::AddonType::Type>::const_iterator it = std::find_if(types.begin(), types.end(), boost::bind(&isSameType, _1, *info));
    if (it != types.end())
      infos.push_back(info->second);
  }

  return infos;
}

bool CAddonMgr::GetDisabledAddonInfos(std::vector<AddonInfoPtr>& addonInfos, AddonType::Type type) const
{
  return GetDisabledAddonInfos(addonInfos, type, AddonDisabledReason::NONE);
}

bool CAddonMgr::GetDisabledAddonInfos(std::vector<AddonInfoPtr>& addonInfos,
                                      AddonType::Type type,
                                      AddonDisabledReason::Type disabledReason) const
{
  CSingleLock lock(m_critSection);

  bool forUnknown = type == AddonType::UNKNOWN;
  for (ADDON_INFO_LIST::const_iterator info = m_installedAddons.begin(); info != m_installedAddons.end(); ++info)
  {
    const std::map<std::string, ADDON::AddonDisabledReason::Type>::const_iterator disabledAddon = m_disabled.find(info->first);
    if (disabledAddon == m_disabled.end())
      continue;

    if (info->second->MainType() != AddonType::UNKNOWN &&
        (forUnknown || info->second->HasType(type)) &&
        (disabledReason == AddonDisabledReason::NONE || disabledReason == disabledAddon->second))
      addonInfos.push_back(info->second);
  }

  return !addonInfos.empty();
}

const AddonInfoPtr CAddonMgr::GetAddonInfo(const std::string& id, AddonType::Type type) const
{
  CSingleLock lock(m_critSection);

  ADDON::ADDON_INFO_LIST::const_iterator addon = m_installedAddons.find(id);
  if (addon != m_installedAddons.end())
    if ((type == AddonType::UNKNOWN || addon->second->HasType(type)))
      return addon->second;

  return AddonInfoPtr();
}

void CAddonMgr::FindAddons(ADDON_INFO_LIST& addonmap, const std::string& path)
{
  CFileItemList items;
  if (XFILE::CDirectory::GetDirectory(path, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS))
  {
    for (int i = 0; i < items.Size(); ++i)
    {
      std::string path = items[i]->GetPath();
      if (CFileUtils::Exists(path + "addon.xml"))
      {
        AddonInfoPtr addonInfo = CAddonInfoBuilder::Generate(path);
        if (addonInfo)
        {
          const ADDON::ADDON_INFO_LIST::iterator &it = addonmap.find(addonInfo->ID());
          if (it != addonmap.end())
          {
            if (it->second->Version() > addonInfo->Version())
            {
              CLog::Log(LOGWARNING, "CAddonMgr::%s: Addon '%s' already present with higher version %s at '%s' - other version %s at '%s' will be ignored",
                           __FUNCTION__, addonInfo->ID().c_str(), it->second->Version().asString().c_str(), it->second->Path().c_str(), addonInfo->Version().asString().c_str(), addonInfo->Path().c_str());
              continue;
            }
            CLog::Log(LOGDEBUG, "CAddonMgr::%s: Addon '%s' already present with version %s at '%s' replaced with version %s at '%s'",
                         __FUNCTION__, addonInfo->ID().c_str(), it->second->Version().asString().c_str(), it->second->Path().c_str(), addonInfo->Version().asString().c_str(), addonInfo->Path().c_str());
          }

          addonmap[addonInfo->ID()] = addonInfo;
        }
      }
    }
  }
}

AddonOriginType::Type CAddonMgr::GetAddonOriginType(const AddonPtr& addon) const
{
  if (addon->Origin() == ORIGIN_SYSTEM)
    return AddonOriginType::SYSTEM;
  else if (!addon->Origin().empty())
    return AddonOriginType::REPOSITORY;
  else
    return AddonOriginType::MANUAL;
}

bool CAddonMgr::IsAddonDisabledWithReason(const std::string& ID,
                                          AddonDisabledReason::Type disabledReason) const
{
  CSingleLock lock(m_critSection);
  const std::map<std::string, ADDON::AddonDisabledReason::Type>::const_iterator &disabledAddon = m_disabled.find(ID);
  return disabledAddon != m_disabled.end() && disabledAddon->second == disabledReason;
}

/*!
 * @brief Addon update and install management.
 */
/*@{{{*/

bool CAddonMgr::SetAddonOrigin(const std::string& addonId, const std::string& repoAddonId, bool isUpdate)
{
  CSingleLock lock(m_critSection);

  m_database->SetOrigin(addonId, repoAddonId);
  if (isUpdate)
    m_database->SetLastUpdated(addonId, CDateTime::GetCurrentDateTime());

  // If available in manager update
  const AddonInfoPtr info = GetAddonInfo(addonId, AddonType::UNKNOWN);
  if (info)
    m_database->GetInstallData(info);
  return true;
}

bool CAddonMgr::AddonsFromRepoXML(const RepositoryDirInfo& repo,
                                  const std::string& xml,
                                  std::vector<AddonInfoPtr>& addons)
{
  CXBMCTinyXML doc;
  if (!doc.Parse(xml))
  {
    CLog::Log(LOGERROR, "CAddonMgr::%s: Failed to parse addons.xml", __FUNCTION__);
    return false;
  }

  if (doc.RootElement() == NULL || doc.RootElement()->ValueStr() != "addons")
  {
    CLog::Log(LOGERROR, "CAddonMgr::%s: Failed to parse addons.xml. Malformed", __FUNCTION__);
    return false;
  }

  // each addon XML should have a UTF-8 declaration
  TiXmlElement *element = doc.RootElement()->FirstChildElement("addon");
  while (element)
  {
    ADDON::AddonInfoPtr addonInfo = CAddonInfoBuilder::Generate(element, repo);
    if (addonInfo)
      addons.push_back(addonInfo);

    element = element->NextSiblingElement("addon");
  }

  return true;
}

/*@}}}*/

} /* namespace ADDON */
