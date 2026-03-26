/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MultiProvider.h"

#include "threads/SingleLock.h"
#include "utils/XBMCTinyXML.h"


CMultiProvider::CMultiProvider(const TiXmlNode *first, int parentID)
 : IListProvider(parentID)
{
  for (const TiXmlNode *content = first; content; content = content->NextSiblingElement("content"))
  {
    IListProviderPtr sub(IListProvider::CreateSingle(content, parentID));
    if (sub)
      m_providers.push_back(boost::move(sub));
  }
}

CMultiProvider::CMultiProvider(const CMultiProvider& other) : IListProvider(other.m_parentID)
{
  for (std::vector<IListProviderPtr>::const_iterator it = other.m_providers.begin(); it != other.m_providers.end(); ++it)
  {
   boost::movelib::unique_ptr<IListProvider> newProvider = (*it)->Clone();
   if (newProvider)
     m_providers.push_back(boost::move(newProvider));
  }
}

boost::movelib::unique_ptr<IListProvider> CMultiProvider::Clone()
{
  return boost::movelib::unique_ptr<IListProvider>(new CMultiProvider(*this));
}

bool CMultiProvider::Update(bool forceRefresh)
{
  bool result = false;
  for (std::vector<IListProviderPtr>::iterator provider = m_providers.begin(); provider != m_providers.end(); ++provider)
    result |= (*provider)->Update(forceRefresh);
  return result;
}

void CMultiProvider::Fetch(std::vector<boost::shared_ptr<CGUIListItem> >& items)
{
  CSingleLock lock(m_section);
  std::vector<boost::shared_ptr<CGUIListItem> > subItems;
  items.clear();
  m_itemMap.clear();
  for (std::vector<IListProviderPtr>::const_iterator provider = m_providers.begin(); provider != m_providers.end(); ++provider)
  {
    (*provider)->Fetch(subItems);
    for (std::vector<boost::shared_ptr<CGUIListItem> >::iterator item = subItems.begin(); item != subItems.end(); ++item)
    {
      CMultiProvider::item_key_type key = GetItemKey(*item);
      m_itemMap[key] = (*provider).get();
      items.push_back(*item);
    }
    subItems.clear();
  }
}

bool CMultiProvider::IsUpdating() const
{
  bool result = false;
  for (std::vector<IListProviderPtr>::const_iterator provider = m_providers.begin(); provider != m_providers.end(); ++provider)
    result |= (*provider)->IsUpdating();
  return result;
}

void CMultiProvider::Reset()
{
  {
    CSingleLock lock(m_section);
    m_itemMap.clear();
  }

  for (std::vector<IListProviderPtr>::const_iterator provider = m_providers.begin(); provider != m_providers.end(); ++provider)
    (*provider)->Reset();
}

bool CMultiProvider::OnClick(const boost::shared_ptr<CGUIListItem>& item)
{
  CSingleLock lock(m_section);
  CMultiProvider::item_key_type key = GetItemKey(item);
  std::map<CMultiProvider::item_key_type, IListProvider *>::iterator it = m_itemMap.find(key);
  if (it != m_itemMap.end())
    return it->second->OnClick(item);
  else
    return false;
}

bool CMultiProvider::OnInfo(const boost::shared_ptr<CGUIListItem>& item)
{
  CSingleLock lock(m_section);
  CMultiProvider::item_key_type key = GetItemKey(item);
  std::map<CMultiProvider::item_key_type, IListProvider *>::iterator it = m_itemMap.find(key);
  if (it != m_itemMap.end())
    return it->second->OnInfo(item);
  else
    return false;
}

bool CMultiProvider::OnContextMenu(const boost::shared_ptr<CGUIListItem>& item)
{
  CSingleLock lock(m_section);
  CMultiProvider::item_key_type key = GetItemKey(item);
  std::map<CMultiProvider::item_key_type, IListProvider *>::iterator it = m_itemMap.find(key);
  if (it != m_itemMap.end())
    return it->second->OnContextMenu(item);
  else
    return false;
}

CMultiProvider::item_key_type CMultiProvider::GetItemKey(boost::shared_ptr<CGUIListItem> const& item)
{
  return reinterpret_cast<item_key_type>(item.get());
}
