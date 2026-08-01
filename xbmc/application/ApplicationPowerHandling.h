/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/IApplicationComponent.h"

#include "utils/Stopwatch.h"

#include "system.h" // <xtl.h>
#include <boost/shared_ptr.hpp>
#include <string>

namespace ADDON
{
class IAddon;
typedef boost::shared_ptr<IAddon> AddonPtr;
} // namespace ADDON

class CApplication;
class CApplicationXbox;
class CSetting;

/*!
 * \brief Class handling application support for screensavers, dpms and shutdown timers.
 */

class CApplicationPowerHandling : public IApplicationComponent
{
  friend class CApplication;
  friend class CApplicationXbox;

public:
  CApplicationPowerHandling() : m_bInhibitScreenSaver(false), m_bResetScreenSaver(false), m_screensaverActive(false), m_iScreenSaveLock(0) {}

  bool IsInScreenSaver() const { return m_screensaverActive; }
  bool IsScreenSaverInhibited() const;
  void ResetScreenSaver();
  void SetScreenSaverLockFailed() { m_iScreenSaveLock = -1; }
  void SetScreenSaverUnlocked() { m_iScreenSaveLock = 1; }
  void StopScreenSaverTimer();
  std::string ScreensaverIdInUse() const { return m_screensaverIdInUse; }

  bool GetRenderGUI() const { return true; }

  int GlobalIdleTime();
  void ResetSystemIdleTimer();

  void ResetShutdownTimers();
  void StopShutdownTimer();

  void ResetNavigationTimer();

  // Wakes up from the screensaver and / or DPMS. Returns true if woken up.
  bool WakeUpScreenSaverAndDPMS();

  bool OnSettingAction(const CSetting& setting);

protected:
  void ActivateScreenSaver(bool forceType = false);
  // Checks whether the screensaver and / or DPMS should become active.
  void CheckScreenSaverAndDPMS();
  void InhibitScreenSaver(bool inhibit);
  void ResetScreenSaverTimer();
  bool WakeUpScreenSaver();

  /*! \brief Helper method to determine how to handle TMSG_SHUTDOWN
  */
  void HandleShutdownMessage();
  void CheckShutdown();

  float NavigationIdleTime();

  bool m_bInhibitScreenSaver;
  bool m_bResetScreenSaver;
  ADDON::AddonPtr
      m_pythonScreenSaver; // @warning: Fallback for Python interface, for binaries not needed!
  bool m_screensaverActive;
  // -1 = failed, 0 = locked, 1 = unlocked, 2 = check in progress
  int m_iScreenSaveLock;
  std::string m_screensaverIdInUse;

  CStopWatch m_navigationTimer;
  CStopWatch m_shutdownTimer;

  CStopWatch m_idleTimer;
  CStopWatch m_screenSaverTimer;
};
