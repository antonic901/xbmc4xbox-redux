/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonRepos.h"

#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/AddonRepoInfo.h"
#include "addons/AddonSystemSettings.h"
#include "addons/Repository.h"
#include "addons/RepositoryUpdater.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/algorithm/cxx11/none_of.hpp>
#include <boost/bind.hpp>
#include <vector>

namespace
{
static const char *ALL_ADDON_IDS = "";
} // anonymous namespace

using namespace ADDON;

std::vector<ADDON::RepoInfo> LoadOfficialRepoInfos()
{
  std::vector<ADDON::RepoInfo> officialRepoInfos;
  ADDON::RepoInfo newRepoInfo;
  newRepoInfo.m_repoId = "repository.xbmc4xbox.org";
  newRepoInfo.m_origin = "https://github.com/xbmc4xbox";
  officialRepoInfos.push_back(newRepoInfo);

  return officialRepoInfos;
}
static std::vector<RepoInfo> officialRepoInfos = LoadOfficialRepoInfos();

/**********************************************************
 * CAddonRepos
 *
 */

CAddonRepos::CAddonRepos() : m_addonMgr(CServiceBroker::GetAddonMgr())
{
  m_valid = m_addonDb.Open() && LoadAddonsFromDatabase(ALL_ADDON_IDS, ADDON::AddonPtr());
}

CAddonRepos::CAddonRepos(const std::string& addonId) : m_addonMgr(CServiceBroker::GetAddonMgr())
{
  m_valid = m_addonDb.Open() && LoadAddonsFromDatabase(addonId, ADDON::AddonPtr());
}

CAddonRepos::CAddonRepos(const boost::shared_ptr<IAddon>& repoAddon)
  : m_addonMgr(CServiceBroker::GetAddonMgr())
{
  m_valid = m_addonDb.Open() && LoadAddonsFromDatabase(ALL_ADDON_IDS, repoAddon);
}

static bool comparator(const ADDON::RepoInfo &officialRepo, const boost::shared_ptr<IAddon>& addon, const CheckAddonPath::Type& checkAddonPath)
{
  if (checkAddonPath == CheckAddonPath::CHOICE_YES)
  {
    return (addon->Origin() == officialRepo.m_repoId &&
            StringUtils::StartsWithNoCase(addon->Path(), officialRepo.m_origin));
  }

  return addon->Origin() == officialRepo.m_repoId;
}

bool CAddonRepos::IsFromOfficialRepo(const boost::shared_ptr<IAddon>& addon,
                                     CheckAddonPath::Type checkAddonPath)
{
  return addon->Origin() == ORIGIN_SYSTEM ||
         boost::algorithm::any_of(officialRepoInfos.begin(), officialRepoInfos.end(), boost::bind(&comparator, _1, addon, checkAddonPath));
}

static bool isOfficialRepoComparator(const RepoInfo& officialRepo, const std::string& repoId) { return repoId == officialRepo.m_repoId; }

bool CAddonRepos::IsOfficialRepo(const std::string& repoId)
{
  return repoId == ORIGIN_SYSTEM || boost::algorithm::any_of(officialRepoInfos.begin(), officialRepoInfos.end(), boost::bind(&isOfficialRepoComparator, _1, repoId));
}

