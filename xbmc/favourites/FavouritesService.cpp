/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FavouritesService.h"

#include "FileItem.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "favourites/FavouritesURL.h"
#include "input/ButtonTranslator.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/ContentUtils.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

namespace
{
bool IsMediasourceOfFavItemUnlocked(const boost::shared_ptr<CFileItem>& item)
{
  if (!item)
  {
    CLog::Log(LOGERROR, "%s: No item passed (NULL).", __FUNCTION__);
    return true;
  }

  if (!item->IsFavourite())
  {
    CLog::Log(LOGERROR, "%s: Wrong item passed (not a favourite).", __FUNCTION__);
    return true;
  }

  const boost::shared_ptr<CSettingsComponent> settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
  {
    CLog::Log(LOGERROR, "%s: returned NULL.", __FUNCTION__);
    return true;
  }

  const boost::shared_ptr<CProfileManager> profileManager = settingsComponent->GetProfileManager();
  if (!profileManager)
  {
    CLog::Log(LOGERROR, "%s: returned NULL.", __FUNCTION__);
    return true;
  }

  const CFavouritesURL url(item->GetPath());
  if (!url.IsValid())
  {
    CLog::Log(LOGERROR, "%s: Invalid exec string (syntax error).", __FUNCTION__);
    return true;
  }

  const CFavouritesURL::Action action = url.GetAction();

  if (action != CFavouritesURL::Action::PLAY_MEDIA &&
      action != CFavouritesURL::Action::SHOW_PICTURE)
    return true;

  const CFileItem itemToCheck(url.GetTarget(), url.IsDir());

  if (action == CFavouritesURL::Action::PLAY_MEDIA)
  {
    if (itemToCheck.IsVideo())
    {
      if (!profileManager->GetCurrentProfile().videoLocked())
        return g_passwordManager.IsMediaFileUnlocked("video", itemToCheck.GetPath());

      return false;
    }
    else if (itemToCheck.IsAudio())
    {
      if (!profileManager->GetCurrentProfile().musicLocked())
        return g_passwordManager.IsMediaFileUnlocked("music", itemToCheck.GetPath());

      return false;
    }
  }
  else if (action == CFavouritesURL::Action::SHOW_PICTURE && itemToCheck.IsPicture())
  {
    if (!profileManager->GetCurrentProfile().picturesLocked())
      return g_passwordManager.IsMediaFileUnlocked("pictures", itemToCheck.GetPath());

    return false;
  }

  return true;
}

bool LoadFromFile(const std::string& strPath, CFileItemList& items)
{
  CXBMCTinyXML doc;
  if (!doc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "Unable to load %s (line %i)", strPath.c_str(), doc.Column());
    return false;
  }
  TiXmlElement *root = doc.RootElement();
  if (!root || strcmp(root->Value(), "favourites"))
  {
    CLog::Log(LOGERROR, "Favourites.xml doesn't contain the <favourites> root element");
    return false;
  }

  TiXmlElement *favourite = root->FirstChildElement("favourite");
  while (favourite)
  {
    // format:
    // <favourite name="Cool Video" thumb="foo.jpg">PlayMedia(c:\videos\cool_video.avi)</favourite>
    // <favourite name="My Album" thumb="bar.tbn">ActivateWindow(MyMusic,c:\music\my album)</favourite>
    // <favourite name="Apple Movie Trailers" thumb="path_to_thumb.png">RunScript(special://xbmc/scripts/apple movie trailers/default.py)</favourite>
    const char *name = favourite->Attribute("name");
    const char *thumb = favourite->Attribute("thumb");
    if (name && favourite->FirstChild())
    {
      const std::string favURL(
          CFavouritesURL(CExecString(favourite->FirstChild()->Value())).GetURL());
      if (!items.Contains(favURL))
      {
        const CFileItemPtr item(boost::make_shared<CFileItem>(name));
        item->SetPath(favURL);
        if (thumb)
          item->SetArt("thumb", thumb);
        items.Add(item);
      }
    }
    favourite = favourite->NextSiblingElement("favourite");
  }
  return true;
}
} // unnamed namespace

