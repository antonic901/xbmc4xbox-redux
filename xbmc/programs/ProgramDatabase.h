/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

#include "ProgramInfoTag.h"
#include "addons/Scraper.h"
#include "dbwrappers/Database.h"
#include "video/VideoDatabase.h" // SDbTableOffsets, my_offsetof
#include "XBDateTime.h"

class CFileItem;
class CFileItemList;
class CTrainer;

#define PROGRAMDB_MAX_COLUMNS 24

  // TODO: clean this up - copy/paste from CVideoDatabase
#define PROGRAMDB_TYPE_UNUSED 0
#define PROGRAMDB_TYPE_STRING 1
#define PROGRAMDB_TYPE_INT 2
#define PROGRAMDB_TYPE_FLOAT 3
#define PROGRAMDB_TYPE_BOOL 4
#define PROGRAMDB_TYPE_COUNT 5
#define PROGRAMDB_TYPE_STRINGARRAY 6
#define PROGRAMDB_TYPE_DATE 7
#define PROGRAMDB_TYPE_DATETIME 8

#define PROGRAMDB_DETAILS_PROGRAM_PLAYCOUNT         PROGRAMDB_MAX_COLUMNS + 1
#define PROGRAMDB_DETAILS_PROGRAM_LASTPLAYED        PROGRAMDB_MAX_COLUMNS + 2
#define PROGRAMDB_DETAILS_PROGRAM_DATEADDED         PROGRAMDB_MAX_COLUMNS + 3

typedef enum
{
  PROGRAMDB_ID_MIN = -1,
  PROGRAMDB_ID_PATH = 0,
  PROGRAMDB_ID_UNIQUE_ID = 1,
  PROGRAMDB_ID_TYPE = 2,
  PROGRAMDB_ID_TITLE = 3,
  PROGRAMDB_ID_PLOT = 4,
  PROGRAMDB_ID_SYSTEM = 5,
  PROGRAMDB_ID_RATING = 6,
  PROGRAMDB_ID_EXCLUSIVE = 7,
  PROGRAMDB_ID_ESRB = 8,
  PROGRAMDB_ID_DEVELOPER = 10,
  PROGRAMDB_ID_PUBLISHER = 11,
  PROGRAMDB_ID_GENRE = 12,
  PROGRAMDB_ID_GENERALFEATURE = 14,
  PROGRAMDB_ID_ONLINEFEATURE = 15,
  PROGRAMDB_ID_PLATFORM = 16,
  PROGRAMDB_ID_TRAILER = 19,
  PROGRAMDB_ID_POSTER = 20,
  PROGRAMDB_ID_FANART = 21,
  PROGRAMDB_ID_RELEASED = 22,
  PROGRAMDB_ID_SIZE = 23,
  PROGRAMDB_ID_MAX
} PROGRAMDB_IDS;

const struct SDbTableOffsets DbProgramOffsets[] =
{
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strFileNameAndPath) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strUniqueID) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_type) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strTitle) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strPlot) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strSystem) },
  { PROGRAMDB_TYPE_FLOAT, my_offsetof(CProgramInfoTag,m_rating) },
  { PROGRAMDB_TYPE_BOOL, my_offsetof(CProgramInfoTag,m_bExclusive) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strESRB) },
  { PROGRAMDB_TYPE_UNUSED, 0 }, // unused
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_developer) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_publisher) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_genre) },
  { PROGRAMDB_TYPE_UNUSED, 0 }, // unused
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_generalFeature) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_onlineFeature) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_platform) },
  { PROGRAMDB_TYPE_UNUSED, 0 }, // unused
  { PROGRAMDB_TYPE_UNUSED, 0 }, // unused
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strTrailer) },
  { PROGRAMDB_TYPE_UNUSED, 0 }, // unused
  { PROGRAMDB_TYPE_UNUSED, 0 }, // unused
  { PROGRAMDB_TYPE_DATE, my_offsetof(CProgramInfoTag,m_releaseDate) },
  { PROGRAMDB_TYPE_UNUSED, 0 } // unused
};

class CProgramDatabase : public CDatabase
{
public:
  CProgramDatabase(void);
  virtual ~CProgramDatabase();

  virtual bool Open();

  // Trainers
  bool AddTrainer(int idTitle, CTrainer &trainer);
  bool RemoveTrainer(int idTrainer);
  bool SetTrainer(int idTitle, CTrainer *trainer);
  bool GetTrainers(CFileItemList& items, unsigned int idTitle = 0);
  bool GetTrainerOptions(int idTrainer, unsigned int iTitleId, unsigned char* data, int numOptions);
  bool HasTrainer(const std::string& strTrainerPath);

  int GetPathId(const std::string& strPath);
  int GetProgramId(const std::string& strFilenameAndPath);

  int AddPath(const std::string& strPath, const CDateTime& dateAdded = CDateTime());
  int AddProgram(const std::string& strFilenameAndPath, const int idPath);

  bool GetPathContent(const std::string& strPath, CFileItemList &items);
  bool GetPathContent(const int idPath, CFileItemList &items);

  int SetDetailsForItem(const CFileItem* item);

  void DeleteProgram(const std::string& strFilenameAndPath);
  void RemoveContentForPath(const std::string& strPath);

  // scraper settings
  void SetScraperForPath(const std::string& strPath, const ADDON::ScraperPtr& scraper);
  ADDON::ScraperPtr GetScraperForPath(const std::string& strPath);

  // Program settings
  bool SetProgramSettings(const std::string& strFileNameAndPath, const std::string& strSettings);
  bool GetProgramSettings(const std::string& strFileNameAndPath, std::string& strSettings);

  // Emulators
  bool GetEmulators(const std::string& shortname, CFileItemList& emulators);

  /*! \brief Update the last played time of program
   Updates the last played date and play count
   \param strFilenameAndPath program to update the last played time for
   */
  void UpdateLastPlayed(const std::string& strFilenameAndPath);

  std::string GetXBEPathByTitleId(const std::string& idTitle);

  bool GetRecentlyPlayedGames(CFileItemList &items);

protected:
  void GetDetailsForItem(boost::movelib::unique_ptr<dbiplus::Dataset> &pDS, CFileItem* pItem);

private:
  virtual void CreateTables();
  virtual void CreateAnalytics();

  virtual int GetSchemaVersion() const;
  const char *GetBaseDBName() const { return "MyPrograms"; };

  int RunQuery(const std::string &sql);

  // TODO: clean this up - copy/paste from CVideoDatabase
  std::string GetValueString(const CProgramInfoTag &details, int min, int max, const SDbTableOffsets *offsets) const;
  void GetDetailsFromDB(const dbiplus::sql_record* const record, int min, int max, const SDbTableOffsets *offsets, CProgramInfoTag &details, int idxOffset = 2);
};