bool CAddonRepos::LoadAddonsFromDatabase(const std::string& addonId,
                                         const boost::shared_ptr<IAddon>& repoAddon)
{
  if (repoAddon != NULL)
  {
    if (!m_addonDb.GetRepositoryContent(repoAddon->ID(), m_allAddons))
    {
      // Repo content is invalid. Ask for update and wait.
      CServiceBroker::GetRepositoryUpdater().CheckForUpdates(
          boost::static_pointer_cast<CRepository>(repoAddon));
      CServiceBroker::GetRepositoryUpdater().Await();

      if (!m_addonDb.GetRepositoryContent(repoAddon->ID(), m_allAddons))
      {

        // could not connect to repository
        KODI::MESSAGING::HELPERS::ShowOKDialogText(repoAddon->Name(), 24991);
        return false;
      }
    }
  }
  else if (addonId == ALL_ADDON_IDS)
  {
    // load full repository content
    m_addonDb.GetRepositoryContent(m_allAddons);
    if (m_allAddons.empty())
      return true;
  }
  else
  {
    // load specific addonId only
    m_addonDb.FindByAddonId(addonId, m_allAddons);
  }

  if (m_allAddons.empty())
    return false;

  for (std::vector<boost::shared_ptr<IAddon> >::const_iterator addon = m_allAddons.begin(); addon != m_allAddons.end(); ++addon)
  {
    if (m_addonMgr.IsCompatible(*addon))
    {
      m_addonsByRepoMap[(*addon)->Origin()].insert(std::make_pair((*addon)->ID(), *addon));
    }
  }

  for (std::map<std::string, std::multimap<std::string, boost::shared_ptr<IAddon> > >::const_iterator repo = m_addonsByRepoMap.begin(); repo != m_addonsByRepoMap.end(); ++repo)
  {
    CLog::Log(LOGDEBUG, "%s - %u addon(s) loaded", repo->first.c_str(),
                static_cast<unsigned int>(repo->second.size()));

    const std::multimap<std::string, boost::shared_ptr<IAddon> >& addonsPerRepo = repo->second;

    for (std::multimap<std::string, boost::shared_ptr<IAddon> >::const_iterator addonMapEntry = addonsPerRepo.begin();
        addonMapEntry != addonsPerRepo.end(); ++addonMapEntry)
    {
      const boost::shared_ptr<IAddon>& addonToAdd = addonMapEntry->second;

      if (IsFromOfficialRepo(addonToAdd, CheckAddonPath::CHOICE_YES))
      {
        AddAddonIfLatest(addonToAdd, m_latestOfficialVersions);
      }
      else
      {
        AddAddonIfLatest(addonToAdd, m_latestPrivateVersions);
      }

      // add to latestVersionsByRepo
      AddAddonIfLatest(repo->first, addonToAdd, m_latestVersionsByRepo);
    }
  }

  return true;
}

void CAddonRepos::AddAddonIfLatest(const boost::shared_ptr<IAddon>& addonToAdd,
                                   std::map<std::string, boost::shared_ptr<IAddon> >& map) const
{
  const std::map<std::string, ADDON::AddonPtr>::iterator &latestKnown = map.find(addonToAdd->ID());
  if (latestKnown == map.end() || addonToAdd->Version() > latestKnown->second->Version())
    map[addonToAdd->ID()] = addonToAdd;
}

void CAddonRepos::AddAddonIfLatest(
    const std::string& repoId,
    const boost::shared_ptr<IAddon>& addonToAdd,
    std::map<std::string, std::map<std::string, boost::shared_ptr<IAddon> > >& map) const
{
  const std::map<std::string, std::map<std::string, ADDON::AddonPtr> >::iterator &latestVersionByRepo = map.find(repoId);

  if (latestVersionByRepo == map.end()) // repo not found
  {
    map[repoId].insert(std::make_pair(addonToAdd->ID(), addonToAdd));
  }
  else
  {
    const std::map<std::string, ADDON::AddonPtr> &latestVersionEntryByRepo = latestVersionByRepo->second;
    const std::map<std::string, ADDON::AddonPtr>::const_iterator &latestKnown = latestVersionEntryByRepo.find(addonToAdd->ID());

    if (latestKnown == latestVersionEntryByRepo.end() ||
        addonToAdd->Version() > latestKnown->second->Version())
      map[repoId][addonToAdd->ID()] = addonToAdd;
  }
}

void CAddonRepos::BuildUpdateOrOutdatedList(const std::vector<boost::shared_ptr<IAddon> >& installed,
                                            std::vector<boost::shared_ptr<IAddon> >& result,
                                            AddonCheckType::Type addonCheckType) const
{
  boost::shared_ptr<IAddon> update;

  CLog::Log(LOGDEBUG, "Building %s list from installed add-ons",
              addonCheckType == AddonCheckType::OUTDATED_ADDONS ? "outdated" : "update");
  for (std::vector<boost::shared_ptr<IAddon> >::const_iterator addon = installed.begin(); addon != installed.end(); ++addon)
  {
    if (DoAddonUpdateCheck(*addon, update))
    {
      result.push_back(addonCheckType == AddonCheckType::OUTDATED_ADDONS ? *addon : update);
    }
  }
}

