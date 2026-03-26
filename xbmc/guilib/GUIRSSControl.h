/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIRSSControl.h
\brief
*/

#include "GUIControl.h"
#include "GUILabel.h"
#include "utils/IRssObserver.h"

#include <vector>

class CRssReader;

/*!
\ingroup controls
\brief
*/
class CGUIRSSControl : public CGUIControl, public IRssObserver
{
public:
  CGUIRSSControl(int parentID, int controlID, float posX, float posY, float width, float height,
                 const CLabelInfo& labelInfo, const KODI::GUILIB::GUIINFO::CGUIInfoColor &channelColor,
                 const KODI::GUILIB::GUIINFO::CGUIInfoColor &headlineColor, std::string& strRSSTags);
  CGUIRSSControl(const CGUIRSSControl &from);
  virtual ~CGUIRSSControl(void);
  virtual CGUIRSSControl* Clone() const { return new CGUIRSSControl(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual void OnFeedUpdate(const vecText &feed);
  virtual void OnFeedRelease();
  virtual bool CanFocus() const { return true; }
  virtual CRect CalcRenderRegion() const;

  virtual void OnFocus();
  virtual void OnUnFocus();

  void SetUrlSet(const int urlset);

protected:
  virtual bool UpdateColors(const CGUIListItem* item);

  CCriticalSection m_criticalSection;

  CRssReader* m_pReader;
  vecText m_feed;

  std::string m_strRSSTags;

  CLabelInfo m_label;
  KODI::GUILIB::GUIINFO::CGUIInfoColor m_channelColor;
  KODI::GUILIB::GUIINFO::CGUIInfoColor m_headlineColor;

  std::vector<std::string> m_vecUrls;
  std::vector<int> m_vecIntervals;
  bool m_rtl;
  CScrollInfo m_scrollInfo;
  bool m_dirty;
  bool m_stopped;
  int  m_urlset;
};

