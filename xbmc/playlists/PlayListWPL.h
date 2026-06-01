/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlayList.h"

namespace PLAYLIST
{

class CPlayListWPL :
      public CPlayList
{
public:
  CPlayListWPL(void);
  virtual ~CPlayListWPL(void);
  virtual bool LoadData(std::istream& stream);
  virtual void Save(const std::string& strFileName) const;
};
}
