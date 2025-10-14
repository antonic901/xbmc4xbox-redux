#pragma once
/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include <string>

#include "dbwrappers/Database.h"
#include "XBDateTime.h"

class CFileItem;
class CFileItemList;

#define PROGRAMDB_MAX_COLUMNS 24

typedef enum
{
  PROGRAMDB_ID_MIN = -1,
  PROGRAMDB_ID_PATH = 0,
  PROGRAMDB_ID_TYPE = 1,
  PROGRAMDB_ID_TITLE = 2,
  PROGRAMDB_ID_PLOT = 3,
  PROGRAMDB_ID_TRAILER = 19,
  PROGRAMDB_ID_POSTER = 20,
  PROGRAMDB_ID_FANART = 21,
  PROGRAMDB_ID_SIZE = 23,
  PROGRAMDB_ID_MAX
} PROGRAMDB_IDS;

class CProgramDatabase : public CDatabase
{
public:
  CProgramDatabase(void);
  virtual ~CProgramDatabase();

  virtual bool Open();

  int GetPathId(const std::string& strPath);
  int GetProgramId(const std::string& strFilenameAndPath);

  int AddPath(const std::string& strPath, const CDateTime& dateAdded = CDateTime());
  int AddProgram(const std::string& strFilenameAndPath, const int idPath);

  bool GetPathContent(const std::string& strPath, CFileItemList &items);
  bool GetPathContent(const int idPath, CFileItemList &items);

  int SetDetailsForItem(const CFileItem &item);

  void DeleteProgram(const std::string& strFilenameAndPath);
  void RemoveContentForPath(const std::string& strPath);

private:
  virtual void CreateTables();
  virtual void CreateAnalytics();

  virtual int GetSchemaVersion() const;
  const char *GetBaseDBName() const { return "MyPrograms"; };

  int RunQuery(const std::string &sql);
};
