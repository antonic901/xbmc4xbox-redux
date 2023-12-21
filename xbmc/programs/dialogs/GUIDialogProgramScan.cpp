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

#include "utils/log.h"
#include "programs/dialogs/GUIDialogProgramScan.h"
#include "GUIProgressControl.h"
#include "Util.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "settings/GUISettings.h"
#include "Application.h"
#include "utils/SingleLock.h"

#define CONTROL_LABELSTATUS       401
#define CONTROL_LABELDIRECTORY    402
#define CONTROL_PROGRESS          403
#define CONTROL_CURRENT_PROGRESS  404
#define CONTROL_LABELTITLE        405

using namespace PROGRAM;

CGUIDialogProgramScan::CGUIDialogProgramScan(void)
: CGUIDialog(WINDOW_DIALOG_PROGRAM_SCAN, "DialogProgramScan.xml")
{
  m_programInfoScanner.SetObserver(this);
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogProgramScan::~CGUIDialogProgramScan(void)
{
}

bool CGUIDialogProgramScan::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);

      m_strCurrentDir.Empty();
      m_strTitle.Empty();

      m_fPercentDone=-1.0f;
      m_fCurrentPercentDone=-1.0f;

      UpdateState();
      return true;
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogProgramScan::FrameMove()
{
  if (m_bRunning)
    UpdateState();

  CGUIDialog::FrameMove();
}

void CGUIDialogProgramScan::OnDirectoryChanged(const CStdString& strDirectory)
{
  CSingleLock lock (m_critical);

  m_strCurrentDir = strDirectory;
}

void CGUIDialogProgramScan::OnStateChanged(SCAN_STATE state)
{
  CSingleLock lock (m_critical);

  m_ScanState = state;
}

void CGUIDialogProgramScan::OnSetProgress(int currentItem, int itemCount)
{
  CSingleLock lock (m_critical);

  m_fPercentDone=(float)((currentItem*100)/itemCount);
  if (m_fPercentDone>100.0F) m_fPercentDone=100.0F;
}

void CGUIDialogProgramScan::OnSetCurrentProgress(int currentItem, int itemCount)
{
  CSingleLock lock (m_critical);

  m_fCurrentPercentDone=(float)((currentItem*100)/itemCount);
  if (m_fCurrentPercentDone>100.0F) m_fCurrentPercentDone=100.0F;
}

void CGUIDialogProgramScan::OnSetTitle(const CStdString& strTitle)
{
  CSingleLock lock (m_critical);

  m_strTitle = strTitle;
}

void CGUIDialogProgramScan::StartScanning(const CStdString& strDirectory, bool scanAll)
{
  m_ScanState = PREPARING;

  if (!g_guiSettings.GetBool("programlibrary.backgroundupdate"))
  {
    Show();
  }

  m_programInfoScanner.Start(strDirectory,scanAll);
}

void CGUIDialogProgramScan::StopScanning()
{
  if (m_programInfoScanner.IsScanning())
    m_programInfoScanner.Stop();
}

bool CGUIDialogProgramScan::IsScanning()
{
  return m_programInfoScanner.IsScanning();
}

void CGUIDialogProgramScan::OnDirectoryScanned(const CStdString& strDirectory)
{
  CGUIMessage msg(GUI_MSG_DIRECTORY_SCANNED, 0, 0, 0);
  msg.SetStringParam(strDirectory);
  g_windowManager.SendThreadMessage(msg);
}

void CGUIDialogProgramScan::OnFinished()
{
  // clear cache
  CUtil::DeleteProgramDatabaseDirectoryCache();

  // send message
  CGUIMessage msg(GUI_MSG_SCAN_FINISHED, 0, 0, 0);
  g_windowManager.SendThreadMessage(msg);

  // be sure to restore the settings
  CLog::Log(LOGINFO,"Program scan was stopped or finished ... restoring FindRemoteThumbs");

  if (!g_guiSettings.GetBool("programlibrary.backgroundupdate"))
  {
    g_application.getApplicationMessenger().Close(this,false,false);
  }
}

void CGUIDialogProgramScan::UpdateState()
{
  CSingleLock lock (m_critical);

  SET_CONTROL_LABEL(CONTROL_LABELSTATUS, GetStateString());

  if (m_ScanState == FETCHING_GAME_INFO || m_ScanState == CLEANING_UP_DATABASE)
  {
    CURL url(m_strCurrentDir);
    CStdString strStrippedPath = url.GetWithoutUserDetails();
    CURL::Decode(strStrippedPath);

    SET_CONTROL_LABEL(CONTROL_LABELDIRECTORY, strStrippedPath);
    SET_CONTROL_LABEL(CONTROL_LABELTITLE, m_strTitle);

    if (m_fCurrentPercentDone>-1.0f)
    {
      SET_CONTROL_VISIBLE(CONTROL_CURRENT_PROGRESS);
      CGUIProgressControl* pProgressCtrl=(CGUIProgressControl*)GetControl(CONTROL_CURRENT_PROGRESS);
      if (pProgressCtrl) pProgressCtrl->SetPercentage(m_fCurrentPercentDone);
    }
    else
      SET_CONTROL_HIDDEN(CONTROL_CURRENT_PROGRESS);

    if (m_fPercentDone>-1.0f)
    {
      SET_CONTROL_VISIBLE(CONTROL_PROGRESS);
      CGUIProgressControl* pProgressCtrl=(CGUIProgressControl*)GetControl(CONTROL_PROGRESS);
      if (pProgressCtrl) pProgressCtrl->SetPercentage(m_fPercentDone);
    }
    else
      SET_CONTROL_HIDDEN(CONTROL_PROGRESS);
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABELDIRECTORY, "");
    SET_CONTROL_LABEL(CONTROL_LABELTITLE, "");
    SET_CONTROL_HIDDEN(CONTROL_PROGRESS);
    SET_CONTROL_HIDDEN(CONTROL_CURRENT_PROGRESS);
  }
}

int CGUIDialogProgramScan::GetStateString()
{
  if (m_ScanState == PREPARING)
    return 314;
  else if (m_ScanState == REMOVING_OLD)
    return 701;
  else if (m_ScanState == CLEANING_UP_DATABASE)
    return 700;
  else if (m_ScanState == FETCHING_GAME_INFO)
    return 35001;
  else if (m_ScanState == COMPRESSING_DATABASE)
    return 331;
  else if (m_ScanState == WRITING_CHANGES)
    return 328;

  return -1;
}
