/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryProvider.h"

#include "ContextMenuManager.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/GUIDialogAddonInfo.h"
#include "filesystem/Directory.h"
#include "filesystem/FavouritesDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "music/MusicThumbLoader.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "pictures/PictureThumbLoader.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/JobManager.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"
#include "video/VideoThumbLoader.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoBase.h"

using namespace XFILE;
using namespace KODI::MESSAGING;

class CDirectoryJob : public CJob
{
public:
  CDirectoryJob(const std::string& url,
                const std::string& target,
                SortDescription sort,
                int limit,
                CDirectoryProvider::BrowseMode browse,
                int parentID)
    : m_url(url),
      m_target(target),
      m_sort(sort),
      m_limit(limit),
      m_browse(browse),
      m_parentID(parentID)
  { }
  virtual ~CDirectoryJob() {}

  virtual const char* GetType() const { return "directory"; }
  virtual bool operator==(const CJob *job) const
  {
    if (strcmp(job->GetType(),GetType()) == 0)
    {
      const CDirectoryJob* dirJob = dynamic_cast<const CDirectoryJob*>(job);
      if (dirJob && dirJob->m_url == m_url)
        return true;
    }
    return false;
  }

  virtual bool DoWork()
  {
    CFileItemList items;
    if (CDirectory::GetDirectory(m_url, items, "", DIR_FLAG_DEFAULTS))
    {
      // sort the items if necessary
      if (m_sort.sortBy != SortByNone)
        items.Sort(m_sort);

      // limit must not exceed the number of items
      int limit = (m_limit == 0) ? items.Size() : std::min(static_cast<int>(m_limit), items.Size());
      if (limit < items.Size())
        m_items.reserve(limit + 1);
      else
        m_items.reserve(limit);
      // convert to CGUIStaticItem's and set visibility and targets
      for (int i = 0; i < limit; i++)
      {
        CGUIStaticItemPtr item(new CGUIStaticItem(*items[i]));
        if (item->HasProperty("node.visible"))
          item->SetVisibleCondition(item->GetProperty("node.visible").asString(), m_parentID);

        getThumbLoader(item)->LoadItem(item.get());

        m_items.push_back(item);
      }

      if (items.HasProperty("node.target"))
        m_target = items.GetProperty("node.target").asString();

      if ((m_browse == CDirectoryProvider::BrowseMode::ALWAYS && !items.IsEmpty()) ||
          (m_browse == CDirectoryProvider::BrowseMode::AUTO && limit < items.Size()))
      {
        // Add a special item to the end of the list, which can be used to open the
        // full listing containg all items in the given target window.
        if (!m_target.empty())
        {
          CFileItem item(m_url, true);
          item.SetLabel(g_localizeStrings.Get(22082)); // More...
          item.SetArt("icon", "DefaultFolder.png");
          item.SetProperty("node.target", m_target);
          item.SetProperty("node.type", "target_folder"); // make item identifyable, e.g. by skins

          m_items.push_back(boost::make_shared<CGUIStaticItem>(item));
        }
        else
          CLog::Log(LOGWARNING, "Cannot add 'More...' item to list. No target window given.");
      }
    }
    return true;
  }

  boost::shared_ptr<CThumbLoader> getThumbLoader(const CGUIStaticItemPtr& item)
  {
    if (item->IsVideo())
    {
      initThumbLoader<CVideoThumbLoader>(InfoTagType::VIDEO);
      return m_thumbloaders[InfoTagType::VIDEO];
    }
    if (item->IsAudio())
    {
      initThumbLoader<CMusicThumbLoader>(InfoTagType::AUDIO);
      return m_thumbloaders[InfoTagType::AUDIO];
    }
    if (item->IsPicture())
    {
      initThumbLoader<CPictureThumbLoader>(InfoTagType::PICTURE);
      return m_thumbloaders[InfoTagType::PICTURE];
    }
    initThumbLoader<CProgramThumbLoader>(InfoTagType::PROGRAM);
    return m_thumbloaders[InfoTagType::PROGRAM];
  }

