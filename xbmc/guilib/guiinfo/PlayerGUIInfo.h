/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"
#include "utils/EventStream.h"
#include "utils/TimeFormat.h"

#include <ctime>
#include <boost/bind.hpp>
#include <boost/move/unique_ptr.hpp>
#include <utility>
#include <vector>

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;

struct PlayerShowInfoChangedEvent
{
  explicit PlayerShowInfoChangedEvent(bool showInfo) : m_showInfo(showInfo) {}
  virtual ~PlayerShowInfoChangedEvent() {}

  bool m_showInfo;
};

class CPlayerGUIInfo : public CGUIInfoProvider
{
public:
  CPlayerGUIInfo();
  virtual ~CPlayerGUIInfo();

  CEventStream<PlayerShowInfoChangedEvent>& Events() { return m_events; }

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  virtual bool InitCurrentItem(CFileItem *item);
  virtual bool GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const;
  virtual bool GetInt(int& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const;
  virtual bool GetBool(bool& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const;

  bool GetDisplayAfterSeek();
  void SetDisplayAfterSeek(unsigned int timeOut = 2500, int seekOffset = 0);
  void SetShowTime(bool showtime) { m_playerShowTime = showtime; }
  void SetShowInfo(bool showinfo);
  bool GetShowInfo() const { return m_playerShowInfo; }
  bool ToggleShowInfo();

private:
  int GetTotalPlayTime() const;
  int GetPlayTime() const;
  int GetPlayTimeRemaining() const;
  float GetSeekPercent() const;

  std::string GetCurrentPlayTime(TIME_FORMAT format) const;
  std::string GetCurrentPlayTimeRemaining(TIME_FORMAT format) const;
  std::string GetDuration(TIME_FORMAT format) const;
  std::string GetCurrentSeekTime(TIME_FORMAT format) const;
  std::string GetSeekTime(TIME_FORMAT format) const;

  boost::movelib::unique_ptr<CFileItem> m_currentItem;
  bool m_playerShowTime;
  bool m_playerShowInfo;
  CEventSource<PlayerShowInfoChangedEvent> m_events;

  unsigned int m_AfterSeekTimeout;
  int m_seekOffset;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
