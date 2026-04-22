/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingConditions.h"

#include "LockType.h"
#include "addons/AddonManager.h"
#include "addons/Skin.h"
#include "ServiceBroker.h"
#include "GUIPassword.h"
#include "profiles/ProfilesManager.h"
#include "settings/SettingAddon.h"
#include "settings/SettingsComponent.h"
#include "utils/FontUtils.h"
#include "utils/StringUtils.h"
#include "windowing/WinSystem.h"

namespace
{
bool AddonHasSettings(const std::string& condition,
                      const std::string& value,
                      const SettingConstPtr& setting,
                      void* data)
{
  if (setting == NULL)
    return false;

  boost::shared_ptr<const CSettingAddon> settingAddon = boost::dynamic_pointer_cast<const CSettingAddon>(setting);
  if (settingAddon == NULL)
    return false;

  ADDON::AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(settingAddon->GetValue(), addon,
                                              settingAddon->GetAddonType()) ||
      addon == NULL)
    return false;

  if (addon->Type() == ADDON::ADDON_SKIN)
    return ((ADDON::CSkinInfo*)addon.get())->HasSkinFile("SkinSettings.xml");

  return addon->HasSettings();
}

bool CheckMasterLock(const std::string& condition,
                     const std::string& value,
                     const SettingConstPtr& setting,
                     void* data)
{
  return g_passwordManager.IsMasterLockUnlocked(StringUtils::EqualsNoCase(value, "true"));
}

bool HasPeripherals(const std::string& condition,
                    const std::string& value,
                    const SettingConstPtr& setting,
                    void* data)
{
  return true;
}

bool IsMasterUser(const std::string& condition,
                  const std::string& value,
                  const SettingConstPtr& setting,
                  void* data)
{
  return g_passwordManager.bMasterUser;
}

bool ProfileCanWriteDatabase(const std::string& condition,
                             const std::string& value,
                             const SettingConstPtr& setting,
                             void* data)
{
  return CSettingConditions::GetCurrentProfile().canWriteDatabases();
}

bool ProfileCanWriteSources(const std::string& condition,
                            const std::string& value,
                            const SettingConstPtr& setting,
                            void* data)
{
  return CSettingConditions::GetCurrentProfile().canWriteSources();
}

bool ProfileHasAddons(const std::string& condition,
                      const std::string& value,
                      const SettingConstPtr& setting,
                      void* data)
{
  return CSettingConditions::GetCurrentProfile().hasAddons();
}

bool ProfileHasDatabase(const std::string& condition,
                        const std::string& value,
                        const SettingConstPtr& setting,
                        void* data)
{
  return CSettingConditions::GetCurrentProfile().hasDatabases();
}

bool ProfileHasSources(const std::string& condition,
                       const std::string& value,
                       const SettingConstPtr& setting,
                       void* data)
{
  return CSettingConditions::GetCurrentProfile().hasSources();
}

bool ProfileHasAddonManagerLocked(const std::string& condition,
                                  const std::string& value,
                                  const SettingConstPtr& setting,
                                  void* data)
{
  return CSettingConditions::GetCurrentProfile().addonmanagerLocked();
}

bool ProfileHasFilesLocked(const std::string& condition,
                           const std::string& value,
                           const SettingConstPtr& setting,
                           void* data)
{
  return CSettingConditions::GetCurrentProfile().filesLocked();
}

bool ProfileHasMusicLocked(const std::string& condition,
                           const std::string& value,
                           const SettingConstPtr& setting,
                           void* data)
{
  return CSettingConditions::GetCurrentProfile().musicLocked();
}

bool ProfileHasPicturesLocked(const std::string& condition,
                              const std::string& value,
                              const SettingConstPtr& setting,
                              void* data)
{
  return CSettingConditions::GetCurrentProfile().picturesLocked();
}

bool ProfileHasProgramsLocked(const std::string& condition,
                              const std::string& value,
                              const SettingConstPtr& setting,
                              void* data)
{
  return CSettingConditions::GetCurrentProfile().programsLocked();
}

bool ProfileHasSettingsLocked(const std::string& condition,
                              const std::string& value,
                              const SettingConstPtr& setting,
                              void* data)
{
  LOCK_LEVEL::SETTINGS_LOCK slValue=LOCK_LEVEL::ALL;
  if (StringUtils::EqualsNoCase(value, "none"))
    slValue = LOCK_LEVEL::NONE;
  else if (StringUtils::EqualsNoCase(value, "standard"))
    slValue = LOCK_LEVEL::STANDARD;
  else if (StringUtils::EqualsNoCase(value, "advanced"))
    slValue = LOCK_LEVEL::ADVANCED;
  else if (StringUtils::EqualsNoCase(value, "expert"))
    slValue = LOCK_LEVEL::EXPERT;
  return slValue <= CSettingConditions::GetCurrentProfile().settingsLockLevel();
}

bool ProfileHasVideosLocked(const std::string& condition,
                            const std::string& value,
                            const SettingConstPtr& setting,
                            void* data)
{
  return CSettingConditions::GetCurrentProfile().videoLocked();
}

bool ProfileLockMode(const std::string& condition,
                     const std::string& value,
                     const SettingConstPtr& setting,
                     void* data)
{
  char* tmp = NULL;
  LockType lock = (LockType)strtol(value.c_str(), &tmp, 0);
  if (tmp != NULL && *tmp != '\0')
    return false;

  return CSettingConditions::GetCurrentProfile().getLockMode() == lock;
}