  template<class CThumbLoaderClass>
  void initThumbLoader(InfoTagType::TagType type)
  {
    if (!m_thumbloaders.count(type))
    {
      boost::shared_ptr<CThumbLoader> thumbLoader = boost::make_shared<CThumbLoaderClass>();
      thumbLoader->OnLoaderStart();
      m_thumbloaders.insert(make_pair(type, thumbLoader));
    }
  }

  const std::vector<CGUIStaticItemPtr> &GetItems() const { return m_items; }
  const std::string &GetTarget() const { return m_target; }
  std::vector<InfoTagType::TagType> GetItemTypes(std::vector<InfoTagType::TagType> &itemTypes) const
  {
    itemTypes.clear();
    for (std::map<InfoTagType::TagType, boost::shared_ptr<CThumbLoader> >::const_iterator i = m_thumbloaders.begin(); i != m_thumbloaders.end(); ++i)
      itemTypes.push_back(i->first);
    return itemTypes;
  }
private:
  std::string m_url;
  std::string m_target;
  SortDescription m_sort;
  unsigned int m_limit;
  CDirectoryProvider::BrowseMode m_browse;
  int m_parentID;
  std::vector<CGUIStaticItemPtr> m_items;
  std::map<InfoTagType::TagType, boost::shared_ptr<CThumbLoader> > m_thumbloaders;
};

CDirectoryProvider::CDirectoryProvider(const TiXmlElement* element, int parentID)
  : IListProvider(parentID),
    m_updateState(OK),
    m_jobID(0),
    m_currentLimit(0),
    m_currentBrowse(AUTO),
    m_isSubscribed(false)
{
  assert(element);
  if (!element->NoChildren())
  {
    const char *target = element->Attribute("target");
    if (target)
      m_target.SetLabel(target, "", parentID);

    const char *sortMethod = element->Attribute("sortby");
    if (sortMethod)
      m_sortMethod.SetLabel(sortMethod, "", parentID);

    const char *sortOrder = element->Attribute("sortorder");
    if (sortOrder)
      m_sortOrder.SetLabel(sortOrder, "", parentID);

    const char *limit = element->Attribute("limit");
    if (limit)
      m_limit.SetLabel(limit, "", parentID);

    const char* browse = element->Attribute("browse");
    if (browse)
      m_browse.SetLabel(browse, "", parentID);

    m_url.SetLabel(element->FirstChild()->ValueStr(), "", parentID);
  }
}

CDirectoryProvider::CDirectoryProvider(const CDirectoryProvider& other)
  : IListProvider(other.m_parentID),
    m_updateState(INVALIDATED),
    m_jobID(0),
    m_url(other.m_url),
    m_target(other.m_target),
    m_sortMethod(other.m_sortMethod),
    m_sortOrder(other.m_sortOrder),
    m_limit(other.m_limit),
    m_browse(other.m_browse),
    m_currentUrl(other.m_currentUrl),
    m_currentTarget(other.m_currentTarget),
    m_currentSort(other.m_currentSort),
    m_currentLimit(other.m_currentLimit),
    m_currentBrowse(other.m_currentBrowse),
    m_isSubscribed(false)
{
}

CDirectoryProvider::~CDirectoryProvider()
{
  Reset();
}

boost::movelib::unique_ptr<IListProvider> CDirectoryProvider::Clone()
{
  return boost::movelib::unique_ptr<IListProvider>(new CDirectoryProvider(*this));
}

