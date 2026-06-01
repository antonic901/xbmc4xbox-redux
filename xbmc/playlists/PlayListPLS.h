/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlayList.h"

#include <string>
#include <vector>

namespace PLAYLIST
{
class CPlayListPLS :
      public CPlayList
{
public:
  CPlayListPLS(void);
  virtual ~CPlayListPLS(void);
  virtual bool Load(const std::string& strFileName);
  virtual void Save(const std::string& strFileName) const;
  virtual bool Resize(std::vector<int>::size_type newSize);
};

class CPlayListASX : public CPlayList
{
public:
  virtual bool LoadData(std::istream &stream);
protected:
  bool LoadAsxIniInfo(std::istream &stream);
};

class CPlayListRAM : public CPlayList
{
public:
  virtual bool LoadData(std::istream &stream);
};


}
