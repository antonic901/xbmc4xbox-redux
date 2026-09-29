/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Settings.h"

#include "settings/SettingsBase.h"
#include "settings/lib/Setting.h"
#include "utils/Variant.h"

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <iterator>

namespace XBMCAddon
{
namespace xbmcaddon
{

template<class TSetting>
bool GetSettingValue(const boost::shared_ptr<CSettingsBase>& settings,
                     const std::string& key,
                     typename TSetting::Value& value)
{
  if (key.empty() || !settings->IsLoaded())
    return false;

  boost::shared_ptr<CSetting> setting = settings->GetSetting(key);
  if (setting == NULL || setting->GetType() != TSetting::Type())
    return false;

  value = boost::static_pointer_cast<TSetting>(setting)->GetValue();
  return true;
}

template<class TSetting>
bool GetSettingValueList(const boost::shared_ptr<CSettingsBase>& settings,
                         const std::string& key,
                         boost::function<typename TSetting::Value(CVariant)> transform,
                         std::vector<typename TSetting::Value>& values)
{
  if (key.empty() || !settings->IsLoaded())
    return false;

  boost::shared_ptr<CSetting> setting = settings->GetSetting(key);
  if (setting == NULL || setting->GetType() != SettingType::List ||
      boost::static_pointer_cast<CSettingList>(setting)->GetElementType() != TSetting::Type())
    return false;

  const std::vector<CVariant> variantValues = settings->GetList(key);
  std::transform(variantValues.begin(), variantValues.end(), std::back_inserter(values), transform);
  return true;
}

template<class TSetting>
bool SetSettingValue(const boost::shared_ptr<CSettingsBase>& settings,
                     const std::string& key,
                     typename TSetting::Value value)
{
  if (key.empty() || !settings->IsLoaded())
    return false;

  // try to get the setting
  boost::shared_ptr<CSetting> setting = settings->GetSetting(key);
  if (setting == NULL || setting->GetType() != TSetting::Type())
    return false;

  return boost::static_pointer_cast<TSetting>(setting)->SetValue(value);
}

template<class TSetting>
bool SetSettingValueList(const boost::shared_ptr<CSettingsBase>& settings,
                         const std::string& key,
                         const std::vector<typename TSetting::Value>& values)
{
  if (key.empty() || !settings->IsLoaded())
    return false;

  // try to get the setting
  boost::shared_ptr<CSetting> setting = settings->GetSetting(key);
  if (setting == NULL || setting->GetType() != SettingType::List ||
      boost::static_pointer_cast<CSettingList>(setting)->GetElementType() != TSetting::Type())
    return false;

  std::vector<CVariant> variantValues;
  variantValues.reserve(values.size());
  for (typename std::vector<typename TSetting::Value>::const_iterator it = values.begin(); it != values.end(); ++it)
    variantValues.push_back(CVariant(static_cast<typename TSetting::Value>(*it)));

  return settings->SetList(key, variantValues);
}

Settings::Settings(boost::shared_ptr<CSettingsBase> settings) : settings(settings)
{
}

bool Settings::getBool(const char* id)
{
  bool value = false;
  if (!GetSettingValue<CSettingBool>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"boolean\" for \"%s\"", id);

  return value;
}

int Settings::getInt(const char* id)
{
  int value = 0;
  if (!GetSettingValue<CSettingInt>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"integer\" for \"%s\"", id);

  return value;
}

double Settings::getNumber(const char* id)
{
  double value = 0.0;
  if (!GetSettingValue<CSettingNumber>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"number\" for \"%s\"", id);

  return value;
}

String Settings::getString(const char* id)
{
  std::string value;
  if (!GetSettingValue<CSettingString>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"string\" for \"%s\"", id);

  return value;
}

std::vector<bool> Settings::getBoolList(const char* id)
{
  const boost::function<bool(const CVariant&)> transform = boost::bind(&CVariant::asBoolean, _1, false);
  std::vector<bool> values;
  if (!GetSettingValueList<CSettingBool>(settings, id, transform, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[boolean]\" for \"%s\"", id);

  return values;
}

std::vector<int> Settings::getIntList(const char* id)
{
  const boost::function<int(const CVariant&)> transform = boost::bind(&CVariant::asInteger32, _1, 0);
  std::vector<int> values;
  if (!GetSettingValueList<CSettingInt>(settings, id, transform, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[integer]\" for \"%s\"", id);

  return values;
}

std::vector<double> Settings::getNumberList(const char* id)
{
  const boost::function<double(const CVariant&)> transform = boost::bind(&CVariant::asDouble, _1, 0.0);
  std::vector<double> values;
  if (!GetSettingValueList<CSettingNumber>(settings, id, transform, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[number]\" for \"%s\"", id);

  return values;
}

std::vector<String> Settings::getStringList(const char* id)
{
  const boost::function<std::string(const CVariant&)> transform = boost::bind(&CVariant::asString, _1, std::string());
  std::vector<std::string> values;
  if (!GetSettingValueList<CSettingString>(settings, id, transform, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[string]\" for \"%s\"", id);

  return values;
}

void Settings::setBool(const char* id, bool value)
{
  if (!SetSettingValue<CSettingBool>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"boolean\" for \"%s\"", id);
  settings->Save();
}

void Settings::setInt(const char* id, int value)
{
  if (!SetSettingValue<CSettingInt>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"integer\" for \"%s\"", id);
  settings->Save();
}

void Settings::setNumber(const char* id, double value)
{
  if (!SetSettingValue<CSettingNumber>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"number\" for \"%s\"", id);
  settings->Save();
}

void Settings::setString(const char* id, const String& value)
{
  if (!SetSettingValue<CSettingString>(settings, id, value))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"string\" for \"%s\"", id);
  settings->Save();
}

void Settings::setBoolList(const char* id, const std::vector<bool>& values)
{
  if (!SetSettingValueList<CSettingBool>(settings, id, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[boolean]\" for \"%s\"", id);
  settings->Save();
}

void Settings::setIntList(const char* id, const std::vector<int>& values)
{
  if (!SetSettingValueList<CSettingInt>(settings, id, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[integer]\" for \"%s\"", id);
  settings->Save();
}

void Settings::setNumberList(const char* id, const std::vector<double>& values)
{
  if (!SetSettingValueList<CSettingNumber>(settings, id, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[number]\" for \"%s\"", id);
  settings->Save();
}

void Settings::setStringList(const char* id, const std::vector<String>& values)
{
  if (!SetSettingValueList<CSettingString>(settings, id, values))
    throw XBMCAddon::WrongTypeException("Invalid setting type \"list[string]\" for \"%s\"", id);
  settings->Save();
}

} // namespace xbmcaddon
} // namespace XBMCAddon
