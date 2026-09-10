/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenuManager.h"

#include "ContextMenuItem.h"
#include "ContextMenus.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/ContextMenuAddon.h"
#include "addons/ContextMenus.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "favourites/ContextMenus.h"
#include "messaging/ApplicationMessenger.h"
#include "music/ContextMenus.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/ContextMenus.h"

#include <boost/bind.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/algorithm/cxx11/copy_if.hpp>
#include <iterator>

using namespace ADDON;

const CContextMenuItem CContextMenuManager::MAIN = CContextMenuItem::CreateGroup("", "", "kodi.core.main", "");
const CContextMenuItem CContextMenuManager::MANAGE = CContextMenuItem::CreateGroup("", "", "kodi.core.manage", "");


CContextMenuManager::CContextMenuManager(CAddonMgr& addonMgr)
  : m_addonMgr(addonMgr) {}

CContextMenuManager::~CContextMenuManager()
{
  Deinit();
}

void CContextMenuManager::Deinit()
{
  m_addonMgr.Events().Unsubscribe(this);
  m_items.clear();
}

void CContextMenuManager::Init()
{
  m_addonMgr.Events().Subscribe(this, &CContextMenuManager::OnEvent);

  CSingleLock lock(m_criticalSection);
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoBrowse>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoChooseVersion>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoPlayVersionUsing>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoResume>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoPlay>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoPlayUsing>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoPlayAndQueue>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoPlayNext>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoQueue>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMusicBrowse>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMusicPlay>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMusicPlayUsing>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMusicPlayNext>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMusicQueue>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CAddonInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CEnableAddon>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CDisableAddon>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CAddonSettings>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CCheckForUpdates>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CEpisodeInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMovieInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMovieSetInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMusicVideoInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CTVShowInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMusicInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CSeasonInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CAlbumInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CArtistInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CSongInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoMarkWatched>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoMarkUnWatched>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CVideoRemoveResumePoint>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CEjectDisk>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CFavouritesTargetBrowse>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CFavouritesTargetResume>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CFavouritesTargetPlay>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CFavouritesTargetInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMoveUpFavourite>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMoveDownFavourite>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CChooseThumbnailForFavourite>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CRenameFavourite>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CRemoveFavourite>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CAddRemoveFavourite>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CFavouritesTargetContextMenu>());

  ReloadAddonItems();
}

void CContextMenuManager::ReloadAddonItems()
{
  VECADDONS addons;
  m_addonMgr.GetAddons(addons, AddonType::CONTEXTMENU_ITEM);

  std::vector<CContextMenuItem> addonItems;
  for (VECADDONS::const_iterator addon = addons.begin(); addon != addons.end(); ++addon)
  {
    std::vector<CContextMenuItem> items = boost::static_pointer_cast<CContextMenuAddon>(*addon)->GetItems();
    for (std::vector<CContextMenuItem>::iterator item = items.begin(); item != items.end(); ++item)
    {
      std::vector<CContextMenuItem>::iterator it = std::find(addonItems.begin(), addonItems.end(), *item);
      if (it == addonItems.end())
        addonItems.push_back(*item);
    }
  }

  CSingleLock lock(m_criticalSection);
  m_addonItems = addonItems;

  CLog::Log(LOGDEBUG, "ContextMenuManager: addon menus reloaded.");
}

void CContextMenuManager::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(AddonEvents::ReInstalled) ||
      typeid(event) == typeid(AddonEvents::UnInstalled))
  {
    ReloadAddonItems();
  }
  else if (typeid(event) == typeid(AddonEvents::Enabled))
  {
    AddonPtr addon;
    if (m_addonMgr.GetAddon(event.addonId, addon, AddonType::CONTEXTMENU_ITEM,
                            OnlyEnabled::CHOICE_YES))
    {
      CSingleLock lock(m_criticalSection);
      std::vector<CContextMenuItem> items = boost::static_pointer_cast<CContextMenuAddon>(addon)->GetItems();
      for (std::vector<CContextMenuItem>::iterator item = items.begin(); item != items.end(); ++item)
      {
        std::vector<CContextMenuItem>::iterator it = std::find(m_addonItems.begin(), m_addonItems.end(), *item);
        if (it == m_addonItems.end())
          m_addonItems.push_back(*item);
      }
      CLog::Log(LOGDEBUG, "ContextMenuManager: loaded %s.", event.addonId.c_str());
    }
  }
  else if (typeid(event) == typeid(AddonEvents::Disabled))
  {
    if (m_addonMgr.HasType(event.addonId, AddonType::CONTEXTMENU_ITEM))
    {
      ReloadAddonItems();
    }
  }
}

bool isItemParentAndVisible(const CContextMenuItem& other, const CContextMenuItem &menuItem, const CFileItem &fileItem)
{
  return menuItem.IsParentOf(other) && other.IsVisible(fileItem);
}

bool CContextMenuManager::IsVisible(
  const CContextMenuItem& menuItem, const CContextMenuItem& root, const CFileItem& fileItem) const
{
  if (menuItem.GetLabel(fileItem).empty() || !root.IsParentOf(menuItem))
    return false;

  if (menuItem.IsGroup())
  {
    CSingleLock lock(m_criticalSection);
    return boost::algorithm::any_of(m_addonItems, boost::bind(isItemParentAndVisible, _1, menuItem, fileItem));
  }

  return menuItem.IsVisible(fileItem);
}

