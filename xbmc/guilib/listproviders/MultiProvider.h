/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IListProvider.h"
#include "threads/CriticalSection.h"

#include <map>
#include <vector>

typedef boost::movelib::unique_ptr<IListProvider> IListProviderPtr;

/*!
 \ingroup listproviders
 \brief A listprovider that handles multiple individual providers.
 */
class CMultiProvider : public IListProvider
{
public:
  CMultiProvider(const TiXmlNode *first, int parentID);
  explicit CMultiProvider(const CMultiProvider& other);

  // Implementation of IListProvider
  virtual boost::movelib::unique_ptr<IListProvider> Clone();
  virtual bool Update(bool forceRefresh);
  virtual void Fetch(std::vector<boost::shared_ptr<CGUIListItem> >& items);
  virtual bool IsUpdating() const;
  virtual void Reset();
  virtual bool OnClick(const boost::shared_ptr<CGUIListItem>& item);
  virtual bool OnInfo(const boost::shared_ptr<CGUIListItem>& item);
  virtual bool OnContextMenu(const boost::shared_ptr<CGUIListItem>& item);

protected:
  typedef size_t item_key_type;
  static item_key_type GetItemKey(boost::shared_ptr<CGUIListItem> const& item);
  std::vector<IListProviderPtr> m_providers;
  std::map<item_key_type, IListProvider*> m_itemMap;
  CCriticalSection m_section; // protects m_itemMap
};
