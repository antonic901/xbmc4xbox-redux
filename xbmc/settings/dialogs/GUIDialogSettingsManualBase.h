/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "settings/dialogs/GUIDialogSettingsManagerBase.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingLevel.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class CSetting;
class CSettingAction;
class CSettingAddon;
class CSettingBool;
class CSettingCategory;
class CSettingDate;
class CSettingGroup;
class CSettingInt;
class CSettingList;
class CSettingNumber;
class CSettingPath;
class CSettingSection;
class CSettingString;
class CSettingsManager;
class CSettingTime;

class CGUIDialogSettingsManualBase : public CGUIDialogSettingsManagerBase
{
public:
  CGUIDialogSettingsManualBase(int windowId, const std::string &xmlFile);
  virtual ~CGUIDialogSettingsManualBase();

protected:
  // implementation of CGUIDialogSettingsBase
  virtual boost::shared_ptr<CSettingSection> GetSection() { return m_section; }
  virtual void SetupView();

  // implementation of CGUIDialogSettingsManagerBase
  virtual CSettingsManager* GetSettingsManager() const;

  virtual void InitializeSettings();

  boost::shared_ptr<CSettingCategory> AddCategory(const std::string &id, int label, int help = -1);
  boost::shared_ptr<CSettingGroup> AddGroup(const boost::shared_ptr<CSettingCategory>& category,
                                          int label = -1,
                                          int help = -1,
                                          bool separatorBelowLabel = true,
                                          bool hideSeparator = false);
  // checkmark control
  boost::shared_ptr<CSettingBool> AddToggle(const boost::shared_ptr<CSettingGroup>& group,
                                          const std::string& id,
                                          int label,
                                          SettingLevel::Type level,
                                          bool value,
                                          bool delayed = false,
                                          bool visible = true,
                                          int help = -1);
  // edit controls
  boost::shared_ptr<CSettingInt> AddEdit(const boost::shared_ptr<CSettingGroup>& group,
                                       const std::string& id,
                                       int label,
                                       SettingLevel::Type level,
                                       int value,
                                       int minimum = 0,
                                       int step = 1,
                                       int maximum = 0,
                                       bool verifyNewValue = false,
                                       int heading = -1,
                                       bool delayed = false,
                                       bool visible = true,
                                       int help = -1);
  boost::shared_ptr<CSettingNumber> AddEdit(const boost::shared_ptr<CSettingGroup>& group,
                                          const std::string& id,
                                          int label,
                                          SettingLevel::Type level,
                                          float value,
                                          float minimum = 0.0f,
                                          float step = 1.0f,
                                          float maximum = 0.0f,
                                          bool verifyNewValue = false,
                                          int heading = -1,
                                          bool delayed = false,
                                          bool visible = true,
                                          int help = -1);
  boost::shared_ptr<CSettingString> AddEdit(const boost::shared_ptr<CSettingGroup>& group,
                                          const std::string& id,
                                          int label,
                                          SettingLevel::Type level,
                                          const std::string& value,
                                          bool allowEmpty = false,
                                          bool hidden = false,
                                          int heading = -1,
                                          bool delayed = false,
                                          bool visible = true,
                                          int help = -1);
  boost::shared_ptr<CSettingString> AddIp(const boost::shared_ptr<CSettingGroup>& group,
                                        const std::string& id,
                                        int label,
                                        SettingLevel::Type level,
                                        const std::string& value,
                                        bool allowEmpty = false,
                                        int heading = -1,
                                        bool delayed = false,
                                        bool visible = true,
                                        int help = -1);
  boost::shared_ptr<CSettingString> AddPasswordMd5(const boost::shared_ptr<CSettingGroup>& group,
                                                 const std::string& id,
                                                 int label,
                                                 SettingLevel::Type level,
                                                 const std::string& value,
                                                 bool allowEmpty = false,
                                                 int heading = -1,
                                                 bool delayed = false,
                                                 bool visible = true,
                                                 int help = -1);
  // button controls
  boost::shared_ptr<CSettingAction> AddButton(const boost::shared_ptr<CSettingGroup>& group,
                                            const std::string& id,
                                            int label,
                                            SettingLevel::Type level,
                                            const std::string& data = "",
                                            bool delayed = false,
                                            bool visible = true,
                                            int help = -1);
  boost::shared_ptr<CSettingString> AddInfoLabelButton(const boost::shared_ptr<CSettingGroup>& group,
                                                     const std::string& id,
                                                     int label,
                                                     SettingLevel::Type level,
                                                     const std::string& info,
                                                     bool visible = true,
                                                     int help = -1);
  boost::shared_ptr<CSettingAddon> AddAddon(const boost::shared_ptr<CSettingGroup>& group,
                                          const std::string& id,
                                          int label,
                                          SettingLevel::Type level,
                                          const std::string& value,
                                          ADDON::TYPE addonType,
                                          bool allowEmpty = false,
                                          int heading = -1,
                                          bool hideValue = false,
                                          bool showInstalledAddons = true,
                                          bool showInstallableAddons = false,
                                          bool showMoreAddons = true,
                                          bool delayed = false,
                                          bool visible = true,
                                          int help = -1);
  boost::shared_ptr<CSettingPath> AddPath(
      const boost::shared_ptr<CSettingGroup>& group,
      const std::string& id,
      int label,
      SettingLevel::Type level,
      const std::string& value,
      bool writable = true,
      const std::vector<std::string>& sources = std::vector<std::string>(),
      bool allowEmpty = false,
      int heading = -1,
      bool hideValue = false,
      bool delayed = false,
      bool visible = true,
      int help = -1);
  boost::shared_ptr<CSettingDate> AddDate(const boost::shared_ptr<CSettingGroup>& group,
                                        const std::string& id,
                                        int label,
                                        SettingLevel::Type level,
                                        const std::string& value,
                                        bool allowEmpty = false,
                                        int heading = -1,
                                        bool delayed = false,
                                        bool visible = true,
                                        int help = -1);
  boost::shared_ptr<CSettingTime> AddTime(const boost::shared_ptr<CSettingGroup>& group,
                                        const std::string& id,
                                        int label,
                                        SettingLevel::Type level,
                                        const std::string& value,
                                        bool allowEmpty = false,
                                        int heading = -1,
                                        bool delayed = false,
                                        bool visible = true,
                                        int help = -1);

