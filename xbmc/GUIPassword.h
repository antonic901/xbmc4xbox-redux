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

#include <map>
#include <vector>

#include "LockType.h"
#include "settings/lib/ISettingCallback.h"
#include "utils/StdString.h"
#include "settings/lib/Setting.h"

class CFileItem;
class CMediaSource;

typedef std::vector<CMediaSource> VECSOURCES;

namespace LOCK_LEVEL {
  /**
   Specifies, what Settings levels are locked for the user
   **/
  enum SETTINGS_LOCK
  {
    NONE,     //settings are unlocked => user can access all settings levels
    ALL,      //all settings are locked => user always has to enter password, when entering the settings screen
    STANDARD, //settings level standard and up are locked => user can still access the beginner levels
    ADVANCED,
    EXPERT
  };
}

class CGUIPassword : public ISettingCallback
{
public:
  CGUIPassword(void);
  virtual ~CGUIPassword(void);
  bool IsItemUnlocked(CFileItem* pItem, const CStdString &strType);
  bool IsItemUnlocked(CMediaSource* pItem, const CStdString &strType);
  bool CheckLock(LockType btnType, const CStdString& strPassword, int iHeading);
  bool CheckLock(LockType btnType, const CStdString& strPassword, int iHeading, bool& bCanceled);
  bool IsProfileLockUnlocked(int iProfile=-1);
  bool IsProfileLockUnlocked(int iProfile, bool& bCanceled, bool prompt = true);
  bool IsMasterLockUnlocked(bool bPromptUser);
  bool IsMasterLockUnlocked(bool bPromptUser, bool& bCanceled);

  void UpdateMasterLockRetryCount(bool bResetCount);
  bool GetSMBShareUserPassword();
  void SetSMBShare(const CStdString &strShare);
  CStdString GetSMBShare();
  bool CheckStartUpLock();
  /*! \brief Checks if the current profile is allowed to access the given settings level
   \param level - The level to check
   \param enforce - If false, CheckSettingLevelLock is allowed to lower the current settings level
                    to a level we're allowed to access
   \returns true if we're allowed to access the settings
   */
  bool CheckSettingLevelLock(const SettingLevel::Type& level, bool enforce = false);
  bool CheckMenuLock(int iWindowID);
  bool SetMasterLockMode(bool bDetails=true);
  CStdString GetSMBAuthFilename(const CStdString& strAuth);
  bool LockSource(const CStdString& strType, const CStdString& strName, bool bState);
  void LockSources(bool lock);
  void RemoveSourceLocks();
  bool IsDatabasePathUnlocked(std::string& strPath, VECSOURCES& VECSOURCES);

  virtual void OnSettingAction(const CSetting *setting);

  bool bMasterUser;
  int iMasterLockRetriesLeft;
protected:
  CStdString m_SMBShare;
private:
  int VerifyPassword(LockType btnType, const CStdString& strPassword, const CStdString& strHeading);
};

extern CGUIPassword g_passwordManager;