CFavouritesService::CFavouritesService(std::string userDataFolder) : m_favourites("favourites://")
{
  ReInit(userDataFolder);
}

void CFavouritesService::ReInit(std::string userDataFolder)
{
  CSingleLock lock(m_criticalSection);

  m_userDataFolder = userDataFolder;
  m_favourites.Clear();
  m_targets.clear();
  m_favourites.SetContent("favourites");

  std::string favourites = "special://xbmc/system/favourites.xml";
  if (CFileUtils::Exists(favourites))
    LoadFromFile(favourites, m_favourites);
  else
    CLog::Log(LOGDEBUG, "CFavourites::Load - no system favourites found, skipping");

  favourites = URIUtils::AddFileToFolder(m_userDataFolder, "favourites.xml");
  if (CFileUtils::Exists(favourites))
    LoadFromFile(favourites, m_favourites);
  else
    CLog::Log(LOGDEBUG, "CFavourites::Load - no userdata favourites found, skipping");
}

bool CFavouritesService::Persist()
{
  CXBMCTinyXML doc;
  TiXmlElement element("favourites");
  TiXmlNode *rootNode = doc.InsertEndChild(element);
  if (!rootNode)
    return false;

  for (int i = 0; i < m_favourites.Size(); i++)
  {
    const CFileItemPtr item = m_favourites[i];
    TiXmlElement favNode("favourite");
    favNode.SetAttribute("name", item->GetLabel().c_str());
    if (item->HasArt("thumb"))
      favNode.SetAttribute("thumb", item->GetArt("thumb").c_str());

    TiXmlText execute(CFavouritesURL(item->GetPath()).GetExecString().c_str());
    favNode.InsertEndChild(execute);
    rootNode->InsertEndChild(favNode);
  }

  std::string path = URIUtils::AddFileToFolder(m_userDataFolder, "favourites.xml");
  return doc.SaveFile(path);
}

bool CFavouritesService::Save(const CFileItemList& items)
{
  {
    CSingleLock lock(m_criticalSection);
    m_favourites.Clear();
    m_targets.clear();
    m_favourites.Copy(items);
    Persist();
  }
  OnUpdated();
  return true;
}

void CFavouritesService::OnUpdated()
{
  m_events.Publish(FavouritesUpdated());
}

bool CFavouritesService::AddOrRemove(const CFileItem& item, int contextWindow)
{
  {
    CSingleLock lock(m_criticalSection);

    const boost::shared_ptr<CFileItem> match = GetFavourite(item, contextWindow);
    if (match)
    {
      // remove the item
      const boost::unordered_map<std::string, boost::shared_ptr<CFileItem> >::iterator it = m_targets.find(match->GetPath());
      if (it != m_targets.end())
        m_targets.erase(it);

      m_favourites.Remove(match.get());
    }
    else
    {
      // create our new favourite item
      const boost::shared_ptr<CFileItem> favourite = boost::make_shared<CFileItem>(item.GetLabel());
      if (item.GetLabel().empty())
        favourite->SetLabel(CUtil::GetTitleFromPath(item.GetPath(), item.m_bIsFolder));
      favourite->SetArt("thumb", ContentUtils::GetPreferredArtImage(item));
      const std::string favUrl(CFavouritesURL(item, contextWindow).GetURL());
      favourite->SetPath(favUrl);
      m_favourites.Add(favourite);
    }
    Persist();
  }
  OnUpdated();
  return true;
}