bool GreaterThan(const std::string& condition,
                 const std::string& value,
                 const SettingConstPtr& setting,
                 void* data)
{
  if (setting == NULL)
    return false;

  boost::shared_ptr<const CSettingInt> settingInt = boost::dynamic_pointer_cast<const CSettingInt>(setting);
  if (settingInt == NULL)
    return false;

  char* tmp = NULL;

  int lhs = settingInt->GetValue();
  int rhs = StringUtils::IsInteger(value) ? (int)strtol(value.c_str(), &tmp, 0) : 0;

  return lhs > rhs;
}

bool GreaterThanOrEqual(const std::string& condition,
                        const std::string& value,
                        const SettingConstPtr& setting,
                        void* data)
{
  if (setting == NULL)
    return false;

  boost::shared_ptr<const CSettingInt> settingInt = boost::dynamic_pointer_cast<const CSettingInt>(setting);
  if (settingInt == NULL)
    return false;

  char* tmp = NULL;

  int lhs = settingInt->GetValue();
  int rhs = StringUtils::IsInteger(value) ? (int)strtol(value.c_str(), &tmp, 0) : 0;

  return lhs >= rhs;
}

bool LessThan(const std::string& condition,
              const std::string& value,
              const SettingConstPtr& setting,
              void* data)
{
  if (setting == NULL)
    return false;

  boost::shared_ptr<const CSettingInt> settingInt = boost::dynamic_pointer_cast<const CSettingInt>(setting);
  if (settingInt == NULL)
    return false;

  char* tmp = NULL;

  int lhs = settingInt->GetValue();
  int rhs = StringUtils::IsInteger(value) ? (int)strtol(value.c_str(), &tmp, 0) : 0;

  return lhs < rhs;
}

bool LessThanOrEqual(const std::string& condition,
                     const std::string& value,
                     const SettingConstPtr& setting,
                     void* data)
{
  if (setting == NULL)
    return false;

  boost::shared_ptr<const CSettingInt> settingInt = boost::dynamic_pointer_cast<const CSettingInt>(setting);
  if (settingInt == NULL)
    return false;

  char* tmp = NULL;

  int lhs = settingInt->GetValue();
  int rhs = StringUtils::IsInteger(value) ? (int)strtol(value.c_str(), &tmp, 0) : 0;

  return lhs <= rhs;
}
}; // anonymous namespace

const CProfilesManager* CSettingConditions::m_profileManager = NULL;
std::set<std::string> CSettingConditions::m_simpleConditions;
std::map<std::string, SettingConditionCheck> CSettingConditions::m_complexConditions;

void CSettingConditions::Initialize()
{
  if (!m_simpleConditions.empty())
    return;

  // add simple conditions
  m_simpleConditions.insert("true");
#ifdef HAS_UPNP
  m_simpleConditions.insert("has_upnp");
#endif
#ifdef HAS_TIME_SERVER
  m_simpleConditions.insert("has_time_server");
#endif
#ifdef HAS_WEB_SERVER
  m_simpleConditions.insert("has_web_server");
#endif
#ifdef HAS_FILESYSTEM_SMB
  m_simpleConditions.insert("has_filesystem_smb");
#endif
#if defined(_XBOX)
  m_simpleConditions.insert("has_dx");
#endif

  m_simpleConditions.insert("isstandalone");

#ifdef HAS_CDDA_RIPPER
  m_simpleConditions.insert("has_cdda_ripper");
#endif

#ifdef HAS_OPTICAL_DRIVE
  m_simpleConditions.insert("has_optical_drive");
#endif

  // add complex conditions
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("addonhassettings", AddonHasSettings));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("checkmasterlock", CheckMasterLock));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("hasperipherals", HasPeripherals));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("ismasteruser", IsMasterUser));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilecanwritedatabase", ProfileCanWriteDatabase));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilecanwritesources", ProfileCanWriteSources));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasaddons", ProfileHasAddons));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasdatabase", ProfileHasDatabase));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehassources", ProfileHasSources));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasaddonmanagerlocked", ProfileHasAddonManagerLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasfileslocked", ProfileHasFilesLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasmusiclocked", ProfileHasMusicLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehaspictureslocked", ProfileHasPicturesLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasprogramslocked", ProfileHasProgramsLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehassettingslocked", ProfileHasSettingsLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilehasvideoslocked", ProfileHasVideosLocked));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("profilelockmode", ProfileLockMode));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("gt", GreaterThan));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("gte", GreaterThanOrEqual));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("lt", LessThan));
  m_complexConditions.insert(std::pair<std::string, SettingConditionCheck>("lte", LessThanOrEqual));
}

void CSettingConditions::Deinitialize()
{
  m_profileManager = NULL;
}

const CProfile& CSettingConditions::GetCurrentProfile()
{
  if (!m_profileManager)
    m_profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager().get();

  if (m_profileManager)
    return m_profileManager->GetCurrentProfile();

  static CProfile emptyProfile;
  return emptyProfile;
}

bool CSettingConditions::Check(const std::string& condition,
                               const std::string& value /* = "" */,
                               const SettingConstPtr& setting /* = NULL */)
{
  if (m_simpleConditions.find(condition) != m_simpleConditions.end())
    return true;

  std::map<std::string, SettingConditionCheck>::const_iterator itCondition = m_complexConditions.find(condition);
  if (itCondition != m_complexConditions.end())
    return itCondition->second(condition, value, setting, NULL);

  return Check(condition);
}
