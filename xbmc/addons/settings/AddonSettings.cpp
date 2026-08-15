/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonSettings.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "addons/settings/SettingUrlEncodedString.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingAddon.h"
#include "settings/SettingConditions.h"
#include "settings/SettingControl.h"
#include "settings/SettingDateTime.h"
#include "settings/SettingPath.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <boost/bind.hpp>
#include <cassert>

namespace
{

const static char *OldSettingValuesSeparator = "|";

const static int UnknownSettingLabelIdStart = 100000;

bool InfoBool(const std::string& condition,
              const std::string& value,
              const SettingConstPtr& setting,
              void* data)
{
  return CServiceBroker::GetGUI()->GetInfoManager().EvaluateBool(value, INFO::DEFAULT_CONTEXT);
}

SettingPtr InitializeFromOldSettingWithoutDefinition(ADDON::CAddonSettings& settings,
                                                     const std::string& settingId,
                                                     const bool& defaultValue)
{
  boost::shared_ptr<CSettingBool> setting =
      boost::make_shared<CSettingBool>(settingId, settings.GetSettingsManager());
  setting->SetLevel(SettingLevel::Internal);
  setting->SetVisible(false);
  setting->SetDefault(defaultValue);

  return setting;
}

SettingPtr InitializeFromOldSettingWithoutDefinition(ADDON::CAddonSettings& settings,
                                                     const std::string& settingId,
                                                     const int& defaultValue)
{
  boost::shared_ptr<CSettingInt> setting =
      boost::make_shared<CSettingInt>(settingId, settings.GetSettingsManager());
  setting->SetLevel(SettingLevel::Internal);
  setting->SetVisible(false);
  setting->SetDefault(defaultValue);

  return setting;
}

SettingPtr InitializeFromOldSettingWithoutDefinition(ADDON::CAddonSettings& settings,
                                                     const std::string& settingId,
                                                     const double& defaultValue)
{
  boost::shared_ptr<CSettingNumber> setting =
      boost::make_shared<CSettingNumber>(settingId, settings.GetSettingsManager());
  setting->SetLevel(SettingLevel::Internal);
  setting->SetVisible(false);
  setting->SetDefault(defaultValue);

  return setting;
}

SettingPtr InitializeFromOldSettingWithoutDefinition(ADDON::CAddonSettings& settings,
                                                     const std::string& settingId,
                                                     const std::string& defaultValue)
{
  boost::shared_ptr<CSettingString> setting =
      boost::make_shared<CSettingString>(settingId, settings.GetSettingsManager());
  setting->SetLevel(SettingLevel::Internal);
  setting->SetVisible(false);
  setting->SetDefault(defaultValue);
  setting->SetAllowEmpty(true);

  return setting;
}

template<class TSetting>
SettingPtr AddSettingWithoutDefinition(ADDON::CAddonSettings& settings,
                                       const std::string& settingId,
                                       typename TSetting::Value defaultValue)
{
  if (settingId.empty())
    return SettingPtr();

  // if necessary try to initialize the settings manager on-the-fly without any definitions
  if (!settings.IsInitialized() && !settings.Initialize(CXBMCTinyXML(), true))
  {
    CLog::Log(LOGWARNING, "failed to initialize settings on-the-fly");
    return SettingPtr();
  }

  // check if we need to add a section on-the-fly
  SettingSectionList sections = settings.GetSettingsManager()->GetSections();
  SettingSectionPtr section;
  if (sections.empty())
    section =
        boost::make_shared<CSettingSection>(settings.GetAddonId(), settings.GetSettingsManager());
  else
    section = sections.back();

  // check if we need to add a category on-the-fly
  SettingCategoryList categories = section->GetCategories();
  SettingCategoryPtr category;
  if (categories.empty())
    category = boost::make_shared<CSettingCategory>("category0", settings.GetSettingsManager());
  else
    category = categories.back();

  // check if we need to add a group on-the-fly
  SettingGroupList groups = category->GetGroups();
  SettingGroupPtr group;
  if (groups.empty())
    group = boost::make_shared<CSettingGroup>("0", settings.GetSettingsManager());
  else
    group = groups.back();

  // create a new setting on-the-fly
  SettingPtr setting =
      InitializeFromOldSettingWithoutDefinition(settings, settingId, defaultValue);
  if (setting == NULL)
  {
    CLog::Log(LOGWARNING, "failed to create setting \"%s\" on-the-fly", settingId.c_str());
    return SettingPtr();
  }

  // add the setting (and if necessary the section, category and/or group)
  if (!settings.GetSettingsManager()->AddSetting(setting, section, category, group))
  {
    CLog::Log(LOGWARNING, "failed to add setting \"%s\" on-the-fly", settingId.c_str());
    return SettingPtr();
  }

  return setting;
}

} // namespace

namespace ADDON
{

CAddonSettings::CAddonSettings(const boost::shared_ptr<IAddon>& addon, AddonInstanceId instanceId)
  : CSettingsBase(),
    m_addonId(addon->ID()),
    m_addonPath(addon->Path()),
    m_addonProfile(addon->Profile()),
    m_instanceId(instanceId),
    m_addon(addon),
    m_unknownSettingLabelId(UnknownSettingLabelIdStart),
    m_unidentifiedSettingId(0)
{
}

boost::shared_ptr<CSetting> CAddonSettings::CreateSetting(
    const std::string& settingType,
    const std::string& settingId,
    CSettingsManager* settingsManager /* = NULL */) const
{
  if (StringUtils::EqualsNoCase(settingType, "urlencodedstring"))
    return boost::make_shared<CSettingUrlEncodedString>(settingId, settingsManager);

  return CSettingCreator::CreateSetting(settingType, settingId, settingsManager);
}

void CAddonSettings::OnSettingAction(const boost::shared_ptr<const CSetting>& setting)
{
  std::string actionData;
  bool closeDialog = false;

  // check if it's an action setting
  if (setting->GetType() == SettingType::Action)
  {
    boost::shared_ptr<const CSettingAction> settingAction = boost::dynamic_pointer_cast<const CSettingAction>(setting);
    if (settingAction != NULL && settingAction->HasData())
    {
      actionData = settingAction->GetData();
      // replace $CWD with the url of the add-on
      StringUtils::Replace(actionData, "$CWD", m_addonPath);
      // replace $ID with the id of the add-on
      StringUtils::Replace(actionData, "$ID", m_addonId);
    }
  }

  // check if the setting control's is a button and its format is action
  if (setting->GetControl()->GetType() == "button" &&
      setting->GetControl()->GetFormat() == "action")
  {
    boost::shared_ptr<const CSettingControlButton> controlButton =
        boost::dynamic_pointer_cast<const CSettingControlButton>(setting->GetControl());
    if (controlButton != NULL)
    {
      if (actionData.empty() && controlButton->HasActionData())
        actionData = controlButton->GetActionData();

      closeDialog = controlButton->CloseDialog();
    }
  }

  if (actionData.empty())
    return;

  if (closeDialog)
    CGUIDialogAddonSettings::SaveAndClose();

  CServiceBroker::GetAppMessenger()->SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, NULL, actionData);
}

static bool isCorrectGroupID(const SettingGroupPtr& group) { return group->GetId() == ADDON_SETTING_INSTANCE_GROUP; }