bool CDirectoryProvider::Update(bool forceRefresh)
{
  // we never need to force refresh here
  bool changed = false;
  bool fireJob = false;

  // update the URL & limit and fire off a new job if needed
  fireJob |= UpdateURL();
  fireJob |= UpdateSort();
  fireJob |= UpdateLimit();
  fireJob |= UpdateBrowse();
  fireJob &= !m_currentUrl.empty();

  CSingleLock lock(m_section);
  if (m_updateState == INVALIDATED)
    fireJob = true;
  else if (m_updateState == DONE)
    changed = true;

  m_updateState = OK;

  if (fireJob)
  {
    CLog::Log(LOGDEBUG, "CDirectoryProvider[%s]: refreshing..", m_currentUrl.c_str());
    if (m_jobID)
      CServiceBroker::GetJobManager()->CancelJob(m_jobID);
    m_jobID = CServiceBroker::GetJobManager()->AddJob(
        new CDirectoryJob(m_currentUrl, m_target.GetLabel(m_parentID, false), m_currentSort,
                          m_currentLimit, m_currentBrowse, m_parentID),
        this);
  }

  if (!changed)
  {
    for (std::vector<CGUIStaticItemPtr>::iterator i = m_items.begin(); i != m_items.end(); ++i)
      changed |= (*i)->UpdateVisibility(m_parentID);
  }
  return changed; //! @todo Also returned changed if properties are changed (if so, need to update scroll to letter).
}

void CDirectoryProvider::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                                  const char *sender,
                                  const char *message,
                                  const CVariant& data)
{
  // we are only interested in library, player and GUI changes
  if ((flag & (ANNOUNCEMENT::VideoLibrary | ANNOUNCEMENT::AudioLibrary | ANNOUNCEMENT::Player | ANNOUNCEMENT::GUI)) == 0)
    return;

  {
    CSingleLock lock(m_section);
    // we don't need to refresh anything if there are no fitting
    // items in this list provider for the announcement flag
    if (((flag & ANNOUNCEMENT::VideoLibrary) &&
         (std::find(m_itemTypes.begin(), m_itemTypes.end(), InfoTagType::VIDEO) == m_itemTypes.end())) ||
        ((flag & ANNOUNCEMENT::AudioLibrary) &&
         (std::find(m_itemTypes.begin(), m_itemTypes.end(), InfoTagType::AUDIO) == m_itemTypes.end())))
      return;

    if (flag & ANNOUNCEMENT::Player)
    {
      if (strcmp(message, "OnPlay") == 0 || strcmp(message, "OnResume") == 0 || strcmp(message, "OnStop") == 0)
      {
        if (m_currentSort.sortBy == SortByNone || // not nice, but many directories that need to be refreshed on start/stop have no special sort order (e.g. in progress movies)
            m_currentSort.sortBy == SortByLastPlayed ||
            m_currentSort.sortBy == SortByPlaycount ||
            m_currentSort.sortBy == SortByLastUsed)
          m_updateState = INVALIDATED;
      }
    }
    else
    {
      // if we're in a database transaction, don't bother doing anything just yet
      if (data.isMember("transaction") && data["transaction"].asBoolean())
        return;

      // if there was a database update, we set the update state
      // to PENDING to fire off a new job in the next update
      if (strcmp(message, "OnScanFinished") == 0 || strcmp(message, "OnCleanFinished") == 0 || strcmp(message, "OnUpdate") == 0 ||
          strcmp(message, "OnRemove") == 0 || strcmp(message, "OnRefresh") == 0)
        m_updateState = INVALIDATED;
    }
  }
}

void CDirectoryProvider::Fetch(std::vector<boost::shared_ptr<CGUIListItem> >& items)
{
  CSingleLock lock(m_section);
  items.clear();
  for (std::vector<CGUIStaticItemPtr>::const_iterator i = m_items.begin(); i != m_items.end(); ++i)
  {
    if ((*i)->IsVisible())
      items.push_back(*i);
  }
}

void CDirectoryProvider::OnAddonEvent(const ADDON::AddonEvent& event)
{
  CSingleLock lock(m_section);
  if (URIUtils::IsProtocol(m_currentUrl, "addons"))
  {
    if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
        typeid(event) == typeid(ADDON::AddonEvents::Disabled) ||
        typeid(event) == typeid(ADDON::AddonEvents::InstalledChanged) ||
        typeid(event) == typeid(ADDON::AddonEvents::MetadataChanged))
      m_updateState = INVALIDATED;
  }
}