void CAddonRepos::BuildAddonsWithUpdateList(
    const std::vector<boost::shared_ptr<IAddon> >& installed,
    std::map<std::string, AddonWithUpdate>& addonsWithUpdate) const
{
  boost::shared_ptr<IAddon> update;

  CLog::Log(LOGDEBUG,
              "Building combined addons-with-update map from installed add-ons");
  for (std::vector<boost::shared_ptr<IAddon> >::const_iterator addon = installed.begin(); addon != installed.end(); ++addon)
  {
    if (DoAddonUpdateCheck(*addon, update))
    {
      AddonWithUpdate addonUpdate = {*addon, update};
      addonsWithUpdate.insert(std::make_pair((*addon)->ID(), addonUpdate));
    }
  }
}

bool CAddonRepos::DoAddonUpdateCheck(const boost::shared_ptr<IAddon>& addon,
                                     boost::shared_ptr<IAddon>& update) const
{
  CLog::Log(LOGDEBUG, "update check: addonID = %s / Origin = %s / Version = %s",
              addon->ID().c_str(), addon->Origin().c_str(), addon->Version().asString().c_str());

  update.reset();

  const AddonRepoUpdateMode::Type updateMode =
      CAddonSystemSettings::GetInstance().GetAddonRepoUpdateMode();

  bool hasOfficialUpdate = FindAddonAndCheckForUpdate(addon, m_latestOfficialVersions, update);

  // addons with an empty origin have at least been checked against official repositories
  if (!addon->Origin().empty())
  {
    if (ORIGIN_SYSTEM != addon->Origin() && !hasOfficialUpdate) // not a system addon
    {

      // we didn't find an official update.
      // either version is current or that add-on isn't contained in official repos
      if (IsFromOfficialRepo(addon, CheckAddonPath::CHOICE_NO))
      {

        // check further if it IS contained in official repos
        if (updateMode == AddonRepoUpdateMode::ANY_REPOSITORY)
        {
          if (!FindAddonAndCheckForUpdate(addon, m_latestPrivateVersions, update))
          {
            return false;
          }
        }
      }
      else
      {
        // ...we check for updates in the origin repo only
        const std::map<std::string, std::map<std::string, ADDON::AddonPtr> >::const_iterator &repoEntry = m_latestVersionsByRepo.find(addon->Origin());
        if (repoEntry != m_latestVersionsByRepo.end())
        {
          if (!FindAddonAndCheckForUpdate(addon, repoEntry->second, update))
          {
            return false;
          }
        }
      }
    }
  }

  if (update != NULL)
  {
    CLog::Log(LOGDEBUG, "-- found -->: addonID = %s / Origin = %s / Version = %s",
                update->ID().c_str(), update->Origin().c_str(), update->Version().asString().c_str());
    return true;
  }

  return false;
}

bool CAddonRepos::FindAddonAndCheckForUpdate(
    const boost::shared_ptr<IAddon>& addonToCheck,
    const std::map<std::string, boost::shared_ptr<IAddon> >& map,
    boost::shared_ptr<IAddon>& update) const
{
  const std::map<std::string, ADDON::AddonPtr>::const_iterator &remote = map.find(addonToCheck->ID());
  if (remote != map.end()) // is addon in the desired map?
  {
    if ((remote->second->Version() > addonToCheck->Version()) ||
        m_addonMgr.IsAddonDisabledWithReason(addonToCheck->ID(), AddonDisabledReason::INCOMPATIBLE))
    {
      // return addon update
      update = remote->second;
      return true; // update found
    }
  }

  // either addon wasn't found or it's up to date
  return false;
}

bool CAddonRepos::GetLatestVersionByMap(const std::string& addonId,
                                        const std::map<std::string, boost::shared_ptr<IAddon> >& map,
                                        boost::shared_ptr<IAddon>& addon) const
{
  const std::map<std::string, ADDON::AddonPtr>::const_iterator &remote = map.find(addonId);
  if (remote != map.end()) // is addon in the desired map?
  {
    addon = remote->second;
    return true;
  }

  return false;
}

