/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationPlayerCallback.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationStackHelper.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/python/XBPython.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/JobManager.h"
#include "utils/SaveFileStateJob.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <memory>

CApplicationPlayerCallback::CApplicationPlayerCallback()
{
}

void CApplicationPlayerCallback::OnPlayBackEnded()
{
  CLog::Log(LOGDEBUG, "CApplicationPlayerCallback::OnPlayBackEnded");

  CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackStarted(const CFileItem& file)
{
  CLog::Log(LOGDEBUG, "CApplication::OnPlayBackStarted");
  boost::shared_ptr<CFileItem> itemCurrentFile;

  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationStackHelper> stackHelper = components.GetComponent<CApplicationStackHelper>();

  if (stackHelper->IsPlayingISOStack() || stackHelper->IsPlayingRegularStack())
    itemCurrentFile = boost::make_shared<CFileItem>(*stackHelper->GetRegisteredStack(file));
  else
    itemCurrentFile = boost::make_shared<CFileItem>(file);

  /* When playing video pause any low priority jobs, they will be unpaused  when playback stops.
   * This should speed up player startup for files on internet filesystems (eg. webdav) and
   * increase performance on low powered systems (Atom/ARM).
   */
  if (file.IsVideo())
  {
    CServiceBroker::GetJobManager()->PauseJobs();
  }

  stackHelper->OnPlayBackStarted(file);

  CGUIMessage msg(GUI_MSG_PLAYBACK_STARTED, 0, 0, 0, 0, itemCurrentFile);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayerCloseFile(const CFileItem& file,
                                                   const CBookmark& bookmarkParam)
{
  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationStackHelper> stackHelper = components.GetComponent<CApplicationStackHelper>();

  CSingleLock lock(stackHelper->m_critSection);

  CFileItem fileItem(file);
  CBookmark bookmark = bookmarkParam;
  CBookmark resumeBookmark;
  bool playCountUpdate = false;
  float percent = 0.0f;

  // Make sure we don't reset existing bookmark etc. on eg. player start failure
  if (bookmark.timeInSeconds == 0.0)
    return;

  if (stackHelper->GetRegisteredStack(fileItem) != NULL &&
      stackHelper->GetRegisteredStackTotalTimeMs(fileItem) > 0)
  {
    // regular stack case: we have to save the bookmark on the stack
    fileItem = *stackHelper->GetRegisteredStack(file);
    // the bookmark coming from the player is only relative to the current part, thus needs to be corrected with these attributes (start time will be 0 for non-stackparts)
    bookmark.timeInSeconds += stackHelper->GetRegisteredStackPartStartTimeMs(file) / 1000.0;
    if (stackHelper->GetRegisteredStackTotalTimeMs(file) > 0)
      bookmark.totalTimeInSeconds = stackHelper->GetRegisteredStackTotalTimeMs(file) / 1000.0;
    bookmark.partNumber = stackHelper->GetRegisteredStackPartNumber(file);
  }

  percent = bookmark.timeInSeconds / bookmark.totalTimeInSeconds * 100;

  const boost::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  if ((fileItem.IsAudio() && advancedSettings->m_audioPlayCountMinimumPercent > 0 &&
       percent >= advancedSettings->m_audioPlayCountMinimumPercent) ||
      (fileItem.IsVideo() && advancedSettings->m_videoPlayCountMinimumPercent > 0 &&
       percent >= advancedSettings->m_videoPlayCountMinimumPercent))
  {
    playCountUpdate = true;
  }

  if (advancedSettings->m_videoIgnorePercentAtEnd > 0 &&
      bookmark.totalTimeInSeconds - bookmark.timeInSeconds <
          0.01 * static_cast<double>(advancedSettings->m_videoIgnorePercentAtEnd) *
              bookmark.totalTimeInSeconds)
  {
    resumeBookmark.timeInSeconds = -1.0;
  }
  else if (bookmark.timeInSeconds > advancedSettings->m_videoIgnoreSecondsAtStart)
  {
    resumeBookmark = bookmark;
    if (stackHelper->GetRegisteredStack(file) != NULL)
    {
      // also update video info tag with total time
      fileItem.GetVideoInfoTag()->m_streamDetails.SetVideoDuration(
          0, resumeBookmark.totalTimeInSeconds);
    }
  }
  else
  {
    resumeBookmark.timeInSeconds = 0.0;
  }

  if (CServiceBroker::GetSettingsComponent()
          ->GetProfileManager()
          ->GetCurrentProfile()
          .canWriteDatabases())
  {
    CSaveFileState::DoWork(fileItem, resumeBookmark, playCountUpdate);
  }
}

void CApplicationPlayerCallback::OnPlayBackPaused()
{
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnPlayBackPaused();
#endif

  CGUIMessage msg(GUI_MSG_PLAYBACK_PAUSED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackResumed()
{
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnPlayBackResumed();
#endif

  CGUIMessage msg(GUI_MSG_PLAYBACK_RESUMED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackStopped()
{
  CLog::Log(LOGDEBUG, "CApplication::OnPlayBackStopped");

  CGUIMessage msg(GUI_MSG_PLAYBACK_STOPPED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackError()
{
  //@todo Playlists can be continued by calling OnPlaybackEnded instead
  // open error dialog
  CGUIMessage msg(GUI_MSG_PLAYBACK_ERROR, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  OnPlayBackStopped();
}

void CApplicationPlayerCallback::OnQueueNextItem()
{
  CLog::Log(LOGDEBUG, "CApplication::OnQueueNextItem");

  // informs python script currently running that we are requesting the next track
  // (does nothing if python is not loaded)
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnQueueNextItem(); // currently unimplemented
#endif

  CGUIMessage msg(GUI_MSG_QUEUE_NEXT_ITEM, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackSeek(int64_t iTime, int64_t seekOffset)
{
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnPlayBackSeek(static_cast<int>(iTime),
                                               static_cast<int>(seekOffset));
#endif

  CGUIMessage msg(GUI_MSG_PLAYBACK_SEEKED, 0, 0, iTime, seekOffset);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackSeekChapter(int iChapter)
{
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnPlayBackSeekChapter(iChapter);
#endif
}

void CApplicationPlayerCallback::OnPlayBackSpeedChanged(int iSpeed)
{
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnPlayBackSpeedChanged(iSpeed);
#endif

  CGUIMessage msg(GUI_MSG_PLAYBACK_SPEED_CHANGED, 0, 0, iSpeed);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::RequestVideoSettings(const CFileItem& fileItem)
{
  CVideoDatabase dbs;
  if (dbs.Open())
  {
    CLog::Log(LOGDEBUG, "Loading settings for %s", CURL::GetRedacted(fileItem.GetPath()).c_str());

    // Load stored settings if they exist, otherwise use default
    CVideoSettings vs;
    if (!dbs.GetVideoSettings(fileItem, vs))
      vs = CMediaSettings::GetInstance().GetDefaultVideoSettings();

    CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
    appPlayer->SetVideoSettings(vs);

    dbs.Close();
  }
}

void CApplicationPlayerCallback::StoreVideoSettings(const CFileItem& fileItem,
                                                    const CVideoSettings& vs)
{
  CVideoDatabase dbs;
  if (dbs.Open())
  {
    if (vs != CMediaSettings::GetInstance().GetDefaultVideoSettings())
    {
      dbs.SetVideoSettings(fileItem, vs);
    }
    else
    {
      dbs.EraseVideoSettings(fileItem);
    }
    dbs.Close();
  }
}
