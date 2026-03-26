/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IListProvider.h"
#include "addons/AddonEvents.h"
#include "addons/RepositoryUpdater.h"
#include "guilib/GUIStaticItem.h"
#include "interfaces/IAnnouncer.h"
#include "threads/CriticalSection.h"
#include "utils/Job.h"

#include <string>
#include <vector>

class CFileItem;
class TiXmlElement;
class CVariant;

namespace InfoTagType
{
  enum TagType
  {
    VIDEO,
    AUDIO,
    PICTURE,
    PROGRAM
  };
}

class CDirectoryProvider :
  public IListProvider,
  public IJobCallback,
  public ANNOUNCEMENT::IAnnouncer
{
public:
  typedef enum
  {
    OK,
    INVALIDATED,
    DONE
  } UpdateState;

  enum BrowseMode
  {
    NEVER,
    AUTO, // add browse item if list is longer than given limit
    ALWAYS
  };

  CDirectoryProvider(const TiXmlElement *element, int parentID);
  explicit CDirectoryProvider(const CDirectoryProvider& other);
  virtual ~CDirectoryProvider();

  // Implementation of IListProvider
  virtual boost::movelib::unique_ptr<IListProvider> Clone();
  virtual bool Update(bool forceRefresh);
  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const char *sender,
                const char *message,
                const CVariant& data);
  virtual void Fetch(std::vector<boost::shared_ptr<CGUIListItem> >& items);
  virtual void Reset();
  virtual bool OnClick(const boost::shared_ptr<CGUIListItem>& item);
  bool OnInfo(const boost::shared_ptr<CFileItem>& item);
  bool OnContextMenu(const boost::shared_ptr<CFileItem>& item);
  virtual bool OnInfo(const boost::shared_ptr<CGUIListItem>& item);
  virtual bool OnContextMenu(const boost::shared_ptr<CGUIListItem>& item);
  virtual bool IsUpdating() const;
  virtual void FreeResources(bool immediately);

  // callback from directory job
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
private:
  UpdateState m_updateState;
  unsigned int m_jobID;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_url;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_target;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_sortMethod;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_sortOrder;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_limit;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_browse;
  std::string      m_currentUrl;
  std::string      m_currentTarget;   ///< \brief node.target property on the list as a whole
  SortDescription  m_currentSort;
  unsigned int m_currentLimit;
  BrowseMode m_currentBrowse;
  std::vector<CGUIStaticItemPtr> m_items;
  std::vector<InfoTagType::TagType> m_itemTypes;
  CCriticalSection m_section;

  bool UpdateURL();
  bool UpdateLimit();
  bool UpdateSort();
  bool UpdateBrowse();
  void OnAddonEvent(const ADDON::AddonEvent& event);
  void OnAddonRepositoryEvent(const ADDON::CRepositoryUpdater::RepositoryUpdated& event);
  std::string GetTarget(const CFileItem& item) const;

  CCriticalSection m_subscriptionSection;
  bool m_isSubscribed;
};
