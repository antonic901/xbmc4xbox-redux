#ifndef SAVE_FILE_STATE_H__
#define SAVE_FILE_STATE_H__

#include "Job.h"
#include "FileItem.h"

class CSaveFileStateJob : public CJob
{
  CFileItem m_item;
  CFileItem m_item_discstack;
  CBookmark m_bookmark;
  bool      m_updatePlayCount;
public:
                CSaveFileStateJob(const CFileItem& item,
                                  const CFileItem& item_discstack,
                                  const CBookmark& bookmark,
                                  bool updatePlayCount)
                  : m_item(item),
                    m_item_discstack(item_discstack),
                    m_bookmark(bookmark),
                    m_updatePlayCount(updatePlayCount) {}
  virtual       ~CSaveFileStateJob() {}
  virtual bool  DoWork();
};

bool CSaveFileStateJob::DoWork()
{
  CStdString progressTrackingFile = m_item.GetPath();

  if (m_item.IsDVD()) 
    progressTrackingFile = m_item.GetVideoInfoTag()->m_strFileNameAndPath; // this variable contains removable:// suffixed by disc label

  if (progressTrackingFile != "")
  {
    if (m_item.IsVideo())
    {
      CLog::Log(LOGDEBUG, "%s - Saving file state for video item %s", __FUNCTION__, progressTrackingFile.c_str());

      CVideoDatabase videodatabase;
      if (videodatabase.Open())
      {
        bool updateListing = false;
        // No resume & watched status for livetv
        if (!m_item.IsLiveTV())
        {
          if (m_updatePlayCount)
          {
            CLog::Log(LOGDEBUG, "%s - Marking video item %s as watched", __FUNCTION__, progressTrackingFile.c_str());

            // consider this item as played
            videodatabase.IncrementPlayCount(m_item);
            updateListing = true;
          }

          if (!m_item.HasVideoInfoTag() || m_item.GetVideoInfoTag()->m_resumePoint.timeInSeconds != m_bookmark.timeInSeconds)
          {
            if (m_bookmark.timeInSeconds <= 0.0f)
            {
              videodatabase.ClearBookMarksOfFile(progressTrackingFile, CBookmark::RESUME);
            }
            else
            {
              videodatabase.AddBookMarkToFile(progressTrackingFile, m_bookmark, CBookmark::RESUME);
            }
            if (m_item.HasVideoInfoTag())
              m_item.GetVideoInfoTag()->m_resumePoint = m_bookmark;
            updateListing = true;
          }
        }

        if (CMediaSettings::Get().GetCurrentVideoSettings() != CMediaSettings::Get().GetDefaultVideoSettings())
        {
          videodatabase.SetVideoSettings(progressTrackingFile, CMediaSettings::Get().GetCurrentVideoSettings());
        }

        if (m_item.HasVideoInfoTag() && m_item.GetVideoInfoTag()->HasStreamDetails())
        {
          CFileItem dbItem(m_item);
          videodatabase.GetStreamDetails(dbItem); // Fetch stream details from the db (if any)

          // Check whether the item's db streamdetails need updating
          if (!dbItem.GetVideoInfoTag()->HasStreamDetails() || dbItem.GetVideoInfoTag()->m_streamDetails != m_item.GetVideoInfoTag()->m_streamDetails)
          {
            videodatabase.SetStreamDetailsForFile(m_item.GetVideoInfoTag()->m_streamDetails, progressTrackingFile);
            updateListing = true;
          }
        }

        // in order to properly update the the list, we need to update the stack item which is held in g_application.m_stackFileItemToUpdate
        if (m_item.HasProperty("stackFileItemToUpdate"))
        {
          m_item = m_item_discstack; // as of now, the item is replaced by the discstack item
          videodatabase.GetResumePoint(*m_item.GetVideoInfoTag());
        }
        videodatabase.Close();

        if (updateListing)
        {
          CUtil::DeleteVideoDatabaseDirectoryCache();
          CGUIMessage message(GUI_MSG_NOTIFY_ALL, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow(), 0, GUI_MSG_UPDATE, 0);
          CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);
        }
      }
    }

    if (m_item.IsAudio())
    {
      CLog::Log(LOGDEBUG, "%s - Saving file state for audio item %s", __FUNCTION__, m_item.GetPath().c_str());

      if (m_updatePlayCount)
      {
#if 0
        // Can't write to the musicdatabase while scanning for music info
        CGUIDialogMusicScan *dialog = (CGUIDialogMusicScan *)CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
        if (dialog && !dialog->IsDialogRunning())
#endif
        {
          // consider this item as played
          CLog::Log(LOGDEBUG, "%s - Marking audio item %s as listened", __FUNCTION__, m_item.GetPath().c_str());

          CMusicDatabase musicdatabase;
          if (musicdatabase.Open())
          {
            musicdatabase.IncrementPlayCount(m_item);
            musicdatabase.Close();
          }
        }
      }
    }
  }
  return true;
}

#endif // SAVE_FILE_STATE_H__
