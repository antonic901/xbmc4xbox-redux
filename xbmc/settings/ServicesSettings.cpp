/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServicesSettings.h"

#include "filesystem/IFileTypes.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

using namespace XFILE;

void CServicesSettings::SettingOptionsBufferModesFiller(const SettingConstPtr& setting,
                                                        std::vector<IntegerSettingOption>& list,
                                                        int& current,
                                                        void* data)
{
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(37110).c_str(), CACHE_BUFFER_MODE_NONE));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(37111).c_str(), CACHE_BUFFER_MODE_TRUE_INTERNET));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(37112).c_str(), CACHE_BUFFER_MODE_INTERNET));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(37113).c_str(), CACHE_BUFFER_MODE_NETWORK));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(37114).c_str(), CACHE_BUFFER_MODE_ALL));
}

void CServicesSettings::SettingOptionsMemorySizesFiller(const SettingConstPtr& setting,
                                                        std::vector<IntegerSettingOption>& list,
                                                        int& current,
                                                        void* data)
{
  const std::string &mb = g_localizeStrings.Get(37122);
  const std::string &gb = g_localizeStrings.Get(37123);

  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 16), 16));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 20), 20));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 24), 24));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 32), 32));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 48), 48));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 64), 64));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 96), 96));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 128), 128));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 192), 192));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 256), 256));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 384), 384));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 512), 512));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 768), 768));
  list.push_back(IntegerSettingOption(StringUtils::Format(gb.c_str(), 1), 1024));
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(37115).c_str(), 0));
}

void CServicesSettings::SettingOptionsReadFactorsFiller(const SettingConstPtr& setting,
                                                        std::vector<IntegerSettingOption>& list,
                                                        int& current,
                                                        void* data)
{
  list.push_back(IntegerSettingOption(g_localizeStrings.Get(37116), 0));
  list.push_back(IntegerSettingOption("1.1x", 110));
  list.push_back(IntegerSettingOption("1.25x", 125));
  list.push_back(IntegerSettingOption("1.5x", 150));
  list.push_back(IntegerSettingOption("1.75x", 175));
  list.push_back(IntegerSettingOption("2x", 200));
  list.push_back(IntegerSettingOption("2.5x", 250));
  list.push_back(IntegerSettingOption("3x", 300));
  list.push_back(IntegerSettingOption("4x", 400));
  list.push_back(IntegerSettingOption("5x", 500));
  list.push_back(IntegerSettingOption("7x", 700));
  list.push_back(IntegerSettingOption("10x", 1000));
  list.push_back(IntegerSettingOption("15x", 1500));
  list.push_back(IntegerSettingOption("20x", 2000));
  list.push_back(IntegerSettingOption("30x", 3000));
  list.push_back(IntegerSettingOption("50x", 5000));
}

void CServicesSettings::SettingOptionsCacheChunkSizesFiller(const SettingConstPtr& setting,
                                                            std::vector<IntegerSettingOption>& list,
                                                            int& current,
                                                            void* data)
{
  const std::string &byte = g_localizeStrings.Get(37120);
  const std::string &kb = g_localizeStrings.Get(37121);
  const std::string &mb = g_localizeStrings.Get(37122);

  list.push_back(IntegerSettingOption(StringUtils::Format(byte.c_str(), 256), 256));
  list.push_back(IntegerSettingOption(StringUtils::Format(byte.c_str(), 512), 512));
  list.push_back(IntegerSettingOption(StringUtils::Format(kb.c_str(), 1), 1024));
  list.push_back(IntegerSettingOption(StringUtils::Format(kb.c_str(), 2), 2 * 1024));
  list.push_back(IntegerSettingOption(StringUtils::Format(kb.c_str(), 4), 4 * 1024));
  list.push_back(IntegerSettingOption(StringUtils::Format(kb.c_str(), 8), 8 * 1024));
  list.push_back(IntegerSettingOption(StringUtils::Format(kb.c_str(), 16), 16 * 1024));
  list.push_back(IntegerSettingOption(StringUtils::Format(kb.c_str(), 32), 32 * 1024));
  list.push_back(IntegerSettingOption(StringUtils::Format(kb.c_str(), 64), 64 * 1024));
  list.push_back(IntegerSettingOption(StringUtils::Format(kb.c_str(), 128), 128 * 1024));
  list.push_back(IntegerSettingOption(StringUtils::Format(kb.c_str(), 256), 256 * 1024));
  list.push_back(IntegerSettingOption(StringUtils::Format(kb.c_str(), 512), 512 * 1024));
  list.push_back(IntegerSettingOption(StringUtils::Format(mb.c_str(), 1), 1024 * 1024));
}
