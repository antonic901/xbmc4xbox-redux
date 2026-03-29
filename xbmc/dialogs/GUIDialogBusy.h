/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <stdint.h>

class IRunnable;
class CEvent;

class CGUIDialogBusy : private CGUIDialog
{
  friend class CGUIWindowManager;

public:
  /*! \brief Wait for a runnable to execute off-thread.
   Creates a thread to run the given runnable, and while waiting
   it displays the busy dialog.
   \param runnable the IRunnable to run.
   \param displaytime the time in ms to wait prior to showing the busy dialog (defaults to 100ms)
   \param allowCancel whether the user can cancel the wait, defaults to true.
   \return true if the runnable completes, false if the user cancels early.
   */
  static bool Wait(IRunnable *runnable, unsigned int displaytime, bool allowCancel);

  /*! \brief Wait on an event while displaying the busy dialog.
   Throws up the busy dialog after the given time.
   \param event the CEvent to wait on.
   \param displaytime the time in ms to wait prior to showing the busy dialog (defaults to 100ms)
   \param allowCancel whether the user can cancel the wait, defaults to true.
   \return true if the event completed, false if cancelled.
   */
  static bool WaitOnEvent(CEvent &event, unsigned int displaytime = 100, bool allowCancel = true, bool isFromDvdPlayer = false);

private:
  CGUIDialogBusy();
  virtual ~CGUIDialogBusy();

  virtual void Open_Internal(bool bProcessRenderLoop, const std::string& param = "");
  virtual bool OnBack(int actionID);
  virtual void DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions);
  virtual void Render();

  bool m_bLastVisible;
  bool m_cancelled;
  uint32_t m_waiters;
};
