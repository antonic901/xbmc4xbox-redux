/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ISetting.h"
#include "ISettingCallback.h"
#include "ISettingControl.h"
#include "SettingDefinitions.h"
#include "SettingDependency.h"
#include "SettingLevel.h"
#include "SettingType.h"
#include "SettingUpdate.h"
#include "threads/SharedSection.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <boost/move/move.hpp>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace SettingOptionsType
{
  enum Type {
    Unknown = 0,
    StaticTranslatable,
    Static,
    Dynamic
  };
}

class CSetting;
typedef boost::shared_ptr<CSetting> SettingPtr;
typedef boost::shared_ptr<const CSetting> SettingConstPtr;
typedef std::vector<SettingPtr> SettingList;

/*!
 \ingroup settings
 \brief Setting base class containing all the properties which are common to
 all settings independent of the setting type.
 */
class CSetting : public ISetting,
                 protected ISettingCallback,
                 public boost::enable_shared_from_this<CSetting>
{
public:
  CSetting(const std::string& id, CSettingsManager* settingsManager = NULL);
  CSetting(const std::string& id, const CSetting& setting);
  virtual ~CSetting() {}

  virtual boost::shared_ptr<CSetting> Clone(const std::string &id) const = 0;
  void MergeBasics(const CSetting& other);
  virtual void MergeDetails(const CSetting& other) = 0;

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual SettingType::Type GetType() const = 0;
  virtual bool FromString(const std::string &value) = 0;
  virtual std::string ToString() const = 0;
  virtual bool Equals(const std::string &value) const = 0;
  virtual bool CheckValidity(const std::string &value) const = 0;
  virtual void Reset() = 0;

  bool IsEnabled() const;
  bool GetEnabled() const { return m_enabled; }
  void SetEnabled(bool enabled);
  bool IsDefault() const { return !m_changed; }
  const std::string& GetParent() const { return m_parentSetting; }
  void SetParent(const std::string& parentSetting) { m_parentSetting = parentSetting; }
  SettingLevel::Type GetLevel() const { return m_level; }
  void SetLevel(SettingLevel::Type level) { m_level = level; }
  boost::shared_ptr<const ISettingControl> GetControl() const { return m_control; }
  boost::shared_ptr<ISettingControl> GetControl() { return m_control; }
  void SetControl(boost::shared_ptr<ISettingControl> control) { m_control = boost::move(control); }
  const SettingDependencies& GetDependencies() const { return m_dependencies; }
  void SetDependencies(const SettingDependencies &dependencies) { m_dependencies = dependencies; }
  const std::set<CSettingUpdate>& GetUpdates() const { return m_updates; }

  void SetCallback(ISettingCallback *callback) { m_callback = callback; }

  bool IsReference() const { return !m_referencedId.empty(); }
  const std::string& GetReferencedId() const { return m_referencedId; }
  void SetReferencedId(const std::string& referencedId) { m_referencedId = referencedId; }
  void MakeReference(const std::string& referencedId = "");

  bool GetVisible() const { return ISetting::IsVisible(); }
  // overrides of ISetting
  virtual bool IsVisible() const;

  // implementation of ISettingCallback
  virtual void OnSettingAction(const boost::shared_ptr<const CSetting>& setting);

  /*!
   \brief Deserializes the given XML node to retrieve a setting object's identifier and
          whether the setting is a reference to another setting or not.

   \param node XML node containing a setting object's identifier
   \param identification Will contain the deserialized setting object's identifier
   \param isReference Whether the setting is a reference to the setting with the determined identifier
   \return True if a setting object's identifier was deserialized, false otherwise
   */
  static bool DeserializeIdentification(const TiXmlNode* node,
                                        std::string& identification,
                                        bool& isReference);

protected:
  // implementation of ISettingCallback
  virtual bool OnSettingChanging(const boost::shared_ptr<const CSetting>& setting);
  virtual void OnSettingChanged(const boost::shared_ptr<const CSetting>& setting);
  virtual bool OnSettingUpdate(const boost::shared_ptr<CSetting>& setting,
                       const char* oldSettingId,
                       const TiXmlNode* oldSettingNode);
  virtual void OnSettingPropertyChanged(const boost::shared_ptr<const CSetting>& setting,
                                const char* propertyName);

  void Copy(const CSetting &setting);

  template<class TSetting>
  boost::shared_ptr<TSetting> shared_from_base()
  {
    return boost::static_pointer_cast<TSetting>(shared_from_this());
  }

  ISettingCallback *m_callback;
  bool m_enabled;
  std::string m_parentSetting;
  SettingLevel::Type m_level;
  boost::shared_ptr<ISettingControl> m_control;
  SettingDependencies m_dependencies;
  std::set<CSettingUpdate> m_updates;
  bool m_changed;
  mutable CSharedSection m_critical;

  std::string m_referencedId;
};

