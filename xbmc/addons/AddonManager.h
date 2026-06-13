/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/AddonInstaller.h" // AllowCheckForUpdates
#include "addons/addoninfo/AddonInfo.h" // AddonDisabledReason, AddonType, AddonUpdateRule, AddonOriginType
#include "threads/CriticalSection.h"
#include "utils/EventStream.h"

#include <stdint.h>
#include <map>
#include <boost/move/unique_ptr.hpp>
#include <set>
#include <string>
#include <vector>

namespace ADDON
{
class CAddonDatabase;
class CAddonUpdateRules;
class CAddonVersion;
class IAddonMgrCallback;

class CAddonInfo;
typedef boost::shared_ptr<CAddonInfo> AddonInfoPtr;
typedef std::map<std::string, AddonInfoPtr> ADDON_INFO_LIST;

class IAddon;
typedef boost::shared_ptr<IAddon> AddonPtr;
typedef std::vector<AddonPtr> VECADDONS;

struct AddonEvent;
struct AddonWithUpdate;
struct DependencyInfo;
struct RepositoryDirInfo;

typedef uint32_t AddonInstanceId;

namespace AddonCheckType
{
enum Type
{
  OUTDATED_ADDONS,
  AVAILABLE_UPDATES,
};
}

namespace OnlyEnabled
{
enum Type
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};
}

namespace OnlyEnabledRootAddon
{
enum Type
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};
}

namespace CheckIncompatible
{
enum Type
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};
}

/**
  * Class - CAddonMgr
  * Holds references to all addons, enabled or
  * otherwise. Services the generic callbacks available
  * to all addon variants.
  */
class CAddonMgr
{
public:
  bool ReInit()
  {
    DeInit();
    return Init();
  }
  bool Init();
  void DeInit();

  CAddonMgr();
  CAddonMgr(const CAddonMgr&);
  virtual ~CAddonMgr();

  CEventStream<AddonEvent>& Events() { return m_events; }
  CEventStream<AddonEvent>& UnloadEvents() { return m_unloadEvents; }

  IAddonMgrCallback* GetCallbackForType(AddonType::Type type);
  bool RegisterAddonMgrCallback(AddonType::Type type, IAddonMgrCallback* cb);
  void UnregisterAddonMgrCallback(AddonType::Type type);

  /*! \brief Retrieve a specific addon (of a specific type)
     \param id the id of the addon to retrieve.
     \param addon[out] the retrieved addon pointer - only use if the function returns true.
     \param type type of addon to retrieve - defaults to any type.
     \param onlyEnabled whether we only want enabled addons - set to false to allow both enabled and disabled addons - defaults to true.
     \return true if an addon matching the id of the given type is available and is enabled (if onlyEnabled is true).
     */
  bool GetAddon(const std::string& id,
                AddonPtr& addon,
                AddonType::Type type,
                OnlyEnabled::Type onlyEnabled) const;

  /*! \brief Retrieve a specific addon (of no specific type)
     \param id the id of the addon to retrieve.
     \param addon[out] the retrieved addon pointer - only use if the function returns true.
     \param onlyEnabled whether we only want enabled addons - set to false to allow both enabled and disabled addons - defaults to true.
     \return true if an addon matching the id of any type is available and is enabled (if onlyEnabled is true).
     */
  bool GetAddon(const std::string& id, AddonPtr& addon, OnlyEnabled::Type onlyEnabled) const;

  bool HasType(const std::string& id, AddonType::Type type);

  bool HasAddons(AddonType::Type type);

  bool HasInstalledAddons(AddonType::Type type);

  /*! Returns all installed, enabled and incompatible (and disabled) add-ons. */
  bool GetAddonsForUpdate(VECADDONS& addons) const;

  /*! Returns all installed, enabled add-ons. */
  bool GetAddons(VECADDONS& addons) const;

  /*! Returns enabled add-ons with given type. */
  bool GetAddons(VECADDONS& addons, AddonType::Type type);

  /*! Returns all installed, including disabled. */
  bool GetInstalledAddons(VECADDONS& addons);