void CDirectoryProvider::OnAddonRepositoryEvent(const ADDON::CRepositoryUpdater::RepositoryUpdated& event)
{
  CSingleLock lock(m_section);
  if (URIUtils::IsProtocol(m_currentUrl, "addons"))
  {
    m_updateState = INVALIDATED;
  }
}

void CDirectoryProvider::Reset()
{
  {
    CSingleLock lock(m_section);
    if (m_jobID)
      CServiceBroker::GetJobManager()->CancelJob(m_jobID);
    m_jobID = 0;
    m_items.clear();
    m_currentTarget.clear();
    m_currentUrl.clear();
    m_itemTypes.clear();
    m_currentSort.sortBy = SortByNone;
    m_currentSort.sortOrder = SortOrderAscending;
    m_currentLimit = 0;
    m_currentBrowse = BrowseMode::AUTO;
    m_updateState = OK;
  }

  CSingleLock subscriptionLock(m_subscriptionSection);
  if (m_isSubscribed)
  {
    m_isSubscribed = false;
    CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
    CServiceBroker::GetRepositoryUpdater().Events().Unsubscribe(this);
    CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
  }
}

void CDirectoryProvider::FreeResources(bool immediately)
{
  CSingleLock lock(m_section);
  for (std::vector<CGUIStaticItemPtr>::iterator item = m_items.begin(); item != m_items.end(); ++item)
    (*item)->FreeMemory(immediately);
}

void CDirectoryProvider::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CSingleLock lock(m_section);
  if (success)
  {
    m_items = static_cast<CDirectoryJob*>(job)->GetItems();
    m_currentTarget = static_cast<CDirectoryJob*>(job)->GetTarget();
    static_cast<CDirectoryJob*>(job)->GetItemTypes(m_itemTypes);
    if (m_updateState == OK)
      m_updateState = DONE;
  }
  m_jobID = 0;
}

std::string CDirectoryProvider::GetTarget(const CFileItem& item) const
{
  std::string target = item.GetProperty("node.target").asString();

  CSingleLock lock(m_section);
  if (target.empty())
    target = m_currentTarget;
  if (target.empty())
    target = m_target.GetLabel(m_parentID, false);

  return target;
}

namespace
{
bool ExecuteAction(const std::string& execute)
{
  if (!execute.empty())
  {
    CGUIMessage message(GUI_MSG_EXECUTE, 0, 0);
    message.SetStringParam(execute);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
    return true;
  }
  return false;
}
} // namespace

bool CDirectoryProvider::OnClick(const boost::shared_ptr<CGUIListItem>& item)
{
  CFileItem fileItem(*boost::static_pointer_cast<CFileItem>(item));

  if (fileItem.HasVideoInfoTag()
      && CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("myvideos.selectaction") == SELECT_ACTION_INFO
      && OnInfo(item))
    return true;

  if (fileItem.HasProperty("node.target_url"))
    fileItem.SetPath(fileItem.GetProperty("node.target_url").asString());

  return ExecuteAction(CFavouritesDirectory::GetExecutePath(fileItem, GetTarget(fileItem)));
}

bool CDirectoryProvider::OnInfo(const boost::shared_ptr<CFileItem>& fileItem)
{
  if (fileItem->HasAddonInfo())
  {
    return CGUIDialogAddonInfo::ShowForItem(fileItem);
  }
  else if (fileItem->HasVideoInfoTag())
  {
    MediaType mediaType = fileItem->GetVideoInfoTag()->m_type;
    if (mediaType == MediaTypeMovie || mediaType == MediaTypeTvShow ||
        mediaType == MediaTypeSeason || mediaType == MediaTypeEpisode ||
        mediaType == MediaTypeVideo || mediaType == MediaTypeVideoCollection ||
        mediaType == MediaTypeMusicVideo)
    {
      CGUIDialogVideoInfo::ShowFor(*fileItem);
      return true;
    }
  }
  else if (fileItem->HasMusicInfoTag())
  {
    CGUIDialogMusicInfo::ShowFor(fileItem.get());
    return true;
  }
  return false;
}

