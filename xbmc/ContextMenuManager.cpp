/*
 *      Copyright (C) 2013-2015 Team XBMC
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

#include "ContextMenuManager.h"
#include "ContextMenuItem.h"
#include "addons/Addon.h"
#include "addons/ContextMenuAddon.h"
#include "addons/ContextMenus.h"
#include "addons/IAddon.h"
#include "music/ContextMenus.h"
#include "video/ContextMenus.h"
#include "utils/log.h"
#include "ServiceBroker.h"

#include <iterator>
#include <boost/bind.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/algorithm/cxx11/copy_if.hpp>

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

CContextMenuManager& CContextMenuManager::GetInstance()
{
  return CServiceBroker::GetContextMenuManager();
}

void CContextMenuManager::Init()
{
  m_addonMgr.Events().Subscribe(this, &CContextMenuManager::OnEvent);

  CSingleLock lock(m_criticalSection);
  m_items.push_back(boost::make_shared<CONTEXTMENU::CResume>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CPlay>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CAddonInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CAddonSettings>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CCheckForUpdates>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CEpisodeInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMovieInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMusicVideoInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CTVShowInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CAlbumInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CArtistInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CSongInfo>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMarkWatched>());
  m_items.push_back(boost::make_shared<CONTEXTMENU::CMarkUnWatched>());
  ReloadAddonItems();
}

void CContextMenuManager::ReloadAddonItems()
{
  VECADDONS addons;
  m_addonMgr.GetAddons(addons, ADDON_CONTEXT_ITEM);

  std::vector<CContextMenuItem> addonItems;
  for (VECADDONS::const_iterator ait = addons.begin(); ait != addons.end(); ++ait)
  {
    const ADDON::AddonPtr &addon = *ait;
    std::vector<CContextMenuItem> items = boost::static_pointer_cast<CContextMenuAddon>(addon)->GetItems();
    for (std::vector<CContextMenuItem>::iterator iit = items.begin(); iit != items.end(); ++iit)
    {
      CContextMenuItem &item = *iit;
      std::vector<CContextMenuItem>::iterator it = std::find(addonItems.begin(), addonItems.end(), item);
      if (it == addonItems.end())
        addonItems.push_back(item);
    }
  }

  CSingleLock lock(m_criticalSection);
  m_addonItems = boost::move(addonItems);

  CLog::Log(LOGDEBUG, "ContextMenuManager: addon menus reloaded.");
}

bool removeItemIf(const CContextMenuItem& item, const std::vector<CContextMenuItem>& menuItems)
{
  if (item.IsGroup())
    return false; //keep in case other items use them
  return std::find(menuItems.begin(), menuItems.end(), item) != menuItems.end();
}

bool CContextMenuManager::Unload(const CContextMenuAddon& addon)
{
  CSingleLock lock(m_criticalSection);

  const std::vector<CContextMenuItem> menuItems = addon.GetItems();

  std::vector<CContextMenuItem>::iterator it = std::remove_if(m_addonItems.begin(), m_addonItems.end(),
    boost::bind(removeItemIf, _1, boost::cref(menuItems)));
  m_addonItems.erase(it, m_addonItems.end());
  CLog::Log(LOGDEBUG, "ContextMenuManager: %s unloaded.", addon.ID().c_str());
  return true;
}

void CContextMenuManager::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(AddonEvents::InstalledChanged))
  {
    ReloadAddonItems();
  }
  else if (const ADDON::AddonEvents::Enabled *enableEvent = dynamic_cast<const AddonEvents::Enabled*>(&event))
  {
    AddonPtr addon;
    if (m_addonMgr.GetAddon(enableEvent->id, addon, ADDON_CONTEXT_ITEM))
    {
      CSingleLock lock(m_criticalSection);
      std::vector<CContextMenuItem> items = boost::static_pointer_cast<CContextMenuAddon>(addon)->GetItems();
      for (std::vector<CContextMenuItem>::iterator iit = items.begin(); iit != items.end(); ++iit)
      {
        CContextMenuItem &item = *iit;
        std::vector<CContextMenuItem>::iterator it = std::find(m_addonItems.begin(), m_addonItems.end(), item);
        if (it == m_addonItems.end())
          m_addonItems.push_back(item);
      }
      CLog::Log(LOGDEBUG, "ContextMenuManager: loaded %s.", enableEvent->id.c_str());
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
    return boost::algorithm::any_of(m_addonItems, boost::bind(isItemParentAndVisible, _1, boost::cref(menuItem), boost::cref(fileItem)));
  }

  return menuItem.IsVisible(fileItem);
}

bool isItemVisible(const boost::shared_ptr<IContextMenuItem> &menu, const CFileItem &fileItem)
{
  return menu->IsVisible(fileItem);
}

ContextMenuView CContextMenuManager::GetItems(const CFileItem& fileItem, const CContextMenuItem& root /*= MAIN*/) const
{
  ContextMenuView result;
  //! @todo implement group support
  if (&root == &MAIN)
  {
    CSingleLock lock(m_criticalSection);
    boost::algorithm::copy_if(m_items, std::back_inserter(result), boost::bind(isItemVisible, _1, boost::cref(fileItem)));
  }
  return result;
}

bool sortByLabel(const ContextMenuView::value_type& lhs, const ContextMenuView::value_type& rhs, const CFileItem &fileItem)
{
  return lhs->GetLabel(fileItem) < rhs->GetLabel(fileItem);
}

ContextMenuView CContextMenuManager::GetAddonItems(const CFileItem& fileItem, const CContextMenuItem& root /*= MAIN*/) const
{
  ContextMenuView result;
  {
    CSingleLock lock(m_criticalSection);
    for (std::vector<CContextMenuItem>::const_iterator it = m_addonItems.begin(); it != m_addonItems.end(); ++it)
     if (IsVisible(*it, root, fileItem))
       result.push_back(boost::shared_ptr<const CContextMenuItem>(new CContextMenuItem(*it)));
  }

  if (&root == &MAIN || &root == &MANAGE)
  {
    std::sort(result.begin(), result.end(), boost::bind(sortByLabel, _1, _2, boost::cref(fileItem)));
  }
  return result;
}

bool CONTEXTMENU::ShowFor(const CFileItemPtr& fileItem, const CContextMenuItem& root)
{
  if (!fileItem)
    return false;

  ContextMenuView menuItems = CContextMenuManager::GetInstance().GetItems(*fileItem, root);
  for (ContextMenuView::const_iterator it = CContextMenuManager::GetInstance().GetAddonItems(*fileItem, root).begin(); it != CContextMenuManager::GetInstance().GetAddonItems(*fileItem, root).end(); ++it)
    menuItems.push_back(boost::move(*it));

  if (menuItems.empty())
    return true;

  CContextButtons buttons;
  for (size_t i = 0; i < menuItems.size(); ++i)
    buttons.Add(i, menuItems[i]->GetLabel(*fileItem));

  int selected = CGUIDialogContextMenu::Show(buttons);
  if (selected < 0 || selected >= static_cast<int>(menuItems.size()))
    return false;

  return menuItems[selected]->IsGroup() ?
         ShowFor(fileItem, static_cast<const CContextMenuItem&>(*menuItems[selected])) :
         menuItems[selected]->Execute(fileItem);
}

bool CONTEXTMENU::LoopFrom(const IContextMenuItem& menu, const CFileItemPtr& fileItem)
{
  if (!fileItem)
    return false;
  if (menu.IsGroup())
    return ShowFor(fileItem, static_cast<const CContextMenuItem&>(menu));
  return menu.Execute(fileItem);
}