boost::shared_ptr<CFileItem> CFavouritesService::GetFavourite(const CFileItem& item,
                                                            int contextWindow) const
{
  CSingleLock lock(m_criticalSection);

  const CFavouritesURL favURL(item, contextWindow);
  const bool isVideoDb(URIUtils::IsVideoDb(favURL.GetTarget()));
  const bool isMusicDb(URIUtils::IsMusicDb(favURL.GetTarget()));

  for (int i = 0; i < m_favourites.Size(); ++i)
  {
    const CFileItemPtr &favItem = m_favourites[i];
    const CFavouritesURL favItemURL(*favItem, contextWindow);

    // Compare the whole target URLs
    if (favItemURL.GetTarget() == item.GetPath())
      return favItem;

    // Compare the target URLs ignoring optional parameters
    if (favItemURL.GetAction() == favURL.GetAction() &&
        (favItemURL.GetAction() != CFavouritesURL::Action::ACTIVATE_WINDOW ||
         favItemURL.GetWindowID() == favURL.GetWindowID()))
    {
      if (favItemURL.GetTarget() == favURL.GetTarget())
        return favItem;

      // Check videodb and musicdb paths. Might be different strings pointing to same resource!
      // Example: "musicdb://recentlyaddedalbums/4711/" and "musicdb://recentlyplayedalbums/4711/",
      // both pointing to same album with db id 4711.
      if ((isVideoDb && URIUtils::IsVideoDb(favItemURL.GetTarget())) ||
          (isMusicDb && URIUtils::IsMusicDb(favItemURL.GetTarget())))
      {
        const boost::shared_ptr<CFileItem> targetItem = ResolveFavourite(*favItem);
        if (targetItem && targetItem->IsSamePath(&item))
          return favItem;
      }
    }
  }
  return boost::shared_ptr<CFileItem>();
}

bool CFavouritesService::IsFavourited(const CFileItem& item, int contextWindow) const
{
  return (GetFavourite(item, contextWindow) != NULL);
}

boost::shared_ptr<CFileItem> CFavouritesService::ResolveFavourite(const CFileItem& item) const
{
  if (item.IsFavourite())
  {
    CSingleLock lock(m_criticalSection);

    const boost::unordered_map<std::string, boost::shared_ptr<CFileItem> >::iterator it = m_targets.find(item.GetPath());
    if (it != m_targets.end())
      return (*it).second;

    const CFavouritesURL favURL(item.GetPath());
    if (favURL.IsValid())
    {
      boost::shared_ptr<CFileItem> targetItem = boost::make_shared<CFileItem>(favURL.GetTarget(), favURL.IsDir());
      targetItem->LoadDetails();
      if (favURL.GetWindowID() != -1)
      {
        const std::string window(CButtonTranslator::TranslateWindow(favURL.GetWindowID()));
        targetItem->SetProperty("targetwindow", window);
      }
      m_targets.insert(std::make_pair(item.GetPath(), targetItem));
      return targetItem;
    }
  }
  return boost::shared_ptr<CFileItem>();
}

int CFavouritesService::Size() const
{
  CSingleLock lock(m_criticalSection);
  return m_favourites.Size();
}

void CFavouritesService::GetAll(CFileItemList& items) const
{
  CSingleLock lock(m_criticalSection);
  items.Clear();
  if (g_passwordManager.IsMasterLockUnlocked(false)) // don't prompt
  {
    items.Copy(m_favourites, true); // copy items
  }
  else
  {
    for (int i = 0; i < m_favourites.Size(); ++i)
    {
      if (IsMediasourceOfFavItemUnlocked(m_favourites[i]))
        items.Add(m_favourites[i]);
    }
  }

  int index = 0;
  for (int i = 0; i < items.Size(); ++i)
  {
    const CFileItemPtr &item = items[i];
    const CFavouritesURL favURL(item->GetPath());
    item->SetProperty("favourite.action", favURL.GetActionLabel());
    item->SetProperty("favourite.provider", favURL.GetProviderLabel());
    item->SetProperty("favourite.index", index++);
  }
}

void CFavouritesService::RefreshFavourites()
{
  m_events.Publish(FavouritesUpdated());
}