  /*! Returns installed add-ons, including disabled, with given type. */
  bool GetInstalledAddons(VECADDONS& addons, AddonType::Type type);

  bool GetDisabledAddons(VECADDONS& addons);

  bool GetDisabledAddons(VECADDONS& addons, AddonType::Type type);

  /*! Get all installable addons */
  bool GetInstallableAddons(VECADDONS& addons);

  bool GetInstallableAddons(VECADDONS& addons, AddonType::Type type);

  /*! \brief Get the installable addon depending on install rules
     *         or fall back to highest version.
     * \note This function gets called in different contexts. If it's
     *       called for checking possible updates for already installed addons
     *       our update restriction rules apply.
     *       If it's called to (for example) populate an addon-select-dialog
     *       the addon is not installed yet, and we have to fall back to the
     *       highest version.
     * \param addonId addon to check for update or installation
     * \param addon[out] the retrieved addon pointer - only use if the function returns true.
     * \return true if an addon matching the id is available.
     */
  bool FindInstallableById(const std::string& addonId, AddonPtr& addon);

  void AddToUpdateableAddons(AddonPtr& pAddon);
  void RemoveFromUpdateableAddons(AddonPtr& pAddon);
  bool ReloadSettings(const std::string& addonId, AddonInstanceId instanceId);

  /*! Get addons with available updates */
  std::vector<boost::shared_ptr<IAddon> > GetAvailableUpdates() const;

  /*! Get addons that are outdated */
  std::vector<boost::shared_ptr<IAddon> > GetOutdatedAddons() const;

  /*! Returns true if there is any addon with available updates, otherwise false */
  bool HasAvailableUpdates();

  /*!
     * \brief Checks if the passed in addon is an orphaned dependency
     * \param addon the add-on/dependency to check
     * \param allAddons vector of all installed add-ons
     * \return true or false
     */
  bool IsOrphaned(const boost::shared_ptr<IAddon>& addon,
                  const std::vector<boost::shared_ptr<IAddon> >& allAddons) const;

  /*! \brief Checks for new / updated add-ons
     \return True if everything went ok, false otherwise
     */
  bool FindAddons();

  /*! \brief Checks whether given addon with given origin/version is installed
     * \param addonId addon to check
     * \param origin origin to check
     * \param addonVersion version to check
     * \return True if installed, false otherwise
     */
  bool FindAddon(const std::string& addonId,
                 const std::string& origin,
                 const CAddonVersion& addonVersion);

  /*!
     * @brief Fills the the provided vector with the list of incompatible
     * enabled addons and returns if there's any.
     *
     * @param[out] incompatible List of incompatible addons
     * @return true if there are incompatible addons
     */
  bool GetIncompatibleEnabledAddonInfos(std::vector<AddonInfoPtr>& incompatible) const;

  /*!
     * Migrate all the addons (updates all addons that have an update pending and disables those
     * that got incompatible)
     *
     * @return list of all addons (infos) that were modified.
     */
  std::vector<AddonInfoPtr> MigrateAddons();

  /*!
     * @brief Try to disable addons in the given list.
     *
     * @param[in] incompatible List of incompatible addon infos
     * @return list of all addon Infos that were disabled
     */
  std::vector<AddonInfoPtr> DisableIncompatibleAddons(
      const std::vector<AddonInfoPtr>& incompatible);

  /*!
     * Install available addon updates, if any.
     * @param wait If kodi should wait for all updates to download and install before returning
     */
  void CheckAndInstallAddonUpdates(bool wait) const;

  /*!
     * @note: should only be called by AddonInstaller
     *
     * Unload addon from the system. Returns true if it was unloaded, otherwise false.
     */
  bool UnloadAddon(const std::string& addonId);

  /*!
     * @note: should only be called by AddonInstaller
     *
     * Returns true if the addon was successfully loaded and enabled; otherwise false.
     */
  bool LoadAddon(const std::string& addonId,
                 const std::string& origin,
                 const CAddonVersion& addonVersion);

  /*! @note: should only be called by AddonInstaller
     *
     * Hook for clearing internal state after uninstall.
     */
  void OnPostUnInstall(const std::string& id);