bool CAddonRepos::GetLatestAddonVersionFromAllRepos(const std::string& addonId,
                                                    boost::shared_ptr<IAddon>& addon) const
{
  const AddonRepoUpdateMode::Type updateMode =
      CAddonSystemSettings::GetInstance().GetAddonRepoUpdateMode();

  bool hasOfficialVersion = GetLatestVersionByMap(addonId, m_latestOfficialVersions, addon);

  if (hasOfficialVersion)
  {
    if (updateMode == AddonRepoUpdateMode::ANY_REPOSITORY)
    {
      boost::shared_ptr<IAddon> thirdPartyAddon;

      // only use this version if it's higher than the official one
      if (GetLatestVersionByMap(addonId, m_latestPrivateVersions, thirdPartyAddon))
      {
        if (thirdPartyAddon->Version() > addon->Version())
          addon = thirdPartyAddon;
      }
    }
  }
  else
  {
    if (!GetLatestVersionByMap(addonId, m_latestPrivateVersions, addon))
      return false;
  }

  return true;
}

void CAddonRepos::GetLatestAddonVersions(std::vector<boost::shared_ptr<IAddon> >& addonList) const
{
  const AddonRepoUpdateMode::Type updateMode =
      CAddonSystemSettings::GetInstance().GetAddonRepoUpdateMode();

  addonList.clear();

  // first we insert all official addon versions into the resulting vector

  for (std::map<std::string, boost::shared_ptr<IAddon> >::const_iterator officialVersion = m_latestOfficialVersions.begin(); officialVersion != m_latestOfficialVersions.end(); ++officialVersion)
    addonList.push_back(officialVersion->second);

  // then we insert private addon versions if they don't exist in the official map
  // or installation from ANY_REPOSITORY is allowed and the private version is higher

  for (std::map<std::string, boost::shared_ptr<IAddon> >::const_iterator privateVersion = m_latestPrivateVersions.begin(); privateVersion != m_latestPrivateVersions.end(); ++privateVersion)
  {
    std::map<std::string, ADDON::AddonPtr>::const_iterator &officialVersion = m_latestOfficialVersions.find(privateVersion->first);
    if (officialVersion == m_latestOfficialVersions.end() ||
        (updateMode == AddonRepoUpdateMode::ANY_REPOSITORY &&
        privateVersion->second->Version() > officialVersion->second->Version()))
    {
      addonList.push_back(privateVersion->second);
    }
  }
}

static bool getLatestAddonVersionsFromAllReposComparator(const ADDON::RepoInfo& officialRepo, const std::string& repoId) { return repoId == officialRepo.m_repoId; }

void CAddonRepos::GetLatestAddonVersionsFromAllRepos(
    std::vector<boost::shared_ptr<IAddon> >& addonList) const
{
  const AddonRepoUpdateMode::Type updateMode =
      CAddonSystemSettings::GetInstance().GetAddonRepoUpdateMode();

  addonList.clear();

  // first we insert all official addon versions into the resulting vector

  for (std::map<std::string, boost::shared_ptr<IAddon> >::const_iterator officialVersion = m_latestOfficialVersions.begin(); officialVersion != m_latestOfficialVersions.end(); ++officialVersion)
    addonList.push_back(officialVersion->second);

  // then we insert latest version per addon and repository if they don't exist in the official map
  // or installation from ANY_REPOSITORY is allowed and the private version is higher

  for (std::map<std::string, std::map<std::string, boost::shared_ptr<IAddon> > >::const_iterator repo = m_latestVersionsByRepo.begin(); repo != m_latestVersionsByRepo.end(); ++repo)
  {
    // content of official repos is stored in m_latestVersionsByRepo too
    // so we need to filter them out

    if (boost::algorithm::none_of(officialRepoInfos.begin(), officialRepoInfos.end(),
          boost::bind(&getLatestAddonVersionsFromAllReposComparator, _1, repo->first)))
    {
      for (std::map<std::string, boost::shared_ptr<IAddon> >::const_iterator latestAddon = repo->second.begin();
          latestAddon != repo->second.end(); ++latestAddon)
      {
        std::map<std::string, ADDON::AddonPtr>::const_iterator officialVersion = m_latestOfficialVersions.find(latestAddon->first);
        if (officialVersion == m_latestOfficialVersions.end() ||
            (updateMode == AddonRepoUpdateMode::ANY_REPOSITORY &&
            latestAddon->second->Version() > officialVersion->second->Version()))
        {
          addonList.push_back(latestAddon->second);
        }
      }
    }
  }
}

