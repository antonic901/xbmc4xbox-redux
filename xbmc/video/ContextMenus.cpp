/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "Autorun.h"
#include "ContextMenuManager.h"
#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ExecString.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"
#include "video/VideoUtils.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/guilib/VideoPlayActionProcessor.h"
#include "video/guilib/VideoSelectActionProcessor.h"
#include "video/guilib/VideoVersionHelper.h"

#include <utility>

namespace CONTEXTMENU
{

CVideoInfoBase::CVideoInfoBase(MediaType mediaType)
  : CStaticContextMenuAction(19033), m_mediaType(boost::move(mediaType))
{
}

bool CVideoInfoBase::IsVisible(const CFileItem& item) const
{
  if (!item.HasVideoInfoTag())
    return false;

  return item.GetVideoInfoTag()->m_type == m_mediaType;
}

bool CVideoInfoBase::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  CGUIDialogVideoInfo::ShowFor(*item);
  return true;
}

bool CVideoInfo::IsVisible(const CFileItem& item) const
{
  if (CVideoInfoBase::IsVisible(item))
    return true;

  if (item.m_bIsFolder)
    return false;

  const CVideoInfoTag *tag = item.GetVideoInfoTag();
  return tag && tag->m_type == MediaTypeNone && !tag->IsEmpty() && item.IsVideo();
}

bool CVideoRemoveResumePoint::IsVisible(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  // Folders don't have a resume point
  return !item.m_bIsFolder && VIDEO_UTILS::GetItemResumeInformation(item).isResumable;
}

bool CVideoRemoveResumePoint::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  CVideoLibraryQueue::GetInstance().ResetResumePoint(item);
  return true;
}

bool CVideoMarkWatched::IsVisible(const CFileItem& item) const
{
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  if (item.m_bIsFolder && item.IsPlugin()) // we cannot manage plugin folder's watched state
    return false;

  if (item.m_bIsFolder)
  {
    if (item.HasProperty("watchedepisodes") && item.HasProperty("totalepisodes"))
    {
      return item.GetProperty("watchedepisodes").asInteger() <
             item.GetProperty("totalepisodes").asInteger();
    }
    else if (item.HasProperty("watched") && item.HasProperty("total"))
    {
      return item.GetProperty("watched").asInteger() < item.GetProperty("total").asInteger();
    }
    else if (item.IsVideoDb())
      return true;
    else if (StringUtils::StartsWithNoCase(item.GetPath(), "library://video/"))
      return true;
    else if (item.GetProperty("IsVideoFolder").asBoolean())
      return true;
    else
      return false;
  }
  else if (!item.HasVideoInfoTag())
    return false;

  return item.GetVideoInfoTag()->GetPlayCount() == 0;
}

bool CVideoMarkWatched::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  CVideoLibraryQueue::GetInstance().MarkAsWatched(item, true);
  return true;
}

bool CVideoMarkUnWatched::IsVisible(const CFileItem& item) const
{
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  if (item.m_bIsFolder && item.IsPlugin()) // we cannot manage plugin folder's watched state
    return false;

  if (item.m_bIsFolder)
  {
    if (item.HasProperty("watchedepisodes"))
    {
      return item.GetProperty("watchedepisodes").asInteger() > 0;
    }
    else if (item.HasProperty("watched"))
    {
      return item.GetProperty("watched").asInteger() > 0;
    }
    else if (item.IsVideoDb())
      return true;
    else if (StringUtils::StartsWithNoCase(item.GetPath(), "library://video/"))
      return true;
    else if (item.GetProperty("IsVideoFolder").asBoolean())
      return true;
    else
      return false;
  }
  else if (!item.HasVideoInfoTag())
    return false;

  return item.GetVideoInfoTag()->GetPlayCount() > 0;
}

bool CVideoMarkUnWatched::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  CVideoLibraryQueue::GetInstance().MarkAsWatched(item, false);
  return true;
}

bool CVideoBrowse::IsVisible(const CFileItem& item) const
{
  return ((item.m_bIsFolder || item.IsFileFolder(EFILEFOLDER_MASK_ONBROWSE)) &&
          VIDEO_UTILS::IsItemPlayable(item));
}

