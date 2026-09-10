/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "ServiceBroker.h"
#include "favourites/FavouritesService.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/ButtonTranslator.h"
#include "storage/MediaManager.h"
#include "storage/IoSupport.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

namespace CONTEXTMENU
{

  bool CEjectDisk::IsVisible(const CFileItem& item) const
  {
#ifdef HAS_OPTICAL_DRIVE
    return item.IsRemovable() && (item.IsDVD() || item.IsCDDA());
#else
    return false;
#endif
  }

  bool CEjectDisk::Execute(const boost::shared_ptr<CFileItem>& item) const
  {
#ifdef HAS_OPTICAL_DRIVE
    CIoSupport::ToggleTray();
#endif
    return true;
  }

namespace
{

int GetTargetWindowID(const CFileItem& item)
{
  int iTargetWindow = WINDOW_INVALID;

  const std::string targetWindow = item.GetProperty("targetwindow").asString();
  if (targetWindow.empty())
    iTargetWindow = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  else
    iTargetWindow = CButtonTranslator::TranslateWindow(targetWindow);

  return iTargetWindow;
}

} // unnamed namespace

std::string CAddRemoveFavourite::GetLabel(const CFileItem& item) const
{
  return g_localizeStrings.Get(CServiceBroker::GetFavouritesService().IsFavourited(item, GetTargetWindowID(item))
                               ? 14077   /* Remove from favourites */
                               : 14076); /* Add to favourites */
}

bool CAddRemoveFavourite::IsVisible(const CFileItem& item) const
{
  if (item.GetProperty("hide_add_remove_favourite").asBoolean())
    return false;

  return (!item.GetPath().empty() && !item.IsParentFolder() && !item.IsPath("add") &&
          !item.IsPath("newplaylist://") && !URIUtils::IsProtocol(item.GetPath(), "favourites") &&
          !URIUtils::IsProtocol(item.GetPath(), "newsmartplaylist") &&
          !URIUtils::IsProtocol(item.GetPath(), "newtag") &&
          !URIUtils::IsProtocol(item.GetPath(), "musicsearch"));
}

bool CAddRemoveFavourite::Execute(const boost::shared_ptr<CFileItem>& item) const
{
  return CServiceBroker::GetFavouritesService().AddOrRemove(*item.get(), GetTargetWindowID(*item));
}

} // namespace CONTEXTMENU