bool CAddonSettings::AddInstanceSettings()
{
  if (GetSetting(ADDON_SETTING_INSTANCE_NAME_VALUE) ||
      GetSetting(ADDON_SETTING_INSTANCE_ENABLED_VALUE))
  {
    CLog::Log(
        LOGDEBUG,
        "CAddonSettings::%s - Add-on %s using instance setting values byself, Kodi's add ignored",
        __FUNCTION__, m_addonId.c_str());
    return true;
  }

  CSettingsManager *mgr = GetSettingsManager();
  if (!mgr)
    return false;

  SettingSectionList sections = mgr->GetSections();
  if (sections.empty())
    return false;

  SettingSectionPtr section = *sections.begin();

  SettingCategoryList categories = section->GetCategories();
  if (categories.empty())
    return false;

  SettingCategoryPtr category = *categories.begin();

  SettingGroupList groups = category->GetGroups();
  SettingGroupList::iterator itr = std::find_if(groups.begin(), groups.end(), boost::bind(&isCorrectGroupID, _1));

  SettingGroupPtr group;
  if (itr != groups.end())
  {
    group = *itr;
  }
  else
  {
    group = boost::make_shared<CSettingGroup>(ADDON_SETTING_INSTANCE_GROUP, mgr);
    group->SetLabel(10017); // Add-on configuration
    category->AddGroupToFront(group);
  }

  const boost::shared_ptr<CSettingString> name =
      boost::make_shared<CSettingString>(ADDON_SETTING_INSTANCE_NAME_VALUE, 551, "", mgr); // Name
  name->SetAllowEmpty(false);
  name->SetControl(boost::make_shared<CSettingControlEdit>());
  if (!mgr->AddSetting(name, section, category, group))
    return false;

  const boost::shared_ptr<CSettingBool> enabled = boost::make_shared<CSettingBool>(
      ADDON_SETTING_INSTANCE_ENABLED_VALUE, 305, true, mgr); // Enabled
  enabled->SetControl(boost::make_shared<CSettingControlCheckmark>());
  if (!mgr->AddSetting(enabled, section, category, group))
    return false;

  return true;
}

bool CAddonSettings::Initialize(const CXBMCTinyXML& doc, bool allowEmpty /* = false */)
{
  CSingleLock lock(m_critical);
  if (m_initialized)
    return false;

  // register custom setting types
  InitializeSettingTypes();
  // register custom setting controls
  InitializeControls();

  // conditions need to be initialized before the setting definitions
  InitializeConditions();

  // load the settings definitions
  if (!InitializeDefinitions(doc) && !allowEmpty)
    return false;

  // Add internal settings to set values about instance set
  if (m_instanceId > 0 && !AddInstanceSettings())
    return false;

  GetSettingsManager()->SetInitialized();

  m_initialized = true;

  return true;
}

// helper lambda for parsing a setting's ID and value from XML
void parseSettingValue(ADDON::InfoMap& settingValues, const TiXmlNode *setting, const std::string &categoryId = "")
{
  // put together the setting ID
  std::string settingId = categoryId;
  if (!settingId.empty())
    settingId += ".";
  const char *id = setting->ToElement()->Attribute("id");
  if (id)
    settingId += id;

  // parse the setting value
  std::string settingValue;
  if (setting->FirstChild())
    settingValue = setting->FirstChild()->ValueStr();

  // add the setting to the map
  settingValues.insert(std::make_pair(settingId, settingValue));
}

bool CAddonSettings::Load(const CXBMCTinyXML& doc)
{
  CSingleLock lock(m_critical);
  if (!m_initialized)
    return false;

  // figure out the version of the setting definitions
  uint32_t version = 0;
  if (!ParseSettingVersion(doc, version))
  {
    CLog::Log(LOGERROR, "failed to determine setting values version");
    return false;
  }

  std::map<std::string, std::string> settingValues;

  // for new/"normal" setting values use the standard process
  if (version != 0)
  {
    bool updated;
    if (!LoadValuesFromXml(doc, updated))
      return false;

    // check if there were any setting values without a definition
    const TiXmlNode *category = doc.RootElement()->FirstChild();
    while (category != NULL)
    {
      // check if this really is a category with setting elements
      if (category->FirstChild() && category->FirstChild()->Type() == CXBMCTinyXML::TINYXML_ELEMENT)
      {
        const std::string &categoryId = category->ValueStr();
        const TiXmlNode *setting = category->FirstChild();
        while (setting != NULL)
        {
          parseSettingValue(settingValues, setting, categoryId);

          setting = setting->NextSibling();
        }
      }
      else
        parseSettingValue(settingValues, category, "");

      category = category->NextSibling();
    }
  }
  // for old setting values do it manually
  else if (!LoadOldSettingValues(doc, settingValues))
  {
    CLog::Log(LOGERROR, "failed to determine setting values from old format");
    return false;
  }

  // process all settings
  for (std::map<std::string, std::string>::const_iterator setting = settingValues.begin(); setting != settingValues.end(); ++setting)
  {
    // ignore setting values without a setting identifier
    if (setting->first.empty())
      continue;

    // try to find a matching setting
    SettingPtr newSetting = GetSetting(setting->first);
    if (newSetting == NULL)
    {
      // create a hidden/internal string setting on-the-fly
      newSetting = AddSettingWithoutDefinition<CSettingString>(*this, setting->first, setting->second);
    }

    // try to load the old setting value
    if (!newSetting)
    {
      CLog::Log(LOGERROR, "had null newSetting for value \"%s\" for setting %s", setting->second.c_str(),
                      setting->first.c_str());
    }
    else if (!newSetting->FromString(setting->second))
    {
      CLog::Log(LOGWARNING, "failed to load value \"%s\" for setting %s", setting->second.c_str(), setting->first.c_str());
    }
  }

  SetLoaded();

  return true;
}

bool CAddonSettings::Save(CXBMCTinyXML& doc) const
{
  CSingleLock lock(m_critical);
  if (!m_initialized)
    return false;

  if (!SaveValuesToXml(doc))
  {
    CLog::Log(LOGERROR, "failed to save settings");
    return false;
  }

  return true;
}

bool CAddonSettings::HasSettings() const
{
  return IsInitialized() && GetSettingsManager()->HasSettings();
}

bool CAddonSettings::Save()
{
  boost::shared_ptr<IAddon> addon = m_addon.lock();
  assert(addon);
  if (addon)
    return addon->SaveSettings();
  else
    return false;
}

std::string CAddonSettings::GetSettingLabel(int label) const
{
  if (label < UnknownSettingLabelIdStart || label >= m_unknownSettingLabelId)
    return "";

  const std::map<int, std::string>::const_iterator labelIt = m_unknownSettingLabels.find(label);
  if (labelIt == m_unknownSettingLabels.end())
    return "";

  return labelIt->second;
}

boost::shared_ptr<CSetting> CAddonSettings::AddSetting(const std::string& settingId, bool value)
{
  return AddSettingWithoutDefinition<CSettingBool>(*this, settingId, value);
}

boost::shared_ptr<CSetting> CAddonSettings::AddSetting(const std::string& settingId, int value)
{
  return AddSettingWithoutDefinition<CSettingInt>(*this, settingId, value);
}

boost::shared_ptr<CSetting> CAddonSettings::AddSetting(const std::string& settingId, double value)
{
  return AddSettingWithoutDefinition<CSettingNumber>(*this, settingId, value);
}