bool CVideoBrowse::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  int target = WINDOW_VIDEO_NAV;

  CGUIWindowManager &windowMgr = CServiceBroker::GetGUI()->GetWindowManager();

  // For file directory browsing, we need item's dyn path, for everything else the path.
  const std::string path(item->IsFileFolder(EFILEFOLDER_MASK_ONBROWSE) ? item->GetDynPath()
                                                                       : item->GetPath());

  if (target == windowMgr.GetActiveWindow())
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, target, 0, GUI_MSG_UPDATE);
    msg.SetStringParam(path);
    windowMgr.SendMessage(msg);
  }
  else
  {
    std::vector<std::string> temp;
    temp.push_back(path);
    temp.push_back("return");
    windowMgr.ActivateWindow(target, temp);
  }
  return true;
}

namespace
{
bool ExecuteAction(const CExecString& execute)
{
  const std::string execStr(execute.GetExecString());
  if (!execStr.empty())
  {
    CGUIMessage message(GUI_MSG_EXECUTE, 0, 0);
    message.SetStringParam(execStr);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
    return true;
  }
  return false;
}

class CVideoSelectActionProcessor : public VIDEO::GUILIB::CVideoSelectActionProcessorBase
{
public:
  explicit CVideoSelectActionProcessor(const boost::shared_ptr<CFileItem>& item)
    : CVideoSelectActionProcessorBase(item)
  {
  }

protected:
  virtual bool OnPlayPartSelected(unsigned int part)
  {
    // part numbers are 1-based
    ExecuteAction(CExecString("PlayMedia", *m_item, StringUtils::Format("playoffset=%u", part - 1)));
    return true;
  }

  virtual bool OnResumeSelected()
  {
    ExecuteAction(CExecString("PlayMedia", *m_item, "resume"));
    return true;
  }

  virtual bool OnPlaySelected()
  {
    ExecuteAction(CExecString("PlayMedia", *m_item, "noresume"));
    return true;
  }

  virtual bool OnQueueSelected()
  {
    ExecuteAction(CExecString("QueueMedia", *m_item, ""));
    return true;
  }

  virtual bool OnInfoSelected()
  {
    CGUIDialogVideoInfo::ShowFor(*m_item);
    return true;
  }

  virtual bool OnChooseSelected()
  {
    CONTEXTMENU::ShowFor(m_item, CContextMenuManager::MAIN);
    return true;
  }
};
} // unnamed namespace

bool CVideoChooseVersion::IsVisible(const CFileItem& item) const
{
  return item.HasVideoVersions() &&
         !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
             CSettings::SETTING_VIDEOLIBRARY_SHOWVIDEOVERSIONSASFOLDER) &&
         !VIDEO::IsVideoAssetFile(item);
}

bool CVideoChooseVersion::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  // force selection dialog, regardless of any settings like 'Select default video version'
  item->SetProperty("needs_resolved_video_asset", true);
  item->SetProperty("video_asset_type", static_cast<int>(VideoAssetType::VERSION));
  CVideoSelectActionProcessor proc(item);
  const bool ret = proc.ProcessDefaultAction();
  item->ClearProperty("needs_resolved_video_asset");
  item->ClearProperty("video_asset_type");
  return ret;
}

std::string CVideoResume::GetLabel(const CFileItem& item) const
{
  return VIDEO_UTILS::GetResumeString(item.GetItemToPlay());
}

bool CVideoResume::IsVisible(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  return VIDEO_UTILS::GetItemResumeInformation(item).isResumable;
}

namespace
{
VECPLAYERCORES GetPlayers(const CPlayerCoreFactory& playerCoreFactory,
                                    const CFileItem& item)
{
  VECPLAYERCORES players;
  if (item.IsVideoDb())
  {
    //! @todo CPlayerCoreFactory and classes called from there do not handle dyn path correctly.
    CFileItem item2(item);
    item2.SetPath(item.GetDynPath());
    playerCoreFactory.GetPlayers(item2, players);
  }
  else
    playerCoreFactory.GetPlayers(item, players);

  return players;
}

class CVideoPlayActionProcessor : public VIDEO::GUILIB::CVideoPlayActionProcessorBase
{
public:
  CVideoPlayActionProcessor(const boost::shared_ptr<CFileItem>& item, bool choosePlayer)
    : CVideoPlayActionProcessorBase(item), m_choosePlayer(choosePlayer)
  {
  }

protected:
  virtual bool OnResumeSelected()
  {
    m_item->SetStartOffset(STARTOFFSET_RESUME);
    Play();
    return true;
  }