  // spinner controls
  boost::shared_ptr<CSettingString> AddSpinner(const boost::shared_ptr<CSettingGroup>& group,
                                             const std::string& id,
                                             int label,
                                             SettingLevel::Type level,
                                             const std::string& value,
                                             StringSettingOptionsFiller filler,
                                             bool delayed = false,
                                             bool visible = true,
                                             int help = -1);
  boost::shared_ptr<CSettingInt> AddSpinner(const boost::shared_ptr<CSettingGroup>& group,
                                          const std::string& id,
                                          int label,
                                          SettingLevel::Type level,
                                          int value,
                                          int minimum,
                                          int step,
                                          int maximum,
                                          int formatLabel = -1,
                                          int minimumLabel = -1,
                                          bool delayed = false,
                                          bool visible = true,
                                          int help = -1);
  boost::shared_ptr<CSettingInt> AddSpinner(const boost::shared_ptr<CSettingGroup>& group,
                                          const std::string& id,
                                          int label,
                                          SettingLevel::Type level,
                                          int value,
                                          int minimum,
                                          int step,
                                          int maximum,
                                          const std::string& formatString,
                                          int minimumLabel = -1,
                                          bool delayed = false,
                                          bool visible = true,
                                          int help = -1);
  boost::shared_ptr<CSettingInt> AddSpinner(const boost::shared_ptr<CSettingGroup>& group,
                                          const std::string& id,
                                          int label,
                                          SettingLevel::Type level,
                                          int value,
                                          const TranslatableIntegerSettingOptions& entries,
                                          bool delayed = false,
                                          bool visible = true,
                                          int help = -1);
  boost::shared_ptr<CSettingInt> AddSpinner(const boost::shared_ptr<CSettingGroup>& group,
                                          const std::string& id,
                                          int label,
                                          SettingLevel::Type level,
                                          int value,
                                          const IntegerSettingOptions& entries,
                                          bool delayed = false,
                                          bool visible = true,
                                          int help = -1);
  boost::shared_ptr<CSettingInt> AddSpinner(const boost::shared_ptr<CSettingGroup>& group,
                                          const std::string& id,
                                          int label,
                                          SettingLevel::Type level,
                                          int value,
                                          IntegerSettingOptionsFiller filler,
                                          bool delayed = false,
                                          bool visible = true,
                                          int help = -1);
  boost::shared_ptr<CSettingNumber> AddSpinner(const boost::shared_ptr<CSettingGroup>& group,
                                             const std::string& id,
                                             int label,
                                             SettingLevel::Type level,
                                             float value,
                                             float minimum,
                                             float step,
                                             float maximum,
                                             int formatLabel = -1,
                                             int minimumLabel = -1,
                                             bool delayed = false,
                                             bool visible = true,
                                             int help = -1);
  boost::shared_ptr<CSettingNumber> AddSpinner(const boost::shared_ptr<CSettingGroup>& group,
                                             const std::string& id,
                                             int label,
                                             SettingLevel::Type level,
                                             float value,
                                             float minimum,
                                             float step,
                                             float maximum,
                                             const std::string& formatString,
                                             int minimumLabel = -1,
                                             bool delayed = false,
                                             bool visible = true,
                                             int help = -1);