boost::shared_ptr<CSetting> CAddonSettings::AddSetting(const std::string& settingId,
                                                     const std::string& value)
{
  return AddSettingWithoutDefinition<CSettingString>(*this, settingId, value);
}

void CAddonSettings::InitializeSettingTypes()
{
  GetSettingsManager()->RegisterSettingType("addon", this);
  GetSettingsManager()->RegisterSettingType("date", this);
  GetSettingsManager()->RegisterSettingType("path", this);
  GetSettingsManager()->RegisterSettingType("time", this);
  GetSettingsManager()->RegisterSettingType("urlencodedstring", this);
}

void CAddonSettings::InitializeControls()
{
  GetSettingsManager()->RegisterSettingControl("toggle", this);
  GetSettingsManager()->RegisterSettingControl("spinner", this);
  GetSettingsManager()->RegisterSettingControl("edit", this);
  GetSettingsManager()->RegisterSettingControl("button", this);
  GetSettingsManager()->RegisterSettingControl("list", this);
  GetSettingsManager()->RegisterSettingControl("slider", this);
  GetSettingsManager()->RegisterSettingControl("range", this);
  GetSettingsManager()->RegisterSettingControl("title", this);
  GetSettingsManager()->RegisterSettingControl("colorbutton", this);
}

void CAddonSettings::InitializeConditions()
{
  CSettingConditions::Initialize();

  // add basic conditions
  const std::set<std::string>& simpleConditions = CSettingConditions::GetSimpleConditions();
  for (std::set<std::string>::const_iterator condition = simpleConditions.begin(); condition != simpleConditions.end(); ++condition)
    GetSettingsManager()->AddCondition(*condition);

  GetSettingsManager()->AddDynamicCondition("InfoBool", InfoBool);
}

bool CAddonSettings::InitializeDefinitions(const CXBMCTinyXML& doc)
{
  // figure out the version of the setting definitions
  uint32_t version = 0;
  if (!ParseSettingVersion(doc, version))
  {
    CLog::Log(LOGERROR, "failed to determine setting definitions version");
    return false;
  }

  // for new/"normal" setting definitions use the standard process
  if (version != 0)
    return InitializeDefinitionsFromXml(doc);

  // for old setting definitions do it manually
  return InitializeFromOldSettingDefinitions(doc);
}

bool CAddonSettings::ParseSettingVersion(const CXBMCTinyXML& doc, uint32_t& version) const
{
  const TiXmlElement* root = doc.RootElement();
  if (root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), SETTING_XML_ROOT))
  {
    CLog::Log(LOGERROR, "error reading setting definitions: no <settings> tag");
    return false;
  }

  version = GetSettingsManager()->ParseVersion(root);
  return true;
}

struct SettingWithConditions
{
  SettingPtr setting;
  std::string enableCondition;
  std::string visibleCondition;
  SettingDependencies deps;
};

static bool isParentEmpty(const SettingConstPtr& setting) { return setting->GetParent().empty(); }