  /*! \brief Disable an addon. Returns true on success, false on failure. */
  bool DisableAddon(const std::string& ID, AddonDisabledReason ::Type disabledReason);

  /*! \brief Updates reason for a disabled addon. Returns true on success, false on failure. */
  bool UpdateDisabledReason(const std::string& id, AddonDisabledReason ::Type newDisabledReason);

  /*! \brief Enable an addon. Returns true on success, false on failure. */
  bool EnableAddon(const std::string& ID);

  /* \brief Check whether an addon has been disabled via DisableAddon.
     In case the disabled cache does not know about the current state the database routine will be used.
     \param ID id of the addon
     \sa DisableAddon
     */
  bool IsAddonDisabled(const std::string& ID) const;

  /*!
     * @brief Check whether an addon has been disabled via DisableAddon except for a particular
     * reason In case the disabled cache does not know about the current state the database routine
     * will be used.
     * @param[in] ID id of the addon
     * @param[in] disabledReason the reason that will be an exception to being disabled
     * @return true if the addon was disabled except for the specified reason
     * @sa DisableAddon
     */
  bool IsAddonDisabledExcept(const std::string& ID, AddonDisabledReason ::Type disabledReason) const;

  /* \brief Checks whether an addon can be disabled via DisableAddon.
     \param ID id of the addon
     \sa DisableAddon
     */
  bool CanAddonBeDisabled(const std::string& ID);

  bool CanAddonBeEnabled(const std::string& id);

  /* \brief Checks whether an addon is installed.
     \param ID id of the addon
    */
  bool IsAddonInstalled(const std::string& ID);

  /* \brief Checks whether an addon is installed from a
     *        particular origin repo
     * \note if checked for an origin defined as official (i.e. repository.xbmc.org)
     *       this function will return true even if the addon is a shipped system add-on
     * \param ID id of the addon
     * \param origin origin repository id
     */
  bool IsAddonInstalled(const std::string& ID, const std::string& origin) const;

  /* \brief Checks whether an addon is installed from a
     *        particular origin repo and version
     * \note if checked for an origin defined as official (i.e. repository.xbmc.org)
     *       this function will return true even if the addon is a shipped system add-on
     * \param ID id of the addon
     * \param origin origin repository id
     * \param version the version of the addon
     */
  bool IsAddonInstalled(const std::string& ID,
                        const std::string& origin,
                        const CAddonVersion& version);

  /* \brief Checks whether an addon can be installed. Broken addons can't be installed.
    \param addon addon to be checked
    */
  bool CanAddonBeInstalled(const AddonPtr& addon);

  bool CanUninstall(const AddonPtr& addon);

  /*!
     * @brief Checks whether an addon is a bundled addon
     *
     * @param[in] id id of the addon
     * @return true if addon is bundled addon, false otherwise.
     */
  bool IsBundledAddon(const std::string& id);

  /*!
     * @brief Checks whether an addon is a system addon
     *
     * @param[in] id id of the addon
     * @return true if addon is system addon, false otherwise.
     */
  bool IsSystemAddon(const std::string& id);

  /*!
     * @brief Checks whether an addon is a required system addon
     *
     * @param[in] id id of the addon
     * @return true if addon is a required system addon, false otherwise.
     */
  bool IsRequiredSystemAddon(const std::string& id);

  /*!
     * @brief Checks whether an addon is an optional system addon
     *
     * @param[in] id id of the addon
     * @return true if addon is an optional system addon, false otherwise.
     */
  bool IsOptionalSystemAddon(const std::string& id);

  /*!
     * @brief Addon update rules.
     *
     * member functions for handling and querying add-on update rules
     *
     * @warning This should be never used from other places outside of addon
     * system directory.
     *
     */
  /*@{{{*/

  /* \brief Add a single update rule to the list for an addon
     * \sa CAddonUpdateRules::AddUpdateRuleToList()
     */
  bool AddUpdateRuleToList(const std::string& id, AddonUpdateRule::Type updateRule);

  /* \brief Remove all rules from update rules list for an addon
     * \sa CAddonUpdateRules::RemoveAllUpdateRulesFromList()
     */
  bool RemoveAllUpdateRulesFromList(const std::string& id);