  // list controls
  boost::shared_ptr<CSettingString> AddList(const boost::shared_ptr<CSettingGroup>& group,
                                          const std::string& id,
                                          int label,
                                          SettingLevel::Type level,
                                          const std::string& value,
                                          StringSettingOptionsFiller filler,
                                          int heading,
                                          bool visible = true,
                                          int help = -1,
                                          bool details = false);
  boost::shared_ptr<CSettingInt> AddList(const boost::shared_ptr<CSettingGroup>& group,
                                       const std::string& id,
                                       int label,
                                       SettingLevel::Type level,
                                       int value,
                                       const TranslatableIntegerSettingOptions& entries,
                                       int heading,
                                       bool visible = true,
                                       int help = -1,
                                       bool details = false);
  boost::shared_ptr<CSettingInt> AddList(const boost::shared_ptr<CSettingGroup>& group,
                                       const std::string& id,
                                       int label,
                                       SettingLevel::Type level,
                                       int value,
                                       const IntegerSettingOptions& entries,
                                       int heading,
                                       bool visible = true,
                                       int help = -1,
                                       bool details = false);
  boost::shared_ptr<CSettingInt> AddList(const boost::shared_ptr<CSettingGroup>& group,
                                       const std::string& id,
                                       int label,
                                       SettingLevel::Type level,
                                       int value,
                                       IntegerSettingOptionsFiller filler,
                                       int heading,
                                       bool visible = true,
                                       int help = -1,
                                       bool details = false);
  boost::shared_ptr<CSettingList> AddList(const boost::shared_ptr<CSettingGroup>& group,
                                        const std::string& id,
                                        int label,
                                        SettingLevel::Type level,
                                        std::vector<std::string> values,
                                        StringSettingOptionsFiller filler,
                                        int heading,
                                        int minimumItems = 0,
                                        int maximumItems = -1,
                                        bool visible = true,
                                        int help = -1,
                                        bool details = false);
  boost::shared_ptr<CSettingList> AddList(const boost::shared_ptr<CSettingGroup>& group,
                                        const std::string& id,
                                        int label,
                                        SettingLevel::Type level,
                                        std::vector<int> values,
                                        const TranslatableIntegerSettingOptions& entries,
                                        int heading,
                                        int minimumItems = 0,
                                        int maximumItems = -1,
                                        bool visible = true,
                                        int help = -1,
                                        bool details = false);
  boost::shared_ptr<CSettingList> AddList(const boost::shared_ptr<CSettingGroup>& group,
                                        const std::string& id,
                                        int label,
                                        SettingLevel::Type level,
                                        std::vector<int> values,
                                        const IntegerSettingOptions& entries,
                                        int heading,
                                        int minimumItems = 0,
                                        int maximumItems = -1,
                                        bool visible = true,
                                        int help = -1,
                                        bool details = false);
  boost::shared_ptr<CSettingList> AddList(const boost::shared_ptr<CSettingGroup>& group,
                                        const std::string& id,
                                        int label,
                                        SettingLevel::Type level,
                                        std::vector<int> values,
                                        IntegerSettingOptionsFiller filler,
                                        int heading,
                                        int minimumItems = 0,
                                        int maximumItems = -1,
                                        bool visible = true,
                                        int help = -1,
                                        SettingControlListValueFormatter formatter = NULL,
                                        bool details = false);

