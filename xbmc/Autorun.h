#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

//  CAutorun   -  Autorun for different Cd Media
//         like DVD Movies or XBOX Games
//
// by Bobbin007 in 2003
//
//
//

#include "system.h" // <xtl.h>

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

struct IntegerSettingOption;

namespace XFILE
{
  class IDirectory;
}

class CSetting;

enum AutoCDAction
{
  AUTOCD_NONE = 0,
  AUTOCD_PLAY,
  AUTOCD_RIP
};

namespace MEDIA_DETECT
{
class CAutorun
{
public:
  CAutorun();
  virtual ~CAutorun();
  static bool PlayDisc(bool restart = false);
  bool IsEnabled() const;
  void Enable();
  void Disable();
  void HandleAutorun();
  static void ExecuteAutorun(bool bypassSettings = false, bool ignoreplaying = false, bool restart = false);

  static void SettingOptionAudioCdActionsFiller(const boost::shared_ptr<const CSetting>& setting,
                                                std::vector<IntegerSettingOption>& list,
                                                int& current,
                                                void* data);
  static void SettingOptionAudioCdEncodersFiller(const boost::shared_ptr<const CSetting>& setting,
                                                 std::vector<IntegerSettingOption>& list,
                                                 int& current,
                                                 void* data);

protected:
  static void ExecuteXBE(const std::string &xbeFile);
  static void RunCdda();
  static void RunMedia(bool bypassSettings, bool restart);
  static bool RunDisc(XFILE::IDirectory* pDir, const std::string& strDrive, int& nAddedToPlaylist, bool bRoot, bool bypassSettings, bool restart);
  bool m_bEnable;
};
}
