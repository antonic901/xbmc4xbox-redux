/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

class CFileItem;
class CFileItemList;

class TiXmlElement;

namespace XFILE
{
  class CDAVDirectory : public IDirectory
  {
    public:
      CDAVDirectory(void);
      virtual ~CDAVDirectory(void);
      virtual bool GetDirectory(const CURL& url, CFileItemList &items);
      virtual bool Create(const CURL& url);
      virtual bool Exists(const CURL& url);
      virtual bool Remove(const CURL& url);
      virtual DIR_CACHE_TYPE GetCacheType(const CURL& url) const { return DIR_CACHE_ONCE; }

    private:
      void ParseResponse(const TiXmlElement* element, CFileItem& item);
  };
}