  // slider controls
  boost::shared_ptr<CSettingInt> AddPercentageSlider(const boost::shared_ptr<CSettingGroup>& group,
                                                   const std::string& id,
                                                   int label,
                                                   SettingLevel::Type level,
                                                   int value,
                                                   int formatLabel,
                                                   int step = 1,
                                                   int heading = -1,
                                                   bool usePopup = false,
                                                   bool delayed = false,
                                                   bool visible = true,
                                                   int help = -1);
  boost::shared_ptr<CSettingInt> AddPercentageSlider(const boost::shared_ptr<CSettingGroup>& group,
                                                   const std::string& id,
                                                   int label,
                                                   SettingLevel::Type level,
                                                   int value,
                                                   const std::string& formatString,
                                                   int step = 1,
                                                   int heading = -1,
                                                   bool usePopup = false,
                                                   bool delayed = false,
                                                   bool visible = true,
                                                   int help = -1);
  boost::shared_ptr<CSettingInt> AddSlider(const boost::shared_ptr<CSettingGroup>& group,
                                         const std::string& id,
                                         int label,
                                         SettingLevel::Type level,
                                         int value,
                                         int formatLabel,
                                         int minimum,
                                         int step,
                                         int maximum,
                                         int heading = -1,
                                         bool usePopup = false,
                                         bool delayed = false,
                                         bool visible = true,
                                         int help = -1);
  boost::shared_ptr<CSettingInt> AddSlider(const boost::shared_ptr<CSettingGroup>& group,
                                         const std::string& id,
                                         int label,
                                         SettingLevel::Type level,
                                         int value,
                                         const std::string& formatString,
                                         int minimum,
                                         int step,
                                         int maximum,
                                         int heading = -1,
                                         bool usePopup = false,
                                         bool delayed = false,
                                         bool visible = true,
                                         int help = -1);
  boost::shared_ptr<CSettingNumber> AddSlider(const boost::shared_ptr<CSettingGroup>& group,
                                            const std::string& id,
                                            int label,
                                            SettingLevel::Type level,
                                            float value,
                                            int formatLabel,
                                            float minimum,
                                            float step,
                                            float maximum,
                                            int heading = -1,
                                            bool usePopup = false,
                                            bool delayed = false,
                                            bool visible = true,
                                            int help = -1);
  boost::shared_ptr<CSettingNumber> AddSlider(const boost::shared_ptr<CSettingGroup>& group,
                                            const std::string& id,
                                            int label,
                                            SettingLevel::Type level,
                                            float value,
                                            const std::string& formatString,
                                            float minimum,
                                            float step,
                                            float maximum,
                                            int heading = -1,
                                            bool usePopup = false,
                                            bool delayed = false,
                                            bool visible = true,
                                            int help = -1);

