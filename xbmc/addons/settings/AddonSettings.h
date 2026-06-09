/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "settings/SettingControl.h"
#include "settings/SettingCreator.h"
#include "settings/SettingsBase.h"
#include "settings/lib/ISettingCallback.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

enum class SettingDependencyOperator;

class CSettingCategory;
class CSettingGroup;
class CSettingDependency;
class CXBMCTinyXML;

struct StringSettingOption;

namespace ADDON
{

class IAddon;
class IAddonInstanceHandler;

class CAddonSettings : public CSettingControlCreator,
                       public CSettingCreator,
                       public CSettingsBase,
                       public ISettingCallback
{
public:
  CAddonSettings(const boost::shared_ptr<IAddon>& addon, AddonInstanceId instanceId);
  virtual ~CAddonSettings() {}

  // specialization of CSettingsBase
  virtual bool Initialize() { return false; }

  // implementations of CSettingsBase
  virtual bool Load() { return false; }
  virtual bool Save();

  // specialization of CSettingCreator
  boost::shared_ptr<CSetting> CreateSetting(
      const std::string& settingType,
      const std::string& settingId,
      virtual CSettingsManager* settingsManager = nullptr) const;

  // implementation of ISettingCallback
  virtual void OnSettingAction(const boost::shared_ptr<const CSetting>& setting);

  const std::string& GetAddonId() const { return m_addonId; }

  bool Initialize(const CXBMCTinyXML& doc, bool allowEmpty = false);
  bool Load(const CXBMCTinyXML& doc);
  bool Save(CXBMCTinyXML& doc) const;

  bool HasSettings() const;

  std::string GetSettingLabel(int label) const;

  boost::shared_ptr<CSetting> AddSetting(const std::string& settingId, bool value);
  boost::shared_ptr<CSetting> AddSetting(const std::string& settingId, int value);
  boost::shared_ptr<CSetting> AddSetting(const std::string& settingId, double value);
  boost::shared_ptr<CSetting> AddSetting(const std::string& settingId, const std::string& value);

protected:
  // specializations of CSettingsBase
  virtual void InitializeSettingTypes();
  virtual void InitializeControls();
  virtual void InitializeConditions();

  // implementation of CSettingsBase
  virtual bool InitializeDefinitions() { return false; }

private:
  bool AddInstanceSettings();
  bool InitializeDefinitions(const CXBMCTinyXML& doc);

  bool ParseSettingVersion(const CXBMCTinyXML& doc, uint32_t& version) const;

  boost::shared_ptr<CSettingGroup> ParseOldSettingElement(
      const TiXmlElement* categoryElement,
      const boost::shared_ptr<CSettingCategory>& category,
      std::set<std::string>& settingIds);

  boost::shared_ptr<CSettingCategory> ParseOldCategoryElement(uint32_t& categoryId,
                                                            const TiXmlElement* categoryElement,
                                                            std::set<std::string>& settingIds);

  bool InitializeFromOldSettingDefinitions(const CXBMCTinyXML& doc);
  boost::shared_ptr<CSetting> InitializeFromOldSettingAction(const std::string& settingId,
                                                           const TiXmlElement* settingElement,
                                                           const std::string& defaultValue);
  boost::shared_ptr<CSetting> InitializeFromOldSettingLabel();
  boost::shared_ptr<CSetting> InitializeFromOldSettingBool(const std::string& settingId,
                                                         const TiXmlElement* settingElement,
                                                         const std::string& defaultValue);
  boost::shared_ptr<CSetting> InitializeFromOldSettingTextIpAddress(
      const std::string& settingId,
      const std::string& settingType,
      const TiXmlElement* settingElement,
      const std::string& defaultValue,
      const int settingLabel);
  boost::shared_ptr<CSetting> InitializeFromOldSettingNumber(const std::string& settingId,
                                                           const TiXmlElement* settingElement,
                                                           const std::string& defaultValue,
                                                           const int settingLabel);
  boost::shared_ptr<CSetting> InitializeFromOldSettingPath(const std::string& settingId,
                                                         const std::string& settingType,
                                                         const TiXmlElement* settingElement,
                                                         const std::string& defaultValue,
                                                         const int settingLabel);
  boost::shared_ptr<CSetting> InitializeFromOldSettingDate(const std::string& settingId,
                                                         const TiXmlElement* settingElement,
                                                         const std::string& defaultValue,
                                                         const int settingLabel);
  boost::shared_ptr<CSetting> InitializeFromOldSettingTime(const std::string& settingId,
                                                         const TiXmlElement* settingElement,
                                                         const std::string& defaultValue,
                                                         const int settingLabel);
  boost::shared_ptr<CSetting> InitializeFromOldSettingSelect(
      const std::string& settingId,
      const TiXmlElement* settingElement,
      const std::string& defaultValue,
      const int settingLabel,
      const std::string& settingValues,
      const std::vector<std::string>& settingLValues);
  boost::shared_ptr<CSetting> InitializeFromOldSettingAddon(const std::string& settingId,
                                                          const TiXmlElement* settingElement,
                                                          const std::string& defaultValue,
                                                          const int settingLabel);
  boost::shared_ptr<CSetting> InitializeFromOldSettingEnums(
      const std::string& settingId,
      const std::string& settingType,
      const TiXmlElement* settingElement,
      const std::string& defaultValue,
      const std::string& settingValues,
      const std::vector<std::string>& settingLValues);
  boost::shared_ptr<CSetting> InitializeFromOldSettingFileEnum(const std::string& settingId,
                                                             const TiXmlElement* settingElement,
                                                             const std::string& defaultValue,
                                                             const std::string& settingValues);
  boost::shared_ptr<CSetting> InitializeFromOldSettingRangeOfNum(const std::string& settingId,
                                                               const TiXmlElement* settingElement,
                                                               const std::string& defaultValue);
  boost::shared_ptr<CSetting> InitializeFromOldSettingSlider(const std::string& settingId,
                                                           const TiXmlElement* settingElement,
                                                           const std::string& defaultValue);
  boost::shared_ptr<CSetting> InitializeFromOldSettingFileWithSource(
      const std::string& settingId,
      const TiXmlElement* settingElement,
      const std::string& defaultValue,
      std::string source);

  bool LoadOldSettingValues(const CXBMCTinyXML& doc,
                            std::map<std::string, std::string>& settings) const;

  struct ConditionExpression
  {
    SettingDependencyOperator m_operator;
    bool m_negated;
    int32_t m_relativeSettingIndex;
    std::string m_value;
  };

  bool ParseOldLabel(const TiXmlElement* element, const std::string& settingId, int& labelId);
  bool ParseOldCondition(const boost::shared_ptr<const CSetting>& setting,
                         const std::vector<boost::shared_ptr<const CSetting>>& settings,
                         const std::string& condition,
                         CSettingDependency& dependeny) const;
  static bool ParseOldConditionExpression(std::string str, ConditionExpression& expression);

  static void FileEnumSettingOptionsFiller(const boost::shared_ptr<const CSetting>& setting,
                                           std::vector<StringSettingOption>& list,
                                           std::string& current,
                                           void* data);

  // store these values so that we don't always have to access the weak pointer
  const std::string m_addonId;
  const std::string m_addonPath;
  const std::string m_addonProfile;
  const AddonInstanceId m_instanceId{ADDON_SETTINGS_ID};
  std::weak_ptr<IAddon> m_addon;

  uint32_t m_unidentifiedSettingId = 0;
  int m_unknownSettingLabelId;
  std::map<int, std::string> m_unknownSettingLabels;

  Logger m_logger;
};

} // namespace ADDON
