#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "dbwrappers/Database.h"
#include "utils/StdString.h"

typedef std::vector<CStdString> VECPROGRAMPATHS;

#define COMPARE_PERCENTAGE     0.90f // 90%
#define COMPARE_PERCENTAGE_MIN 0.50f // 50%

class CFileItem;
class CFileItemList;
class CTrainer;

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

  int GetRegion(const CStdString& strFilenameAndPath);
  bool SetRegion(const CStdString& strFilenameAndPath, int iRegion=-1);

  int GetTitleId(const CStdString& strFilenameAndPath);
  bool SetTitleId(const CStdString& strFilenameAndPath, int idTitle);
  bool IncTimesPlayed(const CStdString& strFileName1);
  bool SetDescription(const CStdString& strFileName1, const CStdString& strDescription);
  bool GetXBEPathByTitleId(const int idTitle, std::string& strPathAndFilename);

  int GetProgramInfo(CFileItem *item);
  bool AddProgramInfo(CFileItem *item, unsigned int titleID);

  bool GetArbitraryQuery(const CStdString& strQuery, const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet,
                         const CStdString& strOpenRecord, const CStdString& strCloseRecord, const CStdString& strOpenField, const CStdString& strCloseField, CStdString& strResult);

protected:
  virtual void CreateTables();
  virtual void CreateAnalytics();
  virtual void UpdateTables(int version);
  virtual int GetSchemaVersion() const { return 1; }
  const char *GetBaseDBName() const { return "MyPrograms"; }

  FILETIME TimeStampToLocalTime( unsigned __int64 timeStamp );
};