  virtual bool OnPlaySelected()
  {
    std::string player;
    if (m_choosePlayer)
    {
      const CPlayerCoreFactory& playerCoreFactory = CServiceBroker::GetPlayerCoreFactory();
      VECPLAYERCORES players = GetPlayers(playerCoreFactory, *m_item);
      g_application.m_eForcedNextPlayer = CServiceBroker::GetPlayerCoreFactory().SelectPlayerDialog(players);
      if (g_application.m_eForcedNextPlayer == EPC_NONE)
      {
        m_userCancelled = true;
        return true; // User cancelled player selection. We're done.
      }
    }

    Play("");
    return true;
  }

private:
  void Play(const std::string& player = "")
  {
    m_item->SetProperty("playlist_type_hint", PLAYLIST::TYPE_VIDEO);
    const ContentUtils::PlayMode::Type mode = m_item->GetProperty("CheckAutoPlayNextItem").asBoolean()
                                          ? ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM
                                          : ContentUtils::PlayMode::PLAY_ONLY_THIS;
    VIDEO_UTILS::PlayItem(m_item, player, mode);
  }

  const bool m_choosePlayer;
};

enum PlayMode
{
  PLAY,
  PLAY_USING,
  PLAY_VERSION_USING,
  RESUME,
};
void SetPathAndPlay(const boost::shared_ptr<CFileItem>& item, PlayMode mode)
{
  item->SetProperty("check_resume", false);

  if (item->IsLiveTV()) // pvr tv or pvr radio?
  {
    g_application.PlayMedia(*item, "", PLAYLIST::TYPE_VIDEO);
  }
  else
  {
    const CFileItemPtr itemCopy = boost::make_shared<CFileItem>(*item);
    if (itemCopy->IsVideoDb())
    {
      if (!itemCopy->m_bIsFolder)
      {
        itemCopy->SetProperty("original_listitem_url", item->GetPath());
        itemCopy->SetPath(item->GetVideoInfoTag()->m_strFileNameAndPath);
      }
      else if (itemCopy->HasVideoInfoTag() && itemCopy->GetVideoInfoTag()->IsDefaultVideoVersion())
      {
        //! @todo get rid of "videos with versions as folder" hack!
        itemCopy->m_bIsFolder = false;
      }
    }

    if (mode == PLAY_VERSION_USING)
    {
      // force video version selection dialog
      itemCopy->SetProperty("needs_resolved_video_asset", true);
    }
    else
    {
      // play the given/default video version, if multiple versions are available
      itemCopy->SetProperty("has_resolved_video_asset", true);
    }

    const bool choosePlayer = mode == PLAY_USING || mode == PLAY_VERSION_USING;
    CVideoPlayActionProcessor proc(itemCopy, choosePlayer);
    if (mode == RESUME && (itemCopy->GetStartOffset() == STARTOFFSET_RESUME ||
                                     VIDEO_UTILS::GetItemResumeInformation(*item).isResumable))
      proc.ProcessAction(VIDEO::GUILIB::ACTION_RESUME);
    else // all other modes are actually PLAY
      proc.ProcessAction(VIDEO::GUILIB::ACTION_PLAY_FROM_BEGINNING);
  }
}
} // unnamed namespace

bool CVideoResume::Execute(const boost::shared_ptr<CFileItem>& itemIn) const
{
  const CFileItemPtr item = boost::make_shared<CFileItem>(itemIn->GetItemToPlay());
#ifdef HAS_OPTICAL_DRIVE
  if (item->IsDVD() || item->IsCDDA())
    return MEDIA_DETECT::CAutorun::PlayDisc();
#endif

  item->SetStartOffset(STARTOFFSET_RESUME);
  SetPathAndPlay(item, RESUME);
  return true;
};

std::string CVideoPlay::GetLabel(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsLiveTV())
    return g_localizeStrings.Get(19000); // Switch to channel
  if (VIDEO_UTILS::GetItemResumeInformation(item).isResumable)
    return g_localizeStrings.Get(12021); // Play from beginning
  return g_localizeStrings.Get(208); // Play
}

bool CVideoPlay::IsVisible(const CFileItem& item) const
{
  return VIDEO_UTILS::IsItemPlayable(item);
}