  /* \brief Remove a specific rule from update rules list for an addon
     * \sa CAddonUpdateRules::RemoveUpdateRuleFromList()
     */
  bool RemoveUpdateRuleFromList(const std::string& id, AddonUpdateRule::Type updateRule);

  /* \brief Check if an addon version is auto-updateable
     * \param id addon id to be checked
     * \return true is addon is auto-updateable, false otherwise
     * \sa CAddonUpdateRules::IsAutoUpdateable()
     */
  bool IsAutoUpdateable(const std::string& id) const;

  /*@}}}*/

  /* \brief Launches event AddonEvent::AutoUpdateStateChanged
     * \param id addon id to pass through
     * \sa CGUIDialogAddonInfo::OnToggleAutoUpdates()
     */
  void PublishEventAutoUpdateStateChanged(const std::string& id);
  void UpdateLastUsed(const std::string& id);

  /*!
     * \brief Launches event @ref AddonEvent::InstanceAdded
     *
     * This is called when a new instance is added in add-on settings.
     *
     * \param[in] addonId Add-on id to pass through
     * \param[in] instanceId Identifier of the add-on instance
     */
  void PublishInstanceAdded(const std::string& addonId, AddonInstanceId instanceId);

  /*!
     * \brief Launches event @ref AddonEvent::InstanceRemoved
     *
     * This is called when an instance is removed in add-on settings.
     *
     * \param[in] addonId Add-on id to pass through
     * \param[in] instanceId Identifier of the add-on instance
     */
  void PublishInstanceRemoved(const std::string& addonId, AddonInstanceId instanceId);

  /*! \brief Load the addon in the given path
     This loads the addon using c-pluff which parses the addon descriptor file.
     \param path folder that contains the addon.
     \param addon [out] returned addon.
     \return true if addon is set, false otherwise.
     */
  bool LoadAddonDescription(const std::string& path, AddonPtr& addon);

  bool ServicesHasStarted() const;

  /*!
     * @brief Check if given addon is compatible with Kodi.
     *
     * @param[in] addon Addon to check
     * @return true if compatible, false if not
     */
  bool IsCompatible(const boost::shared_ptr<const IAddon>& addon) const;

  /*!
     * @brief Check given addon information is compatible with Kodi.
     *
     * @param[in] addonInfo Addon information to check
     * @return true if compatible, false if not
     */
  bool IsCompatible(const AddonInfoPtr& addonInfo) const;

  /*! \brief Recursively get dependencies for an add-on
     *  \param id the id of the root addon
     *  \param onlyEnabledRootAddon whether look for enabled root add-ons only
     */
  std::vector<DependencyInfo> GetDepsRecursive(const std::string& id,
                                               OnlyEnabledRootAddon::Type onlyEnabledRootAddon);

  /*!
     * @brief Get a list of add-on's with info's for the on system available
     * ones.
     *
     * @param[out] addonInfos list where finded addon information becomes stored
     * @param[in] onlyEnabled If true are only enabled ones given back,
     *                        if false all on system available. Default is true.
     * @param[in] type The requested type, with "ADDON_UNKNOWN" are all add-on
     *                 types given back who match the case with value before.
     *                 If a type id becomes added are only add-ons returned who
     *                 match them. Default is for all types.
     * @return true if the list contains entries
     */
  bool GetAddonInfos(std::vector<AddonInfoPtr>& addonInfos, bool onlyEnabled, AddonType::Type type) const;

  /*!
     * @brief Get a list of add-on's with info's for the on system available
     * ones.
     *
     * @param[in] onlyEnabled If true are only enabled ones given back,
     *                        if false all on system available. Default is true.
     * @param[in] types List about requested types.
     * @return List where finded addon information becomes returned.
     *
     * @note @ref ADDON_UNKNOWN should not used for here!
     */
  std::vector<AddonInfoPtr> GetAddonInfos(bool onlyEnabled,
                                          const std::vector<AddonType::Type>& types) const;

