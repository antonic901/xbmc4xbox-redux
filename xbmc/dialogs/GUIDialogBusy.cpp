/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogBusy.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "threads/IRunnable.h"
#include "threads/Thread.h"
#include "utils/log.h"

class CBusyWaiter : public CThread
{
  boost::shared_ptr<CEvent> m_done;
  IRunnable *m_runnable;
public:
  explicit CBusyWaiter(IRunnable *runnable) :
  CThread(runnable, "waiting"), m_done(new CEvent()),  m_runnable(runnable) { }

  virtual ~CBusyWaiter() { StopThread(); }

  bool Wait(unsigned int displaytime, bool allowCancel)
  {
    boost::shared_ptr<CEvent> e_done(m_done);

    Create();
    unsigned int start = XbmcThreads::SystemClockMillis();
    if (!CGUIDialogBusy::WaitOnEvent(*e_done, displaytime, allowCancel))
    {
      m_runnable->Cancel();

      unsigned int elapsed = XbmcThreads::SystemClockMillis() - start;

      unsigned int remaining =
          (elapsed >= displaytime) ? 0 : displaytime - elapsed;
      CGUIDialogBusy::WaitOnEvent(*e_done, remaining, false);
      return false;
    }
    return true;
  }

  // 'this' is actually deleted from the thread where it's on the stack
  virtual void Process()
  {
    boost::shared_ptr<CEvent> e_done(m_done);

    CThread::Process();
    (*e_done).Set();
  }

};

bool CGUIDialogBusy::Wait(IRunnable *runnable, unsigned int displaytime, bool allowCancel)
{
  if (!runnable)
    return false;
  CBusyWaiter waiter(runnable);
  if (!waiter.Wait(displaytime, allowCancel))
  {
    return false;
  }
  return true;
}

bool CGUIDialogBusy::WaitOnEvent(CEvent &event, unsigned int displaytime /* = 100 */, bool allowCancel /* = true */, bool isFromDvdPlayer /* = false */)
{
  bool cancelled = false;
  if (!event.WaitMSec(displaytime))
  {
    CGUIDialogBusy* dialog = static_cast<CGUIDialogBusy*>(
        CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_DIALOG_BUSY));
    if (dialog)
    {
      const uint32_t level = ++dialog->m_waiters;
      if (level == 1)
      {
        dialog->Open();
      }

      while (!event.WaitMSec(1))
      {
        if (level == dialog->m_waiters)
          dialog->ProcessRenderLoop(isFromDvdPlayer);
        if (allowCancel && dialog->m_cancelled)
        {
          cancelled = true;
          break;
        }
      }

      if (--dialog->m_waiters == 0)
      {
        dialog->Close(true); // Force close.
        dialog->ProcessRenderLoop(false); // Force repaint.
      }
    }
  }
  return !cancelled;
}

CGUIDialogBusy::CGUIDialogBusy(void)
  : CGUIDialog(WINDOW_DIALOG_BUSY, "DialogBusy.xml", MODAL)
{
  m_loadType = LOAD_ON_GUI_INIT;
  m_cancelled = false;
  m_bLastVisible = false;
  m_cancelled = false;
  m_waiters = 0;
}

CGUIDialogBusy::~CGUIDialogBusy(void) {}

void CGUIDialogBusy::Open_Internal(bool bProcessRenderLoop, const std::string& param /* = "" */)
{
  m_bLastVisible = true;
  m_cancelled = false;

  CGUIDialog::Open_Internal(false, param);
}

void CGUIDialogBusy::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool visible = CServiceBroker::GetGUI()->GetWindowManager().IsModalDialogTopmost(WINDOW_DIALOG_BUSY);
  if(!visible && m_bLastVisible)
    dirtyregions.push_back(CDirtyRegion(m_renderRegion));
  m_bLastVisible = visible;

  CGUIDialog::DoProcess(currentTime, dirtyregions);
}

void CGUIDialogBusy::Render()
{
  if(!m_bLastVisible)
    return;
  CGUIDialog::Render();
}

bool CGUIDialogBusy::OnBack(int actionID)
{
  m_cancelled = true;
  return true;
}