bool CAddonRepos::FindDependency(const std::string& dependsId,
                                 const std::string& parentRepoId,
                                 boost::shared_ptr<IAddon>& dependencyToInstall,
                                 boost::shared_ptr<CRepository>& repoForDep) const
{
  const AddonRepoUpdateMode::Type updateMode =
      CAddonSystemSettings::GetInstance().GetAddonRepoUpdateMode();

  bool dependencyHasOfficialVersion =
      GetLatestVersionByMap(dependsId, m_latestOfficialVersions, dependencyToInstall);

  if (dependencyHasOfficialVersion)
  {
    if (updateMode == AddonRepoUpdateMode::ANY_REPOSITORY)
    {
      boost::shared_ptr<IAddon> thirdPartyDependency;

      // only use this version if it's higher than the official one
      if (GetLatestVersionByMap(dependsId, m_latestPrivateVersions, thirdPartyDependency))
      {
        if (thirdPartyDependency->Version() > dependencyToInstall->Version())
          dependencyToInstall = thirdPartyDependency;
      }
    }
  }
  else
  {
    // If we didn't find an official version of this dependency
    // ...we check in the origin repo of the parent
    if (!FindDependencyByParentRepo(dependsId, parentRepoId, dependencyToInstall))
      return false;
  }

  // we got the dependency, so now get a repository-pointer to return

  boost::shared_ptr<IAddon> tmp;
  if (!m_addonMgr.GetAddon(dependencyToInstall->Origin(), tmp, AddonType::REPOSITORY,
                           OnlyEnabled::CHOICE_YES))
    return false;

  repoForDep = boost::static_pointer_cast<CRepository>(tmp);

  CLog::Log(LOGDEBUG, "found dependency [%s] for install/update from repo [%s]",
              dependencyToInstall->ID().c_str(), repoForDep->ID().c_str());

  if (dependencyToInstall->HasType(AddonType::REPOSITORY))
  {
    CLog::Log(LOGDEBUG,
                "dependency with id [{}] has type ADDON_REPOSITORY and will not install!",
                dependencyToInstall->ID());

    return false;
  }

  return true;
}

bool CAddonRepos::FindDependencyByParentRepo(const std::string& dependsId,
                                             const std::string& parentRepoId,
                                             boost::shared_ptr<IAddon>& dependencyToInstall) const
{
  const std::map<std::string, std::map<std::string, ADDON::AddonPtr> >::const_iterator &repoEntry = m_latestVersionsByRepo.find(parentRepoId);
  if (repoEntry != m_latestVersionsByRepo.end())
  {
    if (GetLatestVersionByMap(dependsId, repoEntry->second, dependencyToInstall))
      return true;
  }

  return false;
}

bool buildCompatibleVersionsListComparator(const ADDON::AddonPtr &a, const ADDON::AddonPtr &b) { return (a->Version() > b->Version()); }

void CAddonRepos::BuildCompatibleVersionsList(
    std::vector<boost::shared_ptr<IAddon> >& compatibleVersions) const
{
  std::vector<boost::shared_ptr<IAddon> > officialVersions;
  std::vector<boost::shared_ptr<IAddon> > privateVersions;

  for (std::vector<boost::shared_ptr<IAddon> >::const_iterator addon = m_allAddons.begin(); addon != m_allAddons.end(); ++addon)
  {
    if (m_addonMgr.IsCompatible(*addon))
    {
      if (IsFromOfficialRepo(*addon, CheckAddonPath::CHOICE_YES))
      {
        officialVersions.push_back(*addon);
      }
      else
      {
        privateVersions.push_back(*addon);
      }
    }
  }

  std::sort(officialVersions.begin(), officialVersions.end(), buildCompatibleVersionsListComparator);
  std::sort(privateVersions.begin(), privateVersions.end(), buildCompatibleVersionsListComparator);

  compatibleVersions = boost::move(officialVersions);
  boost::move(privateVersions.begin(), privateVersions.end(), std::back_inserter(compatibleVersions));
}
