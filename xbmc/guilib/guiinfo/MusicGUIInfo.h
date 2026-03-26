/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;

class CMusicGUIInfo : public CGUIInfoProvider
{
public:
  CMusicGUIInfo() : m_lastMusicBitrateTime(0) {}
  virtual ~CMusicGUIInfo() {}

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  virtual bool InitCurrentItem(CFileItem *item);
  virtual bool GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const;
  virtual bool GetFallbackLabel(std::string& value,
                        const CFileItem* item,
                        int contextWindow,
                        const CGUIInfo& info,
                        std::string* fallback);
  virtual bool GetInt(int& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const;
  virtual bool GetBool(bool& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const;

private:
  bool GetPartyModeLabel(std::string& value, const CGUIInfo &info) const;
  bool GetPlaylistInfo(std::string& value, const CGUIInfo &info) const;

  mutable unsigned int m_lastMusicBitrateTime;
  mutable unsigned int m_MusicBitrate;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