bool isItemVisible(const boost::shared_ptr<IContextMenuItem> &menu, const CFileItem &fileItem)
{
  return menu->IsVisible(fileItem);
}

bool CContextMenuManager::HasItems(const CFileItem& fileItem, const CContextMenuItem& root) const
{
  //! @todo implement group support
  if (&root == &CContextMenuManager::MAIN)
  {
    CSingleLock lock(m_criticalSection);
    return boost::algorithm::any_of(m_items.begin(), m_items.end(), boost::bind(isItemVisible, _1, fileItem));
  }
  return false;
}

ContextMenuView CContextMenuManager::GetItems(const CFileItem& fileItem,
                                              const CContextMenuItem& root) const
{
  ContextMenuView result;
  //! @todo implement group support
  if (&root == &CContextMenuManager::MAIN)
  {
    CSingleLock lock(m_criticalSection);
    boost::algorithm::copy_if(m_items, std::back_inserter(result), boost::bind(isItemVisible, _1, fileItem));
  }
  return result;
}

bool sortByLabel(const ContextMenuView::value_type& lhs, const ContextMenuView::value_type& rhs, const CFileItem &fileItem)
{
  return lhs->GetLabel(fileItem) < rhs->GetLabel(fileItem);
}

bool CContextMenuManager::HasAddonItems(const CFileItem& fileItem,
                                        const CContextMenuItem& root) const
{
  CSingleLock lock(m_criticalSection);
  return boost::algorithm::any_of(m_addonItems.begin(), m_addonItems.end(), boost::bind(&CContextMenuManager::IsVisible, this, _1, root, fileItem));
}

ContextMenuView CContextMenuManager::GetAddonItems(const CFileItem& fileItem,
                                                   const CContextMenuItem& root) const
{
  ContextMenuView result;
  {
    CSingleLock lock(m_criticalSection);
    for (std::vector<CContextMenuItem>::const_iterator menu = m_addonItems.begin(); menu != m_addonItems.end(); ++menu)
      if (IsVisible(*menu, root, fileItem))
        result.push_back(boost::shared_ptr<const CContextMenuItem>( new CContextMenuItem(*menu)));
  }

  if (&root == &CContextMenuManager::MANAGE)
  {
    std::sort(result.begin(), result.end(), boost::bind(sortByLabel, _1, _2, fileItem));
  }
  return result;
}

bool CONTEXTMENU::HasAnyMenuItemsFor(const boost::shared_ptr<CFileItem>& fileItem,
                                     const CContextMenuItem& root)
{
  if (!fileItem)
    return false;

  if (fileItem->HasProperty("contextmenulabel(0)"))
    return true;

  const CContextMenuManager& contextMenuManager = CServiceBroker::GetContextMenuManager();
  return (contextMenuManager.HasItems(*fileItem, root) ||
          contextMenuManager.HasAddonItems(*fileItem, root));
}

bool CONTEXTMENU::ShowFor(const boost::shared_ptr<CFileItem>& fileItem, const CContextMenuItem& root)
{
  if (!fileItem)
    return false;

  const CContextMenuManager &contextMenuManager = CServiceBroker::GetContextMenuManager();

  ContextMenuView menuItems = contextMenuManager.GetItems(*fileItem, root);
  ContextMenuView vecAddonItems = contextMenuManager.GetAddonItems(*fileItem, root);
  for (ContextMenuView::const_iterator item = vecAddonItems.begin(); item != vecAddonItems.end(); ++item)
    menuItems.push_back(*item);

  CContextButtons buttons;
  // compute fileitem property-based contextmenu items
  // unless we're browsing a child menu item
  if (!root.HasParent())
  {
    int i = 0;
    while (fileItem->HasProperty(StringUtils::Format("contextmenulabel(%i)", i)))
    {
      buttons.push_back(std::make_pair(
          ~buttons.size(),
          fileItem->GetProperty(StringUtils::Format("contextmenulabel(%s)", i)).asString().c_str()));
      ++i;
    }
  }
  const int propertyMenuSize = buttons.size();

  if (menuItems.empty() && propertyMenuSize == 0)
    return true;

  buttons.reserve(menuItems.size());
  for (size_t i = 0; i < menuItems.size(); ++i)
    buttons.Add(i, menuItems[i]->GetLabel(*fileItem));

  int selected = CGUIDialogContextMenu::Show(buttons);
  if (selected < 0 || selected >= static_cast<int>(buttons.size()))
    return false;

  if (selected < propertyMenuSize)
  {
    CServiceBroker::GetAppMessenger()->SendMsg(
        TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
        fileItem->GetProperty(StringUtils::Format("contextmenuaction(%s)", selected)).asString().c_str());
    return true;
  }

  return menuItems[selected - propertyMenuSize]->IsGroup()
             ? ShowFor(fileItem, static_cast<const CContextMenuItem&>(
                                     *menuItems[selected - propertyMenuSize]))
             : menuItems[selected - propertyMenuSize]->Execute(fileItem);
}

bool CONTEXTMENU::LoopFrom(const IContextMenuItem& menu, const boost::shared_ptr<CFileItem>& fileItem)
{
  if (!fileItem)
    return false;
  if (menu.IsGroup())
    return ShowFor(fileItem, static_cast<const CContextMenuItem&>(menu));
  return menu.Execute(fileItem);
}
