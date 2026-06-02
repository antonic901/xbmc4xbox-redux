/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoVersionHelper.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoManagerTypes.h"
#include "video/VideoThumbLoader.h"

using namespace VIDEO::GUILIB;

namespace
{
class CVideoChooser
{
public:
  explicit CVideoChooser(const boost::shared_ptr<const CFileItem>& item) : m_item(item), m_enableTypeSwitch(false), m_initialAssetType(VideoAssetType::UNKNOWN), m_switchType(false) {}
  virtual ~CVideoChooser() {}

  void EnableTypeSwitch(bool enable) { m_enableTypeSwitch = enable; }
  void SetInitialAssetType(VideoAssetType::Type type) { m_initialAssetType = type; }

  boost::shared_ptr<const CFileItem> ChooseVideo();

private:
  CVideoChooser();
  boost::shared_ptr<const CFileItem> ChooseVideoVersion();
  boost::shared_ptr<const CFileItem> ChooseVideoExtra();
  boost::shared_ptr<const CFileItem> ChooseVideo(CGUIDialogSelect& dialog,
                                               int headingId,
                                               int buttonId,
                                               CFileItemList& itemsToDisplay,
                                               const CFileItemList& itemsToSwitchTo);

  const boost::shared_ptr<const CFileItem> m_item;
  bool m_enableTypeSwitch;
  VideoAssetType::Type m_initialAssetType;
  bool m_switchType;
  CFileItemList m_videoVersions;
  CFileItemList m_videoExtras;
};

boost::shared_ptr<const CFileItem> CVideoChooser::ChooseVideo()
{
  m_switchType = false;
  m_videoVersions.Clear();
  m_videoExtras.Clear();

  boost::shared_ptr<const CFileItem> result;
  if (m_enableTypeSwitch && !m_item->HasVideoVersions() && !m_item->HasVideoExtras())
    return result;

  if (!m_enableTypeSwitch && m_initialAssetType == VideoAssetType::VERSION &&
      !m_item->HasVideoVersions())
    return result;

  if (!m_enableTypeSwitch && m_initialAssetType == VideoAssetType::EXTRA &&
      !m_item->HasVideoExtras())
    return result;

  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::Log(LOGERROR, "Unable to open video database!");
    return result;
  }

  if (m_initialAssetType == VideoAssetType::VERSION || m_enableTypeSwitch)
  {
    db.GetAssetsForVideo(m_item->GetVideoContentType(), m_item->GetVideoInfoTag()->m_iDbId,
                         VideoAssetType::VERSION, m_videoVersions);

    // find default version item in list and select it
    for (int i = 0; i < m_videoVersions.Size(); ++i)
    {
      m_videoVersions[i]->Select(m_videoVersions[i]->GetVideoInfoTag()->IsDefaultVideoVersion());
    }
  }

  if (m_initialAssetType == VideoAssetType::EXTRA || m_enableTypeSwitch)
    db.GetAssetsForVideo(m_item->GetVideoContentType(), m_item->GetVideoInfoTag()->m_iDbId,
                         VideoAssetType::EXTRA, m_videoExtras);

  VideoAssetType::Type itemType(m_initialAssetType);
  while (true)
  {
    if (itemType == VideoAssetType::VERSION)
    {
      result = ChooseVideoVersion();
      itemType = VideoAssetType::EXTRA;
    }
    else
    {
      result = ChooseVideoExtra();
      itemType = VideoAssetType::VERSION;
    }

    if (!m_switchType)
      break;

    // switch type button pressed. Re-open, this time with the "other" type to select.
  }
  return result;
}

boost::shared_ptr<const CFileItem> CVideoChooser::ChooseVideoVersion()
{
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT_VIDEO_VERSION);
  if (!dialog)
  {
    CLog::Log(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT_VIDEO_VERSION dialog instance!");
    return boost::shared_ptr<const CFileItem>();
  }

  return ChooseVideo(*dialog, 40208 /* Choose version */, 40211 /* Extras */, m_videoVersions,
                     m_videoExtras);
}

boost::shared_ptr<const CFileItem> CVideoChooser::ChooseVideoExtra()
{
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT_VIDEO_EXTRA);
  if (!dialog)
  {
    CLog::Log(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT_VIDEO_EXTRA dialog instance!");
    return boost::shared_ptr<const CFileItem>();
  }

  return ChooseVideo(*dialog, 40214 /* Choose extra */, 40210 /* Versions */, m_videoExtras,
                     m_videoVersions);
}