  // range controls
  boost::shared_ptr<CSettingList> AddPercentageRange(const boost::shared_ptr<CSettingGroup>& group,
                                                   const std::string& id,
                                                   int label,
                                                   SettingLevel::Type level,
                                                   int valueLower,
                                                   int valueUpper,
                                                   int valueFormatLabel,
                                                   int step = 1,
                                                   int formatLabel = 21469,
                                                   bool delayed = false,
                                                   bool visible = true,
                                                   int help = -1);
  boost::shared_ptr<CSettingList> AddPercentageRange(const boost::shared_ptr<CSettingGroup>& group,
                                                   const std::string& id,
                                                   int label,
                                                   SettingLevel::Type level,
                                                   int valueLower,
                                                   int valueUpper,
                                                   const std::string& valueFormatString = "{:d} %",
                                                   int step = 1,
                                                   int formatLabel = 21469,
                                                   bool delayed = false,
                                                   bool visible = true,
                                                   int help = -1);
  boost::shared_ptr<CSettingList> AddRange(const boost::shared_ptr<CSettingGroup>& group,
                                         const std::string& id,
                                         int label,
                                         SettingLevel::Type level,
                                         int valueLower,
                                         int valueUpper,
                                         int minimum,
                                         int step,
                                         int maximum,
                                         int valueFormatLabel,
                                         int formatLabel = 21469,
                                         bool delayed = false,
                                         bool visible = true,
                                         int help = -1);
  boost::shared_ptr<CSettingList> AddRange(const boost::shared_ptr<CSettingGroup>& group,
                                         const std::string& id,
                                         int label,
                                         SettingLevel::Type level,
                                         int valueLower,
                                         int valueUpper,
                                         int minimum,
                                         int step,
                                         int maximum,
                                         const std::string& valueFormatString = "{:d}",
                                         int formatLabel = 21469,
                                         bool delayed = false,
                                         bool visible = true,
                                         int help = -1);
  boost::shared_ptr<CSettingList> AddRange(const boost::shared_ptr<CSettingGroup>& group,
                                         const std::string& id,
                                         int label,
                                         SettingLevel::Type level,
                                         float valueLower,
                                         float valueUpper,
                                         float minimum,
                                         float step,
                                         float maximum,
                                         int valueFormatLabel,
                                         int formatLabel = 21469,
                                         bool delayed = false,
                                         bool visible = true,
                                         int help = -1);
  boost::shared_ptr<CSettingList> AddRange(const boost::shared_ptr<CSettingGroup>& group,
                                         const std::string& id,
                                         int label,
                                         SettingLevel::Type level,
                                         float valueLower,
                                         float valueUpper,
                                         float minimum,
                                         float step,
                                         float maximum,
                                         const std::string& valueFormatString = "{:.1f}",
                                         int formatLabel = 21469,
                                         bool delayed = false,
                                         bool visible = true,
                                         int help = -1);
  boost::shared_ptr<CSettingList> AddDateRange(const boost::shared_ptr<CSettingGroup>& group,
                                             const std::string& id,
                                             int label,
                                             SettingLevel::Type level,
                                             int valueLower,
                                             int valueUpper,
                                             int minimum,
                                             int step,
                                             int maximum,
                                             int valueFormatLabel,
                                             int formatLabel = 21469,
                                             bool delayed = false,
                                             bool visible = true,
                                             int help = -1);
  boost::shared_ptr<CSettingList> AddDateRange(const boost::shared_ptr<CSettingGroup>& group,
                                             const std::string& id,
                                             int label,
                                             SettingLevel::Type level,
                                             int valueLower,
                                             int valueUpper,
                                             int minimum,
                                             int step,
                                             int maximum,
                                             const std::string& valueFormatString = "",
                                             int formatLabel = 21469,
                                             bool delayed = false,
                                             bool visible = true,
                                             int help = -1);
  boost::shared_ptr<CSettingList> AddTimeRange(const boost::shared_ptr<CSettingGroup>& group,
                                             const std::string& id,
                                             int label,
                                             SettingLevel::Type level,
                                             int valueLower,
                                             int valueUpper,
                                             int minimum,
                                             int step,
                                             int maximum,
                                             int valueFormatLabel,
                                             int formatLabel = 21469,
                                             bool delayed = false,
                                             bool visible = true,
                                             int help = -1);
  boost::shared_ptr<CSettingList> AddTimeRange(const boost::shared_ptr<CSettingGroup>& group,
                                             const std::string& id,
                                             int label,
                                             SettingLevel::Type level,
                                             int valueLower,
                                             int valueUpper,
                                             int minimum,
                                             int step,
                                             int maximum,
                                             const std::string& valueFormatString = "mm:ss",
                                             int formatLabel = 21469,
                                             bool delayed = false,
                                             bool visible = true,
                                             int help = -1);