  /*!
     * @brief Get a list of disabled add-on's with info's
     *
     * @param[out] addonInfos list where finded addon information becomes stored
     * @param[in] type        The requested type, with "ADDON_UNKNOWN"
     *                        are all add-on types given back who match the case
     *                        with value before.
     *                        If a type id becomes added are only add-ons
     *                        returned who match them. Default is for all types.
     * @return true if the list contains entries
     */
  bool GetDisabledAddonInfos(std::vector<AddonInfoPtr>& addonInfos, AddonType::Type type) const;

  /*!
     * @brief Get a list of disabled add-on's with info's for the on system
     * available ones with a specific disabled reason.
     *
     * @param[out] addonInfos list where finded addon information becomes stored
     * @param[in] type        The requested type, with "ADDON_UNKNOWN"
     *                        are all add-on types given back who match the case
     *                        with value before.
     *                        If a type id becomes added are only add-ons
     *                        returned who match them. Default is for all types.
     * @param[in] disabledReason To get all disabled addons use the value
     *                           "AddonDiasbledReason::NONE". If any other value
     *                           is supplied only addons with that reason will be
     *                           returned.
     * @return true if the list contains entries
     */
  bool GetDisabledAddonInfos(std::vector<AddonInfoPtr>& addonInfos,
                             AddonType::Type type,
                             AddonDisabledReason ::Type disabledReason) const;

  const AddonInfoPtr GetAddonInfo(const std::string& id, AddonType::Type type) const;

  /*!
     * @brief Get the path where temporary add-on files are stored
     *
     * @return the base path used for temporary addon paths
     *
     * @warning the folder and its contents are deleted when Kodi is closed
     */
  const std::string& GetTempAddonBasePath() { return m_tempAddonBasePath; }

  AddonOriginType::Type GetAddonOriginType(const AddonPtr& addon) const;

  /*!
     * \brief Check whether an addon has been disabled with a special reason.
     * \param ID id of the addon
     * \param disabledReason reason we want to check for (NONE, USER, INCOMPATIBLE, PERMANENT_FAILURE)
     * \return true or false
     */
  bool IsAddonDisabledWithReason(const std::string& ID, AddonDisabledReason ::Type disabledReason) const;

  /*!
     * @brief Addon update and install management.
     *
     * Parts inside here are used for changes about addon system.
     *
     * @warning This should be never used from other places outside of addon
     * system directory.
     */
  /*@{{{*/

  /*!
     * @brief Update addon origin data.
     *
     * This becomes called from @ref CAddonInstallJob to set the source repo and
     * if update, to set also the date.
     *
     * @note This must be called after the addon manager has inserted a new addon
     * with @ref FindAddons() into database.
     *
     * @param[in] addonId Identifier of addon
     * @param[in] repoAddonId Identifier of related repository addon
     * @param[in] isUpdate If call becomes done on already installed addon and
     *                     update only.
     * @return True if successfully done, otherwise false
     *
     * Currently listed call sources:
     * - @ref CAddonInstallJob::DoWork
     */
  bool SetAddonOrigin(const std::string& addonId, const std::string& repoAddonId, bool isUpdate);

  /*!
     * @brief Parse a repository XML file for addons and load their descriptors.
     *
     * A repository XML is essentially a concatenated list of addon descriptors.
     *
     * @param[in] repo The repository info.
     * @param[in] xml The XML document from repository.
     * @param[out] addons returned list of addons.
     * @return true if the repository XML file is parsed, false otherwise.
     *
     * Currently listed call sources:
     * - @ref CRepository::FetchIndex
     */
  bool AddonsFromRepoXML(const RepositoryDirInfo& repo,
                         const std::string& xml,
                         std::vector<AddonInfoPtr>& addons);

  /*@}}}*/

  /*!
     * \brief Retrieves list of outdated addons as well as their related
     *        available updates and stores them into map.
     * \return map of outdated addons with their update
     */
  std::map<std::string, AddonWithUpdate> GetAddonsWithAvailableUpdate() const;

  /*!
     * \brief Retrieves list of compatible addon versions of all origins
     * \param[in] addonId addon to look up
     * \return vector containing compatible addon versions
     */
  std::vector<boost::shared_ptr<IAddon> > GetCompatibleVersions(const std::string& addonId) const;