boost::shared_ptr<const CFileItem> CVideoChooser::ChooseVideo(CGUIDialogSelect& dialog,
                                                            int headingId,
                                                            int buttonId,
                                                            CFileItemList& itemsToDisplay,
                                                            const CFileItemList& itemsToSwitchTo)
{
  CVideoThumbLoader thumbLoader;
  thumbLoader.Load(itemsToDisplay);
  for (int i = 0; i < itemsToDisplay.Size(); ++i)
    itemsToDisplay[i]->SetLabel2(itemsToDisplay[i]->GetVideoInfoTag()->m_strFileNameAndPath);

  dialog.Reset();

  const std::string heading =
      StringUtils::Format(g_localizeStrings.Get(headingId).c_str(), m_item->GetVideoInfoTag()->GetTitle().c_str());
  dialog.SetHeading(heading);

  dialog.EnableButton(m_enableTypeSwitch && !itemsToSwitchTo.IsEmpty(), buttonId);
  dialog.SetUseDetails(true);
  dialog.SetMultiSelection(false);
  dialog.SetItems(itemsToDisplay);

  dialog.Open();

  if (thumbLoader.IsLoading())
    thumbLoader.StopThread();

  m_switchType = dialog.IsButtonPressed();
  if (dialog.IsConfirmed())
    return dialog.GetSelectedFileItem();

  return boost::shared_ptr<const CFileItem>();
}
} // unnamed namespace

boost::shared_ptr<CFileItem> CVideoVersionHelper::ChooseVideoFromAssets(
    const boost::shared_ptr<CFileItem>& item)
{
  boost::shared_ptr<const CFileItem> video;

  VideoAssetType::Type assetType = static_cast<VideoAssetType::Type>(static_cast<int>(
      item->GetProperty("video_asset_type").asInteger(static_cast<int>(VideoAssetType::UNKNOWN))));
  bool allAssetTypes = false;
  bool hasMultipleChoices = false;

  switch (assetType)
  {
    case VideoAssetType::UNKNOWN:
      // asset type not provided means all types are allowed and the user can switch between types
      allAssetTypes = true;
      if (item->HasVideoVersions() || item->HasVideoExtras())
        hasMultipleChoices = true;
      break;

    case VideoAssetType::VERSION:
      if (item->HasVideoVersions())
        hasMultipleChoices = true;
      break;

    case VideoAssetType::EXTRA:
      if (item->HasVideoExtras())
        hasMultipleChoices = true;
      break;

    default:
      CLog::Log(LOGERROR, "unknown asset type (%i)", static_cast<int>(assetType));
      return boost::shared_ptr<CFileItem>();
  }

  if (hasMultipleChoices)
  {
    if (!item->GetProperty("needs_resolved_video_asset").asBoolean(false))
    {
      // auto select the default video version
      const boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
      if (settings->GetBool(CSettings::SETTING_MYVIDEOS_SELECTDEFAULTVERSION))
      {
        if (item->GetVideoInfoTag()->IsDefaultVideoVersion())
        {
          video = boost::make_shared<const CFileItem>(*item);
        }
        else
        {
          CVideoDatabase db;
          if (!db.Open())
          {
            CLog::Log(LOGERROR, "Unable to open video database!");
          }
          else
          {
            CFileItem defaultVersion;
            if (!db.GetDefaultVersionForVideo(item->GetVideoContentType(),
                                              item->GetVideoInfoTag()->m_iDbId, defaultVersion))
              CLog::Log(LOGERROR, "Unable to get default version from video database!");
            else
              video = boost::make_shared<const CFileItem>(defaultVersion);
          }
        }
      }
    }

    if (!video && (item->GetProperty("needs_resolved_video_asset").asBoolean(false) ||
                   !item->GetProperty("has_resolved_video_asset").asBoolean(false)))
    {
      CVideoChooser chooser(item);

      if (allAssetTypes)
      {
        chooser.EnableTypeSwitch(true);
        chooser.SetInitialAssetType(VideoAssetType::VERSION);
      }
      else
      {
        chooser.EnableTypeSwitch(false);
        chooser.SetInitialAssetType(assetType);
      }

      const boost::shared_ptr<const CFileItem> result = chooser.ChooseVideo();
      if (result)
        video = result;
      else
        return boost::shared_ptr<CFileItem>();
    }
  }

  if (video)
    return boost::make_shared<CFileItem>(*video);

  return item;
}

bool VIDEO::IsVideoAssetFile(const CFileItem& item)
{
  if (item.m_bIsFolder || !item.IsVideoDb())
    return false;

  // @todo maybe in the future look for prefix videodb://movies/videoversions in path instead
  // @todo better encoding of video assets as path, they won't always be tied with movies.
  const CURL itemUrl(item.GetPath());
  if (itemUrl.HasOption("videoversionid"))
    return true;

  return false;
}
