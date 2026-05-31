/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BackgroundInfoLoader.h"
#include "MusicDatabase.h"

class CFileItemList;
class CMusicThumbLoader;

namespace MUSIC_INFO
{
class CMusicInfoLoader : public CBackgroundInfoLoader
{
public:
  CMusicInfoLoader();
  virtual ~CMusicInfoLoader();

  void UseCacheOnHD(const std::string& strFileName);
  virtual bool LoadItem(CFileItem* pItem);
  virtual bool LoadItemCached(CFileItem* pItem);
  virtual bool LoadItemLookup(CFileItem* pItem);
  static bool LoadAdditionalTagInfo(CFileItem* pItem);

protected:
  virtual void OnLoaderStart();
  virtual void OnLoaderFinish();
  void LoadCache(const std::string& strFileName, CFileItemList& items);
  void SaveCache(const std::string& strFileName, CFileItemList& items);
protected:
  std::string m_strCacheFileName;
  CFileItemList* m_mapFileItems;
  MAPSONGS m_songsMap;
  std::string m_strPrevPath;
  CMusicDatabase m_musicDatabase;
  unsigned int m_databaseHits;
  unsigned int m_tagReads;
  CMusicThumbLoader *m_thumbLoader;
};
}