  /*!
     * \brief Return number of available updates formatted as string
     *        this can be used as a lightweight method of retrieving the number of updates
     *        rather than using the expensive GetAvailableUpdates call
     * \return number of available updates
     */
  const std::string& GetLastAvailableUpdatesCountAsString() const;

  /*!
     * \brief returns a vector with all found orphaned dependencies.
     * \return the vector
     */
  std::vector<boost::shared_ptr<IAddon> > GetOrphanedDependencies() const;

private:
  // Used inside UpdateLastUsed to asynchronously add a job to Job Manager
  void OnEventSubmit(const std::string& id, const CDateTime& time);

  CAddonMgr& operator=(CAddonMgr const&);

  VECADDONS m_updateableAddons;

  /*!
     * \brief returns a vector with either available updates or outdated addons.
     *        usually called by its wrappers GetAvailableUpdates() or
     *        GetOutdatedAddons()
     * \param[in] true to return outdated addons, false to return available updates
     * \return vector filled with either available updates or outdated addons
     */
  std::vector<boost::shared_ptr<IAddon> > GetAvailableUpdatesOrOutdatedAddons(
      AddonCheckType::Type addonCheckType) const;

  bool GetAddonsInternal(AddonType::Type type,
                         VECADDONS& addons,
                         OnlyEnabled::Type onlyEnabled,
                         CheckIncompatible::Type checkIncompatible) const;

  bool EnableSingle(const std::string& id);

  void FindAddons(ADDON_INFO_LIST& addonmap, const std::string& path);

  /*!
     * @brief Fills the the provided vector with the list of incompatible
     * addons and returns if there's any.
     *
     * @param[out] incompatible List of incompatible addons
     * @param[in] whether or not to include incompatible addons that are disabled
     * @return true if there are incompatible addons
     */
  bool GetIncompatibleAddonInfos(std::vector<AddonInfoPtr>& incompatible,
                                 bool includeDisabled) const;

  /*!
     * Get the list of of available updates
     * \param[in,out] updates the vector of addons to be filled with addons that need to be updated (not blacklisted)
     * \return if there are any addons needing updates
     */
  bool GetAddonUpdateCandidates(VECADDONS& updates) const;

  /*!\brief Sort a list of addons for installation, i.e., defines the order of installation depending
     * of each addon dependencies.
     * \param[in,out] updates the vector of addons to sort
     */
  void SortByDependencies(VECADDONS& updates) const;

  /*!
     * Install the list of addon updates via AddonInstaller
     * \param[in,out] updates the vector of addons to install (will be sorted)
     * \param wait if the process should wait for all addons to install
     * \param allowCheckForUpdates indicates if content update checks are allowed
     *        after installation of a repository addon from the list
     */
  void InstallAddonUpdates(VECADDONS& updates,
                           bool wait,
                           AllowCheckForUpdates::Type allowCheckForUpdates) const;

  // This guards the addon installation process to make sure
  // addon updates are not installed concurrently
  // while the migration is running. Addon updates can be triggered
  // as a result of a repository update event.
  // (migration will install any available update anyway)
  mutable CCriticalSection m_installAddonsMutex;

  std::map<std::string, AddonDisabledReason::Type> m_disabled;
  static std::map<AddonType::Type, IAddonMgrCallback*> m_managers;
  mutable CCriticalSection m_critSection;
  boost::movelib::unique_ptr<CAddonDatabase> m_database;
  boost::movelib::unique_ptr<CAddonUpdateRules> m_updateRules;
  CEventSource<AddonEvent> m_events;
  CBlockingEventSource<AddonEvent> m_unloadEvents;
  std::set<std::string> m_systemAddons;
  std::set<std::string> m_optionalSystemAddons;
  ADDON_INFO_LIST m_installedAddons;

  // Temporary path given to add-ons, whose content is deleted when Kodi is stopped
  const std::string m_tempAddonBasePath;

  /*!
     * latest count of available updates
     */
  mutable std::string m_lastAvailableUpdatesCountAsString;
  mutable CCriticalSection m_lastAvailableUpdatesCountMutex;
};

}; /* namespace ADDON */