boost::shared_ptr<CSettingGroup> CAddonSettings::ParseOldSettingElement(
    const TiXmlElement* categoryElement,
    const boost::shared_ptr<CSettingCategory>& category,
    std::set<std::string>& settingIds)
{
  // build a vector of settings from the same category
  std::vector<boost::shared_ptr<const CSetting> > categorySettings;

  // prepare for settings with enable/visible conditions
  std::vector<SettingWithConditions> settingsWithConditions;

  SettingGroupPtr group = boost::make_shared<CSettingGroup>("0", GetSettingsManager());
  uint32_t groupId = 1;

  // go through all settings in the category
  const TiXmlElement* settingElement = categoryElement->FirstChildElement("setting");
  while (settingElement != NULL)
  {
    // read the possible attributes
    const std::string settingType = XMLUtils::GetAttribute(settingElement, "type");
    const std::string settingId = XMLUtils::GetAttribute(settingElement, "id");
    const std::string defaultValue = XMLUtils::GetAttribute(settingElement, "default");
    const std::string settingValues = XMLUtils::GetAttribute(settingElement, "values");
    const std::vector<std::string> settingLValues = StringUtils::Split(
        XMLUtils::GetAttribute(settingElement, "lvalues"), OldSettingValuesSeparator);
    int settingLabel = -1;
    bool settingLabelParsed = ParseOldLabel(settingElement, settingId, settingLabel);

    SettingPtr setting;
    if (settingType == "sep" || settingType == "lsep")
    {
      // check if we need to create a new group
      if (!group->GetSettings().empty())
      {
        // add the current group to the category
        category->AddGroup(group);

        // and create a new one
        group = boost::make_shared<CSettingGroup>(std::to_string(groupId), GetSettingsManager());
        groupId += 1;
      }

      if (settingType == "lsep" && settingLabelParsed)
        group->SetLabel(settingLabel);
    }
    else if (settingId.empty() || settingType == "action")
    {
      if (settingType == "action")
        setting = InitializeFromOldSettingAction(settingId, settingElement, defaultValue);
      else
        setting = InitializeFromOldSettingLabel();
    }
    else if (settingType == "bool")
      setting = InitializeFromOldSettingBool(settingId, settingElement, defaultValue);
    else if (settingType == "text" || settingType == "ipaddress")
      setting = InitializeFromOldSettingTextIpAddress(settingId, settingType, settingElement,
                                                      defaultValue, settingLabel);
    else if (settingType == "number")
      setting =
          InitializeFromOldSettingNumber(settingId, settingElement, defaultValue, settingLabel);
    else if (settingType == "video" || settingType == "audio" || settingType == "image" ||
             settingType == "executable" || settingType == "file" || settingType == "folder")
      setting = InitializeFromOldSettingPath(settingId, settingType, settingElement, defaultValue,
                                             settingLabel);
    else if (settingType == "date")
      setting = InitializeFromOldSettingDate(settingId, settingElement, defaultValue, settingLabel);
    else if (settingType == "time")
      setting = InitializeFromOldSettingTime(settingId, settingElement, defaultValue, settingLabel);
    else if (settingType == "select")
      setting = InitializeFromOldSettingSelect(settingId, settingElement, defaultValue,
                                               settingLabel, settingValues, settingLValues);
    else if (settingType == "addon")
      setting =
          InitializeFromOldSettingAddon(settingId, settingElement, defaultValue, settingLabel);
    else if (settingType == "enum" || settingType == "labelenum")
      setting = InitializeFromOldSettingEnums(settingId, settingType, settingElement, defaultValue,
                                              settingValues, settingLValues);
    else if (settingType == "fileenum")
      setting =
          InitializeFromOldSettingFileEnum(settingId, settingElement, defaultValue, settingValues);
    else if (settingType == "rangeofnum")
      setting = InitializeFromOldSettingRangeOfNum(settingId, settingElement, defaultValue);
    else if (settingType == "slider")
      setting = InitializeFromOldSettingSlider(settingId, settingElement, defaultValue);
    else if (settingType.empty())
    {
      // setting definitions without a type are considered as "text" / strings but are hidden
      setting = InitializeFromOldSettingTextIpAddress(settingId, "text", settingElement,
                                                      defaultValue, settingLabel);
      setting->SetLevel(SettingLevel::Internal);
    }
    else
    {
      CLog::Log(LOGWARNING, "failed to parse old setting definition for \"%s\" of type \"%s\"", settingId.c_str(),
                     settingType.c_str());
    }

    // process general properties
    if (setting != NULL)
    {
      // set the default level to be Basic
      if (setting->GetLevel() != SettingLevel::Internal)
      {
        setting->SetLevel(SettingLevel::Basic);
      }

      // use the setting's ID if there's no label
      if (settingLabel < 0)
      {
        settingLabel = m_unknownSettingLabelId;
        m_unknownSettingLabelId += 1;

        m_unknownSettingLabels.insert(std::make_pair(settingLabel, settingId));
      }

      // set the setting's label
      setting->SetLabel(settingLabel);

      // handle subsettings
      bool isSubsetting = false;
      if (settingElement->QueryBoolAttribute("subsetting", &isSubsetting) == TIXML_SUCCESS &&
          isSubsetting)
      {
        // find the last non-subsetting in the current group and use that as the parent setting
        const SettingList groupSettings = group->GetSettings();
        const std::reverse_iterator<SettingList::const_iterator> parentSetting = std::find_if(
            groupSettings.rbegin(), groupSettings.rend(), boost::bind(&isParentEmpty, _1));

        if (parentSetting != groupSettings.rend())
        {
          if ((*parentSetting)->IsReference())
            setting->SetParent((*parentSetting)->GetReferencedId());
          else
            setting->SetParent((*parentSetting)->GetId());
        }
      }

      SettingWithConditions settingWithConditions;

      // parse enable status
      const std::string conditionEnable = XMLUtils::GetAttribute(settingElement, "enable");
      if (StringUtils::EqualsNoCase(conditionEnable, "true"))
        setting->SetEnabled(true);
      else if (StringUtils::EqualsNoCase(conditionEnable, "false"))
        setting->SetEnabled(false);
      else if (!conditionEnable.empty())
        settingWithConditions.enableCondition = conditionEnable;

      // parse visible status
      const std::string conditionVisible = XMLUtils::GetAttribute(settingElement, "visible");
      if (StringUtils::EqualsNoCase(conditionVisible, "true"))
        setting->SetVisible(true);
      else if (StringUtils::EqualsNoCase(conditionVisible, "false"))
        setting->SetVisible(false);
      else if (!conditionVisible.empty())
        settingWithConditions.visibleCondition = conditionVisible;

      // check if there already is a setting with the setting identifier
      if (settingIds.find(settingId) != settingIds.end())
      {
        // turn the setting into a reference setting
        setting->MakeReference();
      }
      else
      {
        // add the setting's identifier to the list of all identifiers
        settingIds.insert(setting->GetId());
      }

      if (!settingWithConditions.enableCondition.empty() ||
          !settingWithConditions.visibleCondition.empty())
      {
        settingWithConditions.setting = setting;
        settingsWithConditions.push_back(settingWithConditions);
      }

      // add the setting to the list of settings from the same category
      categorySettings.push_back(setting);

      // add the setting to the current group
      group->AddSetting(setting);
    }
    else
    {
      // add a dummy setting for the group / separator to the list of settings from the same category
      categorySettings.push_back(SettingPtr());
    }

    // look for the next setting
    settingElement = settingElement->NextSiblingElement("setting");
  }

  // process settings with enable/visible conditions
  for (std::vector<SettingWithConditions>::iterator setting = settingsWithConditions.begin(); setting != settingsWithConditions.end(); ++setting)
  {
    if (!setting->enableCondition.empty())
    {
      CSettingDependency dependencyEnable(SettingDependencyType::Enable, GetSettingsManager());
      if (ParseOldCondition(setting->setting, categorySettings, setting->enableCondition,
                            dependencyEnable))
        setting->deps.push_back(dependencyEnable);
      else
      {
        CLog::Log(LOGWARNING,
            "failed to parse enable condition \"{}\" of old setting definition for \"{}\"",
            setting->enableCondition, setting->setting->GetId());
      }
    }

    if (!setting->visibleCondition.empty())
    {
      CSettingDependency dependencyVisible(SettingDependencyType::Visible, GetSettingsManager());
      if (ParseOldCondition(setting->setting, categorySettings, setting->visibleCondition,
                            dependencyVisible))
        setting->deps.push_back(dependencyVisible);
      else
      {
        CLog::Log(LOGWARNING,
            "failed to parse visible condition \"{}\" of old setting definition for \"{}\"",
            setting->visibleCondition, setting->setting->GetId());
      }
    }

    // set dependencies
    setting->setting->SetDependencies(setting->deps);
  }

  return group;
}

boost::shared_ptr<CSettingCategory> CAddonSettings::ParseOldCategoryElement(
    uint32_t& categoryId, const TiXmlElement* categoryElement, std::set<std::string>& settingIds)
{
  // create the category
  SettingCategoryPtr category = boost::make_shared<CSettingCategory>(StringUtils::Format("category%" PRIu32, categoryId),
                                                     GetSettingsManager());
  categoryId += 1;

  // try to get the category's label and fall back to "General"
  int categoryLabel = 128;
  ParseOldLabel(categoryElement, g_localizeStrings.Get(categoryLabel), categoryLabel);
  category->SetLabel(categoryLabel);

  // prepare a setting group
  SettingGroupPtr group = ParseOldSettingElement(categoryElement, category, settingIds);

  // add the group to the category
  category->AddGroup(group);

  return category;
}

bool CAddonSettings::InitializeFromOldSettingDefinitions(const CXBMCTinyXML& doc)
{
  CLog::Log(LOGDEBUG, "trying to load setting definitions from old format...");

  const TiXmlElement* root = doc.RootElement();
  if (root == NULL)
    return false;

  boost::shared_ptr<CSettingSection> section =
      boost::make_shared<CSettingSection>(m_addonId, GetSettingsManager());

  boost::shared_ptr<CSettingCategory> category;
  uint32_t categoryId = 0;

  // Settings id set
  std::set<std::string> settingIds;

  // Special case for no category settings
  section->AddCategory(ParseOldCategoryElement(categoryId, root, settingIds));

  const TiXmlElement* categoryElement = root->FirstChildElement("category");
  while (categoryElement != NULL)
  {
    section->AddCategory(ParseOldCategoryElement(categoryId, categoryElement, settingIds));

    // look for the next category
    categoryElement = categoryElement->NextSiblingElement("category");
  }

  // add the section to the settingsmanager
  GetSettingsManager()->AddSection(section);

  return true;
}

SettingPtr CAddonSettings::InitializeFromOldSettingAction(const std::string& settingId,
                                                          const TiXmlElement* settingElement,
                                                          const std::string& defaultValue)
{
  // parse the action attribute
  std::string action = XMLUtils::GetAttribute(settingElement, "action");
  // replace $CWD with the url of the add-on
  StringUtils::Replace(action, "$CWD", m_addonPath);
  // replace $ID with the id of the add-on
  StringUtils::Replace(action, "$ID", m_addonId);

  // prepare the setting's control
  boost::shared_ptr<CSettingControlButton> control = boost::make_shared<CSettingControlButton>();
  control->SetFormat("action");

  SettingPtr setting;
  // action settings don't require a setting id
  if (settingId.empty())
  {
    std::string actionSettingId = StringUtils::Format("action%" PRIu32, m_unidentifiedSettingId);
    m_unidentifiedSettingId += 1;

    boost::shared_ptr<CSettingAction> settingAction = boost::make_shared<CSettingAction>(actionSettingId, GetSettingsManager());
    settingAction->SetData(action);

    setting = settingAction;
  }
  else
  {
    // assume that the setting might store a value as a string
    boost::shared_ptr<CSettingString> settingString = boost::make_shared<CSettingString>(settingId, GetSettingsManager());
    settingString->SetDefault(defaultValue);
    settingString->SetAllowEmpty(true);

    control->SetActionData(action);

    setting = settingString;
  }

  // get any options
  std::string option = XMLUtils::GetAttribute(settingElement, "option");
  // handle the "close" option
  if (StringUtils::EqualsNoCase(option, "close"))
    control->SetCloseDialog(true);

  setting->SetControl(control);

  return setting;
}

