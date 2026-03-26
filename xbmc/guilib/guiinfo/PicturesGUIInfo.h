/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"

#include <boost/move/unique_ptr.hpp>

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfo;

class CPicturesGUIInfo : public CGUIInfoProvider
{
public:
  CPicturesGUIInfo();
  virtual ~CPicturesGUIInfo();

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  virtual bool InitCurrentItem(CFileItem *item);
  virtual bool GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const;
  virtual bool GetInt(int& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const;
  virtual bool GetBool(bool& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const;

  void SetCurrentSlide(CFileItem *item);
  const CFileItem* GetCurrentSlide() const;

private:
  boost::movelib::unique_ptr<CFileItem> m_currentSlide;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