template<typename TValue, SettingType::Type TSettingType>
class CTraitedSetting : public CSetting
{
public:
  typedef TValue Value;

  // implementation of CSetting
  virtual SettingType::Type GetType() const { return TSettingType; }

  static SettingType::Type Type() { return TSettingType; }

protected:
  CTraitedSetting(const std::string& id, CSettingsManager* settingsManager = NULL)
    : CSetting(id, settingsManager)
  { }
  CTraitedSetting(const std::string& id, const CTraitedSetting& setting) : CSetting(id, setting) {}
  virtual ~CTraitedSetting() {}
};

/*!
 \ingroup settings
 \brief List setting implementation
 \sa CSetting
 */
class CSettingList : public CSetting
{
public:
  CSettingList(const std::string &id, boost::shared_ptr<CSetting> settingDefinition, CSettingsManager *settingsManager = NULL);
  CSettingList(const std::string &id, boost::shared_ptr<CSetting> settingDefinition, int label, CSettingsManager *settingsManager = NULL);
  CSettingList(const std::string &id, const CSettingList &setting);
  virtual ~CSettingList() {}

  virtual boost::shared_ptr<CSetting> Clone(const std::string &id) const;
  virtual void MergeDetails(const CSetting& other);

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual SettingType::Type GetType() const { return SettingType::List; }
  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual void Reset();

  SettingType::Type GetElementType() const;
  boost::shared_ptr<CSetting> GetDefinition() { return m_definition; }
  boost::shared_ptr<const CSetting> GetDefinition() const { return m_definition; }
  void SetDefinition(boost::shared_ptr<CSetting> definition) { m_definition = boost::move(definition); }

  const std::string& GetDelimiter() const { return m_delimiter; }
  void SetDelimiter(const std::string &delimiter) { m_delimiter = delimiter; }
  int GetMinimumItems() const { return m_minimumItems; }
  void SetMinimumItems(int minimumItems) { m_minimumItems = minimumItems; }
  int GetMaximumItems() const { return m_maximumItems; }
  void SetMaximumItems(int maximumItems) { m_maximumItems = maximumItems; }

  bool FromString(const std::vector<std::string> &value);

  const SettingList& GetValue() const { return m_values; }
  bool SetValue(const SettingList &values);
  const SettingList& GetDefault() const { return m_defaults; }
  void SetDefault(const SettingList &values);

protected:
  void copy(const CSettingList &setting);
  static void copy(const SettingList &srcValues, SettingList &dstValues);
  bool fromString(const std::string &strValue, SettingList &values) const;
  bool fromValues(const std::vector<std::string> &strValues, SettingList &values) const;
  std::string toString(const SettingList &values) const;

  SettingList m_values;
  SettingList m_defaults;
  boost::shared_ptr<CSetting> m_definition;
  std::string m_delimiter;
  int m_minimumItems;
  int m_maximumItems;
};

/*!
 \ingroup settings
 \brief Boolean setting implementation.
 \sa CSetting
 */