boost::shared_ptr<CSetting> CAddonSettings::InitializeFromOldSettingLabel()
{
  // label settings don't require a setting id
  std::string labelSettingId = StringUtils::Format("label%" PRIu32, m_unidentifiedSettingId);
  m_unidentifiedSettingId += 1;

  boost::shared_ptr<CSettingString> settingLabel = boost::make_shared<CSettingString>(labelSettingId, GetSettingsManager());

  // create the setting's control
  settingLabel->SetControl(boost::make_shared<CSettingControlLabel>());

  return settingLabel;
}

SettingPtr CAddonSettings::InitializeFromOldSettingBool(const std::string& settingId,
                                                        const TiXmlElement* settingElement,
                                                        const std::string& defaultValue)
{
  boost::shared_ptr<CSettingBool> setting = boost::make_shared<CSettingBool>(settingId, GetSettingsManager());
  if (setting->FromString(defaultValue))
    setting->SetDefault(setting->GetValue());

  setting->SetControl(boost::make_shared<CSettingControlCheckmark>());

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingTextIpAddress(const std::string& settingId,
                                                                 const std::string& settingType,
                                                                 const TiXmlElement* settingElement,
                                                                 const std::string& defaultValue,
                                                                 const int settingLabel)
{
  boost::shared_ptr<CSettingString> setting;
  boost::shared_ptr<CSettingControlEdit> control = boost::make_shared<CSettingControlEdit>();
  control->SetHeading(settingLabel);

  // get any options
  std::string option = XMLUtils::GetAttribute(settingElement, "option");

  if (settingType == "ipaddress")
  {
    setting = boost::make_shared<CSettingString>(settingId, GetSettingsManager());
    control->SetFormat("ip");
  }
  else if (settingType == "text")
  {

    if (StringUtils::EqualsNoCase(option, "urlencoded"))
    {
      setting = boost::make_shared<CSettingUrlEncodedString>(settingId, GetSettingsManager());
      control->SetFormat("urlencoded");
    }
    else
    {
      setting = boost::make_shared<CSettingString>(settingId, GetSettingsManager());
      control->SetFormat("string");
      control->SetHidden(StringUtils::EqualsNoCase(option, "hidden"));
    }
  }

  setting->SetDefault(defaultValue);
  setting->SetAllowEmpty(true);
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingNumber(const std::string& settingId,
                                                          const TiXmlElement* settingElement,
                                                          const std::string& defaultValue,
                                                          const int settingLabel)
{
  boost::shared_ptr<CSettingInt> setting = boost::make_shared<CSettingInt>(settingId, GetSettingsManager());
  if (setting->FromString(defaultValue))
    setting->SetDefault(setting->GetValue());

  boost::shared_ptr<CSettingControlEdit> control = boost::make_shared<CSettingControlEdit>();
  control->SetHeading(settingLabel);
  control->SetFormat("integer");
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingPath(const std::string& settingId,
                                                        const std::string& settingType,
                                                        const TiXmlElement* settingElement,
                                                        const std::string& defaultValue,
                                                        const int settingLabel)
{
  boost::shared_ptr<CSettingPath> setting = boost::make_shared<CSettingPath>(settingId, GetSettingsManager());
  setting->SetDefault(defaultValue);

  // parse sources/shares
  const std::string source = XMLUtils::GetAttribute(settingElement, "source");
  if (!source.empty())
    setting->SetSources(std::vector<std::string>(1, source));

  // setup masking
  const std::string audioMask = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicExtensions;
  const std::string videoMask = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoExtensions;
  const std::string imageMask = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pictureExtensions;
  const char *execMask = "";
#if defined(TARGET_WINDOWS)
  execMask = ".exe|.bat|.cmd|.py";
#endif // defined(TARGET_WINDOWS)

  std::string mask = XMLUtils::GetAttribute(settingElement, "mask");
  if (!mask.empty())
  {
    // convert mask qualifiers
    StringUtils::Replace(mask, "$AUDIO", audioMask);
    StringUtils::Replace(mask, "$VIDEO", videoMask);
    StringUtils::Replace(mask, "$IMAGE", imageMask);
    StringUtils::Replace(mask, "$EXECUTABLE", execMask);
  }
  else
  {
    if (settingType == "video")
      mask = videoMask;
    else if (settingType == "audio")
      mask = audioMask;
    else if (settingType == "image")
      mask = imageMask;
    else if (settingType == "executable")
      mask = execMask;
  }
  setting->SetMasking(mask);

  // parse options
  const std::string option = XMLUtils::GetAttribute(settingElement, "option");
  setting->SetWritable(StringUtils::EqualsNoCase(option, "writeable"));

  boost::shared_ptr<CSettingControlButton> control = boost::make_shared<CSettingControlButton>();
  if (settingType == "folder")
    control->SetFormat("path");
  else if (settingType == "image")
    control->SetFormat("image");
  else
  {
    control->SetFormat("file");

    // parse the options
    const std::vector<std::string> options = StringUtils::Split(option, OldSettingValuesSeparator);
    control->SetUseImageThumbs(std::find(options.begin(), options.end(), "usethumbs") !=
                               options.end());
    control->SetUseFileDirectories(std::find(options.begin(), options.end(), "treatasfolder") !=
                                   options.end());
  }
  control->SetHeading(settingLabel);
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingDate(const std::string& settingId,
                                                        const TiXmlElement* settingElement,
                                                        const std::string& defaultValue,
                                                        const int settingLabel)
{
  boost::shared_ptr<CSettingDate> setting = boost::make_shared<CSettingDate>(settingId, GetSettingsManager());
  if (setting->FromString(defaultValue))
    setting->SetDefault(setting->GetValue());

  boost::shared_ptr<CSettingControlButton> control = boost::make_shared<CSettingControlButton>();
  control->SetFormat("date");
  control->SetHeading(settingLabel);
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingTime(const std::string& settingId,
                                                        const TiXmlElement* settingElement,
                                                        const std::string& defaultValue,
                                                        const int settingLabel)
{
  boost::shared_ptr<CSettingTime> setting = boost::make_shared<CSettingTime>(settingId, GetSettingsManager());
  if (setting->FromString(defaultValue))
    setting->SetDefault(setting->GetValue());

  boost::shared_ptr<CSettingControlButton> control = boost::make_shared<CSettingControlButton>();
  control->SetFormat("time");
  control->SetHeading(settingLabel);
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingSelect(
    const std::string& settingId,
    const TiXmlElement* settingElement,
    const std::string& defaultValue,
    const int settingLabel,
    const std::string& settingValues,
    const std::vector<std::string>& settingLValues)
{
  // process values and lvalues
  std::vector<std::string> values;
  if (!settingLValues.empty())
    values = settingLValues;
  else
    values = StringUtils::Split(settingValues, OldSettingValuesSeparator);

  SettingPtr setting;
  if (!values.empty())
  {
    if (settingLValues.empty())
    {
      boost::shared_ptr<CSettingString> settingString = boost::make_shared<CSettingString>(settingId, GetSettingsManager());
      settingString->SetDefault(defaultValue);

      StringSettingOptions options;
      for (std::vector<std::string>::const_iterator value = values.begin(); value != values.end(); ++value)
        options.push_back(StringSettingOption(*value, *value));
      settingString->SetOptions(options);

      setting = settingString;
    }
    else
    {
      boost::shared_ptr<CSettingInt> settingInt = boost::make_shared<CSettingInt>(settingId, GetSettingsManager());
      if (settingInt->FromString(defaultValue))
        settingInt->SetDefault(settingInt->GetValue());

      TranslatableIntegerSettingOptions options;
      for (uint32_t i = 0; i < values.size(); ++i)
        options.push_back(TranslatableIntegerSettingOption(static_cast<int>(strtol(values[i].c_str(), NULL, 0)), i));
      settingInt->SetTranslatableOptions(options);

      setting = settingInt;
    }
  }
  else
  {
    // parse sources/shares
    const std::string source = XMLUtils::GetAttribute(settingElement, "source");
    if (!source.empty())
      setting = InitializeFromOldSettingFileWithSource(settingId, settingElement, defaultValue,
                                                       settingValues);
    else
      CLog::Log(LOGWARNING, "failed to parse old setting definition for \"%s\" of type \"select\"",
                     settingId.c_str());
  }

  if (setting != NULL)
  {
    boost::shared_ptr<CSettingControlList> control = boost::make_shared<CSettingControlList>();
    control->SetHeading(settingLabel);
    control->SetFormat("string");
    setting->SetControl(control);
  }

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingAddon(const std::string& settingId,
                                                         const TiXmlElement* settingElement,
                                                         const std::string& defaultValue,
                                                         const int settingLabel)
{
  // get addon types
  std::string addonTypeStr = XMLUtils::GetAttribute(settingElement, "addontype");
  const std::vector<std::string> addonTypesStr = StringUtils::Split(addonTypeStr, ",");
  std::set<AddonType::Type> addonTypes;
  for (std::vector<std::string>::const_iterator it = addonTypesStr.begin(); it != addonTypesStr.end(); ++it)
  {
    std::string addonType = *it;
    ADDON::AddonType::Type type = ADDON::CAddonInfo::TranslateType(StringUtils::Trim(addonType));
    if (type != ADDON::AddonType::UNKNOWN)
      addonTypes.insert(type);
  }

  if (addonTypes.empty())
  {
    CLog::Log(LOGERROR, "missing addon type for addon setting \"%s\"", settingId.c_str());
    return SettingPtr();
  }

  // TODO: support multiple addon types
  if (addonTypes.size() > 1)
  {
    CLog::Log(LOGERROR, "multiple addon types are not supported (addon setting \"%s\")", settingId.c_str());
    return SettingPtr();
  }

  // parse addon ids
  std::vector<std::string> addonIds = StringUtils::Split(defaultValue, ",");

  // parse multiselect option
  bool multiselect = false;
  settingElement->QueryBoolAttribute("multiselect", &multiselect);

  // sanity check
  if (addonIds.size() > 1 && !multiselect)
  {
    CLog::Log(LOGWARNING, "multiple default addon ids on non-multiselect addon setting \"%s\"", settingId.c_str());
    addonIds.erase(++addonIds.begin(), addonIds.end());
  }

  boost::shared_ptr<CSettingAddon> settingAddon = boost::make_shared<CSettingAddon>(settingId, GetSettingsManager());
  settingAddon->SetAddonType(*addonTypes.begin());

  SettingPtr setting = settingAddon;
  if (multiselect)
  {
    boost::shared_ptr<CSettingList> settingList =
        boost::make_shared<CSettingList>(settingId, settingAddon, GetSettingsManager());
    settingList->SetDelimiter(",");
    if (settingList->FromString(addonIds))
      settingList->SetDefault(settingList->GetValue());

    setting = settingList;
  }
  else if (!addonIds.empty())
    settingAddon->SetDefault(addonIds.front());

  boost::shared_ptr<CSettingControlButton> control = boost::make_shared<CSettingControlButton>();
  control->SetFormat("addon");
  control->SetHeading(settingLabel);
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingEnums(
    const std::string& settingId,
    const std::string& settingType,
    const TiXmlElement* settingElement,
    const std::string& defaultValue,
    const std::string& settingValues,
    const std::vector<std::string>& settingLValues)
{
  // process values and lvalues
  std::vector<std::string> values;
  if (!settingLValues.empty())
    values = settingLValues;
  else if (settingValues == "$HOURS")
  {
    for (uint32_t hour = 0; hour < 24; hour++)
      values.push_back(
          CDateTime(2000, 1, 1, hour, 0, 0).GetAsLocalizedTime(g_langInfo.GetTimeFormat(), false));
  }
  else
    values = StringUtils::Split(settingValues, OldSettingValuesSeparator);

  // process entries
  const std::vector<std::string> settingEntries = StringUtils::Split(XMLUtils::GetAttribute(settingElement, "entries"),
                                                 OldSettingValuesSeparator);

  // process sort
  bool sortAscending = false;
  std::string sort = XMLUtils::GetAttribute(settingElement, "sort");
  if (sort == "true" || sort == "yes")
    sortAscending = true;

  SettingPtr setting;
  if (settingType == "enum")
  {
    boost::shared_ptr<CSettingInt> settingInt = boost::make_shared<CSettingInt>(settingId, GetSettingsManager());

    if (settingLValues.empty())
    {
      IntegerSettingOptions options;
      for (uint32_t i = 0; i < values.size(); ++i)
      {
        std::string label = values[i];
        int value = i;
        if (settingEntries.size() > i)
          value = static_cast<int>(strtol(settingEntries[i].c_str(), NULL, 0));

        options.push_back(IntegerSettingOption(label, value));
      }

      settingInt->SetOptions(options);
    }
    else
    {
      TranslatableIntegerSettingOptions options;
      for (uint32_t i = 0; i < values.size(); ++i)
      {
        int label = static_cast<int>(strtol(values[i].c_str(), NULL, 0));
        int value = i;
        if (settingEntries.size() > i)
          value = static_cast<int>(strtol(settingEntries[i].c_str(), NULL, 0));

        options.push_back(TranslatableIntegerSettingOption(label, value));
      }

      settingInt->SetTranslatableOptions(options);
    }

    if (sortAscending)
      settingInt->SetOptionsSort(SettingOptionsSort::Ascending);

    // set the default value
    if (settingInt->FromString(defaultValue))
      settingInt->SetDefault(settingInt->GetValue());

    setting = settingInt;
  }
  else
  {
    boost::shared_ptr<CSettingString> settingString = boost::make_shared<CSettingString>(settingId, GetSettingsManager());

    if (settingLValues.empty())
    {
      StringSettingOptions options;
      for (uint32_t i = 0; i < values.size(); ++i)
      {
        std::string value = values[i];
        if (settingEntries.size() > i)
          value = settingEntries[i];

        options.push_back(StringSettingOption(value, value));
      }

      settingString->SetOptions(options);
    }
    else
    {
      TranslatableStringSettingOptions options;
      for (uint32_t i = 0; i < values.size(); ++i)
      {
        int label = static_cast<int>(strtol(values[i].c_str(), NULL, 0));
        std::string value = g_localizeStrings.GetAddonString(m_addonId, label);
        if (settingEntries.size() > i)
          value = settingEntries[i];

        options.push_back(TranslatableStringSettingOption(label, value));
      }

      settingString->SetTranslatableOptions(options);
    }

    if (sortAscending)
      settingString->SetOptionsSort(SettingOptionsSort::Ascending);

    // set the default value
    settingString->SetDefault(defaultValue);

    setting = settingString;
  }

  boost::shared_ptr<CSettingControlSpinner> control = boost::make_shared<CSettingControlSpinner>();
  control->SetFormat("string");
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingFileEnum(const std::string& settingId,
                                                            const TiXmlElement* settingElement,
                                                            const std::string& defaultValue,
                                                            const std::string& settingValues)
{
  SettingPtr setting = InitializeFromOldSettingFileWithSource(settingId, settingElement, defaultValue,
                                                        settingValues);

  boost::shared_ptr<CSettingControlSpinner> control = boost::make_shared<CSettingControlSpinner>();
  control->SetFormat("string");
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingRangeOfNum(const std::string& settingId,
                                                              const TiXmlElement* settingElement,
                                                              const std::string& defaultValue)
{
  boost::shared_ptr<CSettingNumber> setting = boost::make_shared<CSettingNumber>(settingId, GetSettingsManager());
  if (setting->FromString(defaultValue))
    setting->SetDefault(setting->GetValue());

  // parse rangestart and rangeend
  double rangeStart = 0.0, rangeEnd = 1.0;
  settingElement->QueryDoubleAttribute("rangestart", &rangeStart);
  settingElement->QueryDoubleAttribute("rangeend", &rangeEnd);
  setting->SetMinimum(rangeStart);
  setting->SetMaximum(rangeEnd);

  // parse elements
  uint32_t elements = 2;
  settingElement->QueryUnsignedAttribute("elements", &elements);
  if (elements > 1)
    setting->SetStep((rangeEnd - rangeStart) / (elements - 1));

  // parse valueformat
  int valueFormat = -1;
  settingElement->QueryIntAttribute("valueformat", &valueFormat);

  boost::shared_ptr<CSettingControlSpinner> control = boost::make_shared<CSettingControlSpinner>();
  control->SetFormat("string");
  control->SetFormatLabel(valueFormat);
  setting->SetControl(control);

  return setting;
}

SettingPtr CAddonSettings::InitializeFromOldSettingSlider(const std::string& settingId,
                                                          const TiXmlElement* settingElement,
                                                          const std::string& defaultValue)
{
  // parse range
  double min = 0.0, max = 100.0, step = 1.0;
  const std::vector<std::string> range = StringUtils::Split(XMLUtils::GetAttribute(settingElement, "range"), ',');

  if (range.size() > 1)
  {
    min = strtod(range[0].c_str(), NULL);

    if (range.size() > 2)
    {
      max = strtod(range[2].c_str(), NULL);
      step = strtod(range[1].c_str(), NULL);
    }
    else
      max = strtod(range[1].c_str(), NULL);
  }

  // parse option
  std::string option = XMLUtils::GetAttribute(settingElement, "option");
  if (option.empty() || StringUtils::EqualsNoCase(option, "float"))
  {
    boost::shared_ptr<CSettingNumber> setting = boost::make_shared<CSettingNumber>(settingId, GetSettingsManager());
    if (setting->FromString(defaultValue))
      setting->SetDefault(setting->GetValue());

    setting->SetMinimum(min);
    setting->SetStep(step);
    setting->SetMaximum(max);

    boost::shared_ptr<CSettingControlSlider> control = boost::make_shared<CSettingControlSlider>();
    control->SetFormat("number");
    control->SetPopup(false);
    setting->SetControl(control);

    return setting;
  }

  if (StringUtils::EqualsNoCase(option, "int") || StringUtils::EqualsNoCase(option, "percent"))
  {
    boost::shared_ptr<CSettingInt> setting = boost::make_shared<CSettingInt>(settingId, GetSettingsManager());
    if (setting->FromString(defaultValue))
      setting->SetDefault(setting->GetValue());

    setting->SetMinimum(static_cast<int>(min));
    setting->SetStep(static_cast<int>(step));
    setting->SetMaximum(static_cast<int>(max));

    boost::shared_ptr<CSettingControlSlider> control = boost::make_shared<CSettingControlSlider>();
    control->SetFormat(StringUtils::EqualsNoCase(option, "int") ? "integer" : "percentage");
    control->SetPopup(false);
    setting->SetControl(control);

    return setting;
  }

  CLog::Log(LOGWARNING, "ignoring old setting definition for \"%s\" of type \"slider\" because of unknown "
                 "option \"%s\"",
                 settingId.c_str(), option.c_str());

  return SettingPtr();
}

SettingPtr CAddonSettings::InitializeFromOldSettingFileWithSource(
    const std::string& settingId,
    const TiXmlElement* settingElement,
    const std::string& defaultValue,
    std::string source)
{
  boost::shared_ptr<CSettingPath> setting = boost::make_shared<CSettingPath>(settingId, GetSettingsManager());
  setting->SetDefault(defaultValue);

  if (source.find("$PROFILE") != std::string::npos)
    StringUtils::Replace(source, "$PROFILE", m_addonProfile);
  else
    source = URIUtils::AddFileToFolder(m_addonPath, source);

  setting->SetSources(std::vector<std::string>(1, source));

  // process the path/file mask
  setting->SetMasking(XMLUtils::GetAttribute(settingElement, "mask"));

  // process option
  std::string option = XMLUtils::GetAttribute(settingElement, "option");
  setting->SetHideExtension(StringUtils::EqualsNoCase(option, "hideext"));

  setting->SetOptionsFiller(FileEnumSettingOptionsFiller);

  return setting;
}

bool CAddonSettings::LoadOldSettingValues(const CXBMCTinyXML& doc,
                                          std::map<std::string, std::string>& settings) const
{
  if (!doc.RootElement())
    return false;

  const TiXmlElement* category = doc.RootElement()->FirstChildElement("category");
  if (category == NULL)
    category = doc.RootElement();

  while (category != NULL)
  {
    const TiXmlElement* setting = category->FirstChildElement("setting");
    while (setting != NULL)
    {
      const char* id = setting->Attribute("id");
      const char* value = setting->Attribute("value");
      if (id != NULL && value != NULL)
        settings[id] = value;

      setting = setting->NextSiblingElement("setting");
    }

    category = category->NextSiblingElement("category");
  }

  return !settings.empty();
}

bool CAddonSettings::ParseOldLabel(const TiXmlElement* element,
                                   const std::string& settingId,
                                   int& labelId)
{
  if (element == NULL)
  {
    labelId = -1;
    return false;
  }

  // label value as a string
  std::string labelString;
  element->QueryStringAttribute("label", &labelString);

  bool parsed = !labelString.empty();

  // try to parse the label as a pure number, i.e. a localized string
  if (parsed)
  {
    char* endptr;
    labelId = std::strtol(labelString.c_str(), &endptr, 10);
    if (endptr == NULL || *endptr == '\0')
      return true;
  }
  else if (labelId != -1)
    return true;
  // make sure the label string is not empty
  else
    labelString = " ";

  labelId = m_unknownSettingLabelId;
  m_unknownSettingLabelId += 1;
  m_unknownSettingLabels.insert(std::make_pair(labelId, labelString));

  return parsed;
}

static bool isSameSettingID(const SettingConstPtr& otherSetting, const SettingConstPtr &setting)
{
  if (otherSetting == NULL)
    return false;

  return setting->GetId() == otherSetting->GetId();
}

bool CAddonSettings::ParseOldCondition(const boost::shared_ptr<const CSetting>& setting,
                                       const std::vector<boost::shared_ptr<const CSetting> >& settings,
                                       const std::string& condition,
                                       CSettingDependency& dependeny) const
{
  if (setting == NULL)
    return false;

  if (condition.empty())
    return true;

  // find the index of the setting in the list of all settings of the category
  std::vector<SettingConstPtr>::const_iterator settingIt = std::find_if(settings.begin(), settings.end(), boost::bind(&isSameSettingID, _1, setting));
  if (settingIt == settings.end())
  {
    CLog::Log(LOGWARNING, "failed to parse old setting conditions \"%s\" for \"%s\"", condition.c_str(),
                   setting->GetId().c_str());
    return false;
  }
  int32_t currentSettingIndex = std::distance(settings.begin(), settingIt);

  CSettingDependencyConditionCombinationPtr dependencyCombination;
  std::vector<std::string> conditions;
  if (condition.find('+') != std::string::npos)
  {
    StringUtils::Tokenize(condition, conditions, '+');
    dependencyCombination = dependeny.And();
  }
  else
  {
    StringUtils::Tokenize(condition, conditions, '|');
    dependencyCombination = dependeny.Or();
  }

  bool error = false;
  for (std::vector<std::string>::const_iterator cond = conditions.begin(); cond != conditions.end(); ++cond)
  {
    ConditionExpression expression;
    if (!ParseOldConditionExpression(*cond, expression))
      continue;

    // determine the absolute setting index
    int32_t absoluteSettingIndex = currentSettingIndex + expression.m_relativeSettingIndex;

    // we cannot handle relative indices pointing to settings not belonging to the same category
    if (absoluteSettingIndex < 0 || static_cast<size_t>(absoluteSettingIndex) >= settings.size())
    {
      CLog::Log(LOGWARNING, "cannot reference setting (relative index: %" PRIu32 "; absolute index: %" PRIu32 ") in another "
                     "category in old setting condition \"%s\" for \"%s\"",
                     expression.m_relativeSettingIndex, absoluteSettingIndex, (*cond).c_str(),
                     setting->GetId().c_str());
      error = true;
      continue;
    }

    const SettingConstPtr& referencedSetting = settings.at(absoluteSettingIndex);
    if (referencedSetting == NULL)
    {
      CLog::Log(LOGWARNING,
          "cannot reference separator setting in old setting condition \"{}\" for \"{}\"", *cond,
          setting->GetId());
      error = true;
      continue;
    }

    // try to handle some odd cases where the setting is of type string but the comparison value references the index of the value in the list of options
    if (referencedSetting->GetType() == SettingType::String &&
        StringUtils::IsNaturalNumber(expression.m_value))
    {
      // try to parse the comparison value
      size_t valueIndex = static_cast<size_t>(strtoul(expression.m_value.c_str(), NULL, 10));

      const boost::shared_ptr<const CSettingString> referencedSettingString =
          boost::static_pointer_cast<const CSettingString>(referencedSetting);
      switch (referencedSettingString->GetOptionsType())
      {
        case SettingOptionsType::Static:
        {
          const StringSettingOptions &options = referencedSettingString->GetOptions();
          if (options.size() > valueIndex)
            expression.m_value = options.at(valueIndex).value;
          break;
        }

        case SettingOptionsType::StaticTranslatable:
        {
          const TranslatableStringSettingOptions &options = referencedSettingString->GetTranslatableOptions();
          if (options.size() > valueIndex)
            expression.m_value = options.at(valueIndex).second;
          break;
        }

        default:
          break;
      }
    }

    // add the condition to the value of the referenced setting
    dependencyCombination->Add(boost::make_shared<CSettingDependencyCondition>(
        referencedSetting->GetId(), expression.m_value, expression.m_operator, expression.m_negated,
        GetSettingsManager()));
  }

  // if the condition doesn't depend on other settings it might be an infobool expression
  if (!error && dependencyCombination->GetOperations().empty() &&
      dependencyCombination->GetValues().empty())
    dependencyCombination->Add(boost::make_shared<CSettingDependencyCondition>(
        "InfoBool", condition, "", false, GetSettingsManager()));

  return !error;
}

bool CAddonSettings::ParseOldConditionExpression(std::string str, ConditionExpression& expression)
{
  StringUtils::Trim(str);

  size_t posOpen = str.find('(');
  size_t posSep = str.find(',', posOpen);
  size_t posClose = str.find(')', posSep);

  if (posOpen == std::string::npos || posSep == std::string::npos || posClose == std::string::npos)
    return false;

  std::string op = str.substr(0, posOpen);

  // check if the operator is negated
  expression.m_negated = StringUtils::StartsWith(op, "!");
  if (expression.m_negated)
    op = op.substr(1);

  // parse the operator
  if (StringUtils::EqualsNoCase(op, "eq"))
    expression.m_operator = SettingDependencyOperator::Equals;
  else if (StringUtils::EqualsNoCase(op, "gt"))
    expression.m_operator = SettingDependencyOperator::GreaterThan;
  else if (StringUtils::EqualsNoCase(op, "lt"))
    expression.m_operator = SettingDependencyOperator::LessThan;
  else
    return false;

  expression.m_relativeSettingIndex = static_cast<int32_t>(
      strtol(str.substr(posOpen + 1, posSep - posOpen - 1).c_str(), NULL, 10));
  expression.m_value = str.substr(posSep + 1, posClose - posSep - 1);

  return true;
}

void CAddonSettings::FileEnumSettingOptionsFiller(const boost::shared_ptr<const CSetting>& setting,
                                                  std::vector<StringSettingOption>& list,
                                                  std::string& current,
                                                  void* data)
{
  if (setting == NULL)
    return;

  boost::shared_ptr<const CSettingPath> settingPath = boost::dynamic_pointer_cast<const CSettingPath>(setting);
  if (settingPath == NULL)
    return;

  if (settingPath->GetSources().empty())
    return;

  const std::string& masking = settingPath->GetMasking();

  // fetch the matching files/directories
  CFileItemList items;
  XFILE::CDirectory::GetDirectory(settingPath->GetSources().front(), items, masking,
                                  XFILE::DIR_FLAG_NO_FILE_DIRS);

  // process the matching files/directories
  for (int i = 0; i < items.Size(); ++i)
  {
    const CFileItemPtr &item = items[i];
    if ((masking == "/" && item->m_bIsFolder) || !item->m_bIsFolder)
    {
      if (settingPath->HideExtension())
        item->RemoveExtension();
      list.push_back(StringSettingOption(item->GetLabel(), item->GetLabel()));
    }
  }
}

} // namespace ADDON
