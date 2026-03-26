/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IListProvider.h"
#include "guilib/GUIStaticItem.h"

#include <vector>

class CStaticListProvider : public IListProvider
{
public:
  CStaticListProvider(const TiXmlElement *element, int parentID);
  explicit CStaticListProvider(const std::vector<CGUIStaticItemPtr> &items); // for python
  explicit CStaticListProvider(const CStaticListProvider& other);
  virtual ~CStaticListProvider();

  // Implementation of IListProvider
  virtual boost::movelib::unique_ptr<IListProvider> Clone();
  virtual bool Update(bool forceRefresh);
  virtual void Fetch(std::vector<boost::shared_ptr<CGUIListItem> >& items);
  virtual bool OnClick(const boost::shared_ptr<CGUIListItem>& item);
  virtual bool OnInfo(const boost::shared_ptr<CGUIListItem>& item) { return false; }
  virtual bool OnContextMenu(const boost::shared_ptr<CGUIListItem>& item) { return false; }
  virtual void SetDefaultItem(int item, bool always);
  virtual int GetDefaultItem() const;
  virtual bool AlwaysFocusDefaultItem() const;
private:
  int m_defaultItem;
  bool m_defaultAlways;
  unsigned int m_updateTime;
  std::vector<CGUIStaticItemPtr> m_items;
};