class CSettingBool : public CTraitedSetting<bool, SettingType::Boolean>
{
public:
  CSettingBool(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingBool(const std::string &id, const CSettingBool &setting);
  CSettingBool(const std::string &id, int label, bool value, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingBool() {}

  virtual boost::shared_ptr<CSetting> Clone(const std::string &id) const;
  virtual void MergeDetails(const CSetting& other);

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual void Reset() { SetValue(m_default); }

  bool GetValue() const
  {
    CSharedLock lock(m_critical);
    return m_value;
  }
  bool SetValue(bool value);
  bool GetDefault() const { return m_default; }
  void SetDefault(bool value);

private:
  static const Value DefaultValue;

  void copy(const CSettingBool &setting);
  bool fromString(const std::string &strValue, bool &value) const;

  bool m_value;
  bool m_default;
};

/*!
 \ingroup settings
 \brief Integer setting implementation
 \sa CSetting
 */
class CSettingInt : public CTraitedSetting<int, SettingType::Integer>
{
public:
  CSettingInt(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingInt(const std::string &id, const CSettingInt &setting);
  CSettingInt(const std::string &id, int label, int value, CSettingsManager *settingsManager = NULL);
  CSettingInt(const std::string &id, int label, int value, int minimum, int step, int maximum, CSettingsManager *settingsManager = NULL);
  CSettingInt(const std::string &id, int label, int value, const TranslatableIntegerSettingOptions &options, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingInt() {}

  virtual boost::shared_ptr<CSetting> Clone(const std::string &id) const;
  virtual void MergeDetails(const CSetting& other);

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual bool CheckValidity(int value) const;
  virtual void Reset() { SetValue(m_default); }

  int GetValue() const
  {
    CSharedLock lock(m_critical);
    return m_value;
  }
  bool SetValue(int value);
  int GetDefault() const { return m_default; }
  void SetDefault(int value);

  int GetMinimum() const { return m_min; }
  void SetMinimum(int minimum) { m_min = minimum; }
  int GetStep() const { return m_step; }
  void SetStep(int step) { m_step = step; }
  int GetMaximum() const { return m_max; }
  void SetMaximum(int maximum) { m_max = maximum; }

  SettingOptionsType::Type GetOptionsType() const;
  const TranslatableIntegerSettingOptions& GetTranslatableOptions() const { return m_translatableOptions; }
  void SetTranslatableOptions(const TranslatableIntegerSettingOptions &options) { m_translatableOptions = options; }
  const IntegerSettingOptions& GetOptions() const { return m_options; }
  void SetOptions(const IntegerSettingOptions &options) { m_options = options; }
  const std::string& GetOptionsFillerName() const { return m_optionsFillerName; }
  void SetOptionsFillerName(const std::string &optionsFillerName, void *data = NULL)
  {
    m_optionsFillerName = optionsFillerName;
    m_optionsFillerData = data;
  }
  void SetOptionsFiller(IntegerSettingOptionsFiller optionsFiller, void *data = NULL)
  {
    m_optionsFiller = optionsFiller;
    m_optionsFillerData = data;
  }
  IntegerSettingOptions GetDynamicOptions() const { return m_dynamicOptions; }
  IntegerSettingOptions UpdateDynamicOptions();
  SettingOptionsSort::Type GetOptionsSort() const { return m_optionsSort; }
  void SetOptionsSort(SettingOptionsSort::Type optionsSort) { m_optionsSort = optionsSort; }

private:
  static const Value DefaultValue;
  static const Value DefaultMin;
  static const Value DefaultStep;
  static const Value DefaultMax;

  void copy(const CSettingInt &setting);
  static bool fromString(const std::string &strValue, int &value);

  int m_value;
  int m_default;
  int m_min;
  int m_step;
  int m_max;
  TranslatableIntegerSettingOptions m_translatableOptions;
  IntegerSettingOptions m_options;
  std::string m_optionsFillerName;
  IntegerSettingOptionsFiller m_optionsFiller;
  void *m_optionsFillerData;
  IntegerSettingOptions m_dynamicOptions;
  SettingOptionsSort::Type m_optionsSort;
};

/*!
 \ingroup settings
 \brief Real number setting implementation.
 \sa CSetting
 */
class CSettingNumber : public CTraitedSetting<double, SettingType::Number>
{
public:
  CSettingNumber(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingNumber(const std::string &id, const CSettingNumber &setting);
  CSettingNumber(const std::string &id, int label, float value, CSettingsManager *settingsManager = NULL);
  CSettingNumber(const std::string &id, int label, float value, float minimum, float step, float maximum, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingNumber() {}

  virtual boost::shared_ptr<CSetting> Clone(const std::string &id) const;
  virtual void MergeDetails(const CSetting& other);

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual bool CheckValidity(double value) const;
  virtual void Reset() { SetValue(m_default); }

  double GetValue() const
  {
    CSharedLock lock(m_critical);
    return m_value;
  }
  bool SetValue(double value);
  double GetDefault() const { return m_default; }
  void SetDefault(double value);

  double GetMinimum() const { return m_min; }
  void SetMinimum(double minimum) { m_min = minimum; }
  double GetStep() const { return m_step; }
  void SetStep(double step) { m_step = step; }
  double GetMaximum() const { return m_max; }
  void SetMaximum(double maximum) { m_max = maximum; }

private:
  static const Value DefaultValue;
  static const Value DefaultMin;
  static const Value DefaultStep;
  static const Value DefaultMax;

  virtual void copy(const CSettingNumber &setting);
  static bool fromString(const std::string &strValue, double &value);

  double m_value;
  double m_default;
  double m_min;
  double m_step;
  double m_max;
};

/*!
 \ingroup settings
 \brief String setting implementation.
 \sa CSetting
 */
class CSettingString : public CTraitedSetting<std::string, SettingType::String>
{
public:
  CSettingString(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingString(const std::string &id, const CSettingString &setting);
  CSettingString(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingString() {}

  virtual boost::shared_ptr<CSetting> Clone(const std::string &id) const;
  virtual void MergeDetails(const CSetting& other);

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual bool FromString(const std::string &value) { return SetValue(value); }
  virtual std::string ToString() const { return m_value; }
  virtual bool Equals(const std::string &value) const { return m_value == value; }
  virtual bool CheckValidity(const std::string &value) const;
  virtual void Reset() { SetValue(m_default); }

  virtual const std::string& GetValue() const
  {
    CSharedLock lock(m_critical);
    return m_value;
  }
  virtual bool SetValue(const std::string &value);
  virtual const std::string& GetDefault() const { return m_default; }
  virtual void SetDefault(const std::string &value);

  virtual bool AllowEmpty() const { return m_allowEmpty; }
  void SetAllowEmpty(bool allowEmpty) { m_allowEmpty = allowEmpty; }
  virtual bool AllowNewOption() const { return m_allowNewOption; }
  void SetAllowNewOption(bool allowNewOption) { m_allowNewOption = allowNewOption; }

  SettingOptionsType::Type GetOptionsType() const;
  const TranslatableStringSettingOptions& GetTranslatableOptions() const { return m_translatableOptions; }
  void SetTranslatableOptions(const TranslatableStringSettingOptions &options) { m_translatableOptions = options; }
  const StringSettingOptions& GetOptions() const { return m_options; }
  void SetOptions(const StringSettingOptions &options) { m_options = options; }
  const std::string& GetOptionsFillerName() const { return m_optionsFillerName; }
  void SetOptionsFillerName(const std::string &optionsFillerName, void *data = NULL)
  {
    m_optionsFillerName = optionsFillerName;
    m_optionsFillerData = data;
  }
  void SetOptionsFiller(StringSettingOptionsFiller optionsFiller, void *data = NULL)
  {
    m_optionsFiller = optionsFiller;
    m_optionsFillerData = data;
  }
  StringSettingOptions GetDynamicOptions() const { return m_dynamicOptions; }
  StringSettingOptions UpdateDynamicOptions();
  SettingOptionsSort::Type GetOptionsSort() const { return m_optionsSort; }
  void SetOptionsSort(SettingOptionsSort::Type optionsSort) { m_optionsSort = optionsSort; }

protected:
  static const Value DefaultValue;

  virtual void copy(const CSettingString &setting);

  std::string m_value;
  std::string m_default;
  bool m_allowEmpty;
  bool m_allowNewOption;
  TranslatableStringSettingOptions m_translatableOptions;
  StringSettingOptions m_options;
  std::string m_optionsFillerName;
  StringSettingOptionsFiller m_optionsFiller;
  void *m_optionsFillerData;
  StringSettingOptions m_dynamicOptions;
  SettingOptionsSort::Type m_optionsSort;
};

/*!
 \ingroup settings
 \brief Action setting implementation.

 A setting action will trigger a call to the OnSettingAction() callback method
 when activated.

 \sa CSetting
 */
class CSettingAction : public CSetting
{
public:
  CSettingAction(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingAction(const std::string &id, int label, CSettingsManager *settingsManager = NULL);
  CSettingAction(const std::string &id, const CSettingAction &setting);
  virtual ~CSettingAction() {}

  virtual boost::shared_ptr<CSetting> Clone(const std::string &id) const;
  virtual void MergeDetails(const CSetting& other);

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual SettingType::Type GetType() const { return SettingType::Action; }
  virtual bool FromString(const std::string &value) { return CheckValidity(value); }
  virtual std::string ToString() const { return ""; }
  virtual bool Equals(const std::string &value) const { return value.empty(); }
  virtual bool CheckValidity(const std::string &value) const { return value.empty(); }
  virtual void Reset() { }

  bool HasData() const { return !m_data.empty(); }
  const std::string& GetData() const { return m_data; }
  void SetData(const std::string& data) { m_data = data; }

protected:
  virtual void copy(const CSettingAction& setting);

  std::string m_data;
};