  boost::shared_ptr<ISettingControl> GetTitleControl(bool separatorBelowLabel = true, bool hideSeparator = false);
  boost::shared_ptr<ISettingControl> GetCheckmarkControl(bool delayed = false);
  boost::shared_ptr<ISettingControl> GetEditControl(const std::string &format, bool delayed = false, bool hidden = false, bool verifyNewValue = false, int heading = -1);
  boost::shared_ptr<ISettingControl> GetButtonControl(const std::string &format, bool delayed = false, int heading = -1, bool hideValue = false, bool showInstalledAddons = true,
    bool showInstallableAddons = false, bool showMoreAddons = true);
  boost::shared_ptr<ISettingControl> GetSpinnerControl(const std::string &format, bool delayed = false, int minimumLabel = -1, int formatLabel = -1, const std::string &formatString = "");
  boost::shared_ptr<ISettingControl> GetListControl(
      const std::string& format,
      bool delayed = false,
      int heading = -1,
      bool multiselect = false,
      SettingControlListValueFormatter formatter = NULL,
      bool details = false);
  boost::shared_ptr<ISettingControl> GetSliderControl(const std::string &format, bool delayed = false, int heading = -1, bool usePopup = false, int formatLabel = -1, const std::string &formatString = "");
  boost::shared_ptr<ISettingControl> GetRangeControl(const std::string &format, bool delayed = false, int formatLabel = -1, int valueFormatLabel = -1, const std::string &valueFormatString = "");

private:
  boost::shared_ptr<CSettingList> AddRange(const boost::shared_ptr<CSettingGroup>& group,
                                         const std::string& id,
                                         int label,
                                         SettingLevel::Type level,
                                         int valueLower,
                                         int valueUpper,
                                         int minimum,
                                         int step,
                                         int maximum,
                                         const std::string& format,
                                         int formatLabel,
                                         int valueFormatLabel,
                                         const std::string& valueFormatString,
                                         bool delayed,
                                         bool visible,
                                         int help);
  boost::shared_ptr<CSettingList> AddRange(const boost::shared_ptr<CSettingGroup>& group,
                                         const std::string& id,
                                         int label,
                                         SettingLevel::Type level,
                                         float valueLower,
                                         float valueUpper,
                                         float minimum,
                                         float step,
                                         float maximum,
                                         const std::string& format,
                                         int formatLabel,
                                         int valueFormatLabel,
                                         const std::string& valueFormatString,
                                         bool delayed,
                                         bool visible,
                                         int help);

  void setSettingDetails(const boost::shared_ptr<CSetting>& setting,
                         SettingLevel::Type level,
                         bool visible,
                         int help);

  mutable CSettingsManager* m_settingsManager;
  boost::shared_ptr<CSettingSection> m_section;
};