bool CVideoPlay::Execute(const boost::shared_ptr<CFileItem>& itemIn) const
{
  const CFileItemPtr item = boost::make_shared<CFileItem>(itemIn->GetItemToPlay());
#ifdef HAS_OPTICAL_DRIVE
  if (item->IsDVD() || item->IsCDDA())
    MEDIA_DETECT::CAutorun::PlayDisc();
#endif
  SetPathAndPlay(item, PLAY);
  return true;
};

bool CVideoPlayUsing::IsVisible(const CFileItem& item) const
{
  if (item.HasVideoVersions() &&
      !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_VIDEOLIBRARY_SHOWVIDEOVERSIONSASFOLDER) &&
      !VIDEO::IsVideoAssetFile(item))
    return false;

  if (item.IsLiveTV())
    return false;

  const CPlayerCoreFactory& playerCoreFactory = CServiceBroker::GetPlayerCoreFactory();
  return (GetPlayers(playerCoreFactory, item).size() > 1) && VIDEO_UTILS::IsItemPlayable(item);
}

bool CVideoPlayUsing::Execute(const boost::shared_ptr<CFileItem>& itemIn) const
{
  const CFileItemPtr item = boost::make_shared<CFileItem>(itemIn->GetItemToPlay());
  SetPathAndPlay(item, PLAY_USING);
  return true;
}

bool CVideoPlayVersionUsing::IsVisible(const CFileItem& item) const
{
  return item.HasVideoVersions() &&
         !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
             CSettings::SETTING_VIDEOLIBRARY_SHOWVIDEOVERSIONSASFOLDER) &&
         !VIDEO::IsVideoAssetFile(item);
}

bool CVideoPlayVersionUsing::Execute(const boost::shared_ptr<CFileItem>& itemIn) const
{
  const CFileItemPtr item = boost::make_shared<CFileItem>(itemIn->GetItemToPlay());
  item->SetProperty("video_asset_type", static_cast<int>(VideoAssetType::VERSION));
  SetPathAndPlay(item, PLAY_VERSION_USING);
  return true;
}

namespace
{
void SelectNextItem(int windowID)
{
  CGUIWindowManager &windowMgr = CServiceBroker::GetGUI()->GetWindowManager();
  CGUIWindow* window = windowMgr.GetWindow(windowID);
  if (window)
  {
    const int viewContainerID = window->GetViewContainerID();
    if (viewContainerID > 0)
    {
      CGUIMessage msg1(GUI_MSG_ITEM_SELECTED, windowID, viewContainerID);
      windowMgr.SendMessage(msg1, windowID);

      CGUIMessage msg2(GUI_MSG_ITEM_SELECT, windowID, viewContainerID, msg1.GetParam1() + 1);
      windowMgr.SendMessage(msg2, windowID);
    }
  }
}

bool CanQueue(const CFileItem& item)
{
  if (!item.CanQueue())
    return false;

  const int windowId = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (windowId == WINDOW_VIDEO_PLAYLIST)
    return false; // Already queued

  return true;
}
} // unnamed namespace

bool CVideoQueue::IsVisible(const CFileItem& item) const
{
  if (!CanQueue(item))
    return false;

  return VIDEO_UTILS::IsItemPlayable(item);
}

bool CVideoQueue::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  VIDEO_UTILS::QueueItem(item, VIDEO_UTILS::POSITION_END);

  // Set selection to next item in active window's view.
  const int windowID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  SelectNextItem(windowID);

  return true;
};

bool CVideoPlayNext::IsVisible(const CFileItem& item) const
{
  if (!CanQueue(item))
    return false;

  return VIDEO_UTILS::IsItemPlayable(item);
}

bool CVideoPlayNext::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  VIDEO_UTILS::QueueItem(item, VIDEO_UTILS::POSITION_BEGIN);
  return true;
};

std::string CVideoPlayAndQueue::GetLabel(const CFileItem& item) const
{
  if (VIDEO_UTILS::IsAutoPlayNextItem(item))
    return g_localizeStrings.Get(13434); // Play only this
  else
    return g_localizeStrings.Get(13412); // Play from here
}

bool CVideoPlayAndQueue::IsVisible(const CFileItem& item) const
{
  if (!CanQueue(item))
    return false;

  return false; //! @todo implement
}

bool CVideoPlayAndQueue::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  return true; //! @todo implement
};

}
