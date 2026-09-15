/*
 *  Copyright (C) 2025-2025
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ProgramDatabaseDirectory.h"

#include "programs/ProgramDatabase.h"
#include "URL.h"

using namespace XFILE;

CProgramDatabaseDirectory::CProgramDatabaseDirectory(void)
{
}

CProgramDatabaseDirectory::~CProgramDatabaseDirectory(void)
{
}

bool CProgramDatabaseDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  CProgramDatabase database;
  if (!database.Open())
    return false;

  if (url.Get() == "programdb://games/recentlyplayed/")
    return database.GetRecentlyPlayedGames(items);

  int idPath = atoi(url.GetShareName().c_str());
  return database.GetPathContent(idPath, items);
}