bool CDirectoryProvider::OnInfo(const boost::shared_ptr<CGUIListItem>& item)
{
  boost::shared_ptr<CFileItem> fileItem = boost::static_pointer_cast<CFileItem>(item);
  return OnInfo(fileItem);
}

bool CDirectoryProvider::OnContextMenu(const boost::shared_ptr<CFileItem>& fileItem)
{
  const std::string target = GetTarget(*fileItem);
  if (!target.empty())
    fileItem->SetProperty("targetwindow", target);

  return CONTEXTMENU::ShowFor(fileItem, CContextMenuManager::MAIN);
}

bool CDirectoryProvider::OnContextMenu(const boost::shared_ptr<CGUIListItem>& item)
{
  boost::shared_ptr<CFileItem> fileItem = boost::static_pointer_cast<CFileItem>(item);
  return OnContextMenu(fileItem);
}

bool CDirectoryProvider::IsUpdating() const
{
  CSingleLock lock(m_section);
  return m_jobID || m_updateState == DONE || m_updateState == INVALIDATED;
}

bool CDirectoryProvider::UpdateURL()
{
  {
    CSingleLock lock(m_section);
    std::string value(m_url.GetLabel(m_parentID, false));
    if (value == m_currentUrl)
      return false;

    m_currentUrl = value;
  }

  CSingleLock subscriptionLock(m_subscriptionSection);
  if (!m_isSubscribed)
  {
    m_isSubscribed = true;
    CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
    CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CDirectoryProvider::OnAddonEvent);
    CServiceBroker::GetRepositoryUpdater().Events().Subscribe(this, &CDirectoryProvider::OnAddonRepositoryEvent);
  }
  return true;
}

bool CDirectoryProvider::UpdateLimit()
{
  CSingleLock lock(m_section);
  unsigned int value = m_limit.GetIntValue(m_parentID);
  if (value == m_currentLimit)
    return false;

  m_currentLimit = value;

  return true;
}

bool CDirectoryProvider::UpdateBrowse()
{
  CSingleLock lock(m_section);
  const std::string stringValue(m_browse.GetLabel(m_parentID, false));
  BrowseMode value(m_currentBrowse);
  if (StringUtils::EqualsNoCase(stringValue, "always"))
    value = BrowseMode::ALWAYS;
  else if (StringUtils::EqualsNoCase(stringValue, "auto"))
    value = BrowseMode::AUTO;
  else if (StringUtils::EqualsNoCase(stringValue, "never"))
    value = BrowseMode::NEVER;

  if (value == m_currentBrowse)
    return false;

  m_currentBrowse = value;
  return true;
}

bool CDirectoryProvider::UpdateSort()
{
  CSingleLock lock(m_section);
  SortBy sortMethod(SortUtils::SortMethodFromString(m_sortMethod.GetLabel(m_parentID, false)));
  SortOrder sortOrder(SortUtils::SortOrderFromString(m_sortOrder.GetLabel(m_parentID, false)));
  if (sortOrder == SortOrderNone)
    sortOrder = SortOrderAscending;

  if (sortMethod == m_currentSort.sortBy && sortOrder == m_currentSort.sortOrder)
    return false;

  m_currentSort.sortBy = sortMethod;
  m_currentSort.sortOrder = sortOrder;
  m_currentSort.sortAttributes = SortAttributeIgnoreFolders;

  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("filelists.ignorethewhensorting"))
    m_currentSort.sortAttributes = static_cast<SortAttribute>(m_currentSort.sortAttributes | SortAttributeIgnoreArticle);

  return true;
}
