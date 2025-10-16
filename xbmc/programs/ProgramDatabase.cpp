/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ProgramDatabase.h"

#include "dbwrappers/dataset.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Trainer.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

using namespace dbiplus;
using namespace XFILE;

CProgramDatabase::CProgramDatabase(void)
{
}

CProgramDatabase::~CProgramDatabase(void)
{}

bool CProgramDatabase::Open()
{
  return CDatabase::Open();
}

void CProgramDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create path table");
  m_pDS->exec("CREATE TABLE path (idPath integer primary key, strPath text, strContent text, strHash text, dateAdded text)");

  CLog::Log(LOGINFO, "create program table");
  std::string columns = "CREATE TABLE program (idProgram integer primary key, idPath integer";

  for (int i = 0; i < PROGRAMDB_MAX_COLUMNS; i++)
    columns += StringUtils::Format(",c%02d text", i);

  columns += ", playCount integer, lastPlayed text, dateAdded text, strSettings text)";
  m_pDS->exec(columns);

  CLog::Log(LOGINFO, "create trainers table");
  m_pDS->exec("CREATE TABLE trainers (idTrainer integer primary key, idTitle integer, strTrainerPath text, strSettings text, Active integer)\n");
}

void CProgramDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s - creating indicies", __FUNCTION__);
  m_pDS->exec("CREATE INDEX ix_path ON path ( strPath(255) )");

  m_pDS->exec("CREATE UNIQUE INDEX ix_program_file_1 ON program (idPath, idProgram)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_program_file_2 ON program (idProgram, idPath)");
}

int CProgramDatabase::GetSchemaVersion() const
{
  return 1;
}

int CProgramDatabase::RunQuery(const std::string &sql)
{
  unsigned int time = XbmcThreads::SystemClockMillis();
  int rows = -1;
  if (m_pDS->query(sql))
  {
    rows = m_pDS->num_rows();
    if (rows == 0)
      m_pDS->close();
  }
  CLog::Log(LOGDEBUG, "%s took %d ms for %d items query: %s", __FUNCTION__, XbmcThreads::SystemClockMillis() - time, rows, sql.c_str());
  return rows;
}

bool CProgramDatabase::AddTrainer(int idTitle, CTrainer &trainer)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    char* temp = new char[trainer.GetNumberOfOptions() + 1];
    int i;
    for(i = 0; i < trainer.GetNumberOfOptions(); ++i)
      temp[i] = '0';
    temp[i] = '\0';

    strSQL = PrepareSQL("insert into trainers (idTrainer, idTitle, strTrainerPath, strSettings, Active) values (NULL, %u, '%s', '%s', %i)", idTitle, trainer.GetPath().c_str(), temp, 0);
    m_pDS->exec(strSQL.c_str());

    delete[] temp;
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::RemoveTrainer(int idTrainer)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    strSQL = PrepareSQL("delete from trainers where idTrainer=%i", idTrainer);
    return m_pDS->exec(strSQL.c_str()) == 0;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::SetTrainer(int idTitle, CTrainer *trainer)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // deactivate all trainers
    strSQL = PrepareSQL("update trainers set Active=%u where idTitle=%u", 0, idTitle);
    m_pDS->exec(strSQL.c_str());

    if (trainer == nullptr)
      return true;

    // set current trainer as active
    char* temp = new char[trainer->GetNumberOfOptions() + 1];
    int i;
    for (i = 0; i < trainer->GetNumberOfOptions(); ++i)
    {
      if (trainer->GetOptions()[i] == 1)
        temp[i] = '1';
      else
        temp[i] = '0';
    }
    temp[i] = '\0';

    strSQL = PrepareSQL("update trainers set Active=%u, strSettings='%s' where idTrainer=%i and idTitle=%u", 1, temp, trainer->GetTrainerId(), idTitle);
    m_pDS->exec(strSQL.c_str());

    delete[] temp;
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::GetTrainers(CFileItemList& items, unsigned int idTitle /* = 0 */)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    strSQL = PrepareSQL("select * from trainers");
    if (idTitle)
      strSQL += PrepareSQL(" where idTitle = %u", idTitle);
    if (!m_pDS->query(strSQL.c_str()))
      return false;

    while (!m_pDS->eof())
    {
      std::string strPath = m_pDS->fv("strTrainerPath").get_asString();
      CFileItemPtr pItem(new CFileItem(strPath, false));
      items.Add(pItem);
      pItem->SetProperty("idtrainer", m_pDS->fv("idTrainer").get_asInt());
      pItem->SetProperty("isactive", m_pDS->fv("Active").get_asBool());
      m_pDS->next();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::GetTrainerOptions(int idTrainer, unsigned int idTitle, unsigned char* data, int numOptions)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    strSQL = PrepareSQL("select * from trainers where idTrainer=%i and idTitle=%u", idTrainer, idTitle);
    if (!m_pDS->query(strSQL.c_str()))
      return false;

    std::string strSettings = m_pDS->fv("strSettings").get_asString();
    for (int i = 0; i < numOptions && i < 100; i++)
      data[i] = strSettings[i] == '1' ? 1 : 0;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::HasTrainer(const std::string& strTrainerPath)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    strSQL = PrepareSQL("SELECT strTrainerPath FROM trainers WHERE strTrainerPath='%s'", strTrainerPath.c_str());
    return !GetSingleValue(strSQL).empty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

int CProgramDatabase::GetPathId(const std::string& strPath)
{
  std::string strSQL;
  try
  {
    int idPath=-1;
    if (NULL == m_pDB.get())
      return -1;
    if (NULL == m_pDS.get())
      return -1;

    std::string strPath1(strPath);
    URIUtils::AddSlashAtEnd(strPath1);

    strSQL=PrepareSQL("select idPath from path where strPath='%s'", strPath1.c_str());
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
      idPath = m_pDS->fv("path.idPath").get_asInt();

    m_pDS->close();
    return idPath;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to getpath (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

int CProgramDatabase::AddPath(const std::string& strPath, const CDateTime& dateAdded /* = CDateTime() */)
{
  std::string strSQL;
  try
  {
    int idPath = GetPathId(strPath);
    if (idPath >= 0)
      return idPath; // already have the path

    if (NULL == m_pDB.get())
      return -1;
    if (NULL == m_pDS.get())
      return -1;

    std::string strPath1(strPath);
    URIUtils::AddSlashAtEnd(strPath1);

    // add the path
    if (dateAdded.IsValid())
      strSQL=PrepareSQL("insert into path (idPath, strPath, dateAdded) values (NULL, '%s', '%s')", strPath1.c_str(), dateAdded.GetAsDBDateTime().c_str());
    else
      strSQL=PrepareSQL("insert into path (idPath, strPath) values (NULL, '%s')", strPath1.c_str());

    m_pDS->exec(strSQL);
    idPath = (int)m_pDS->lastinsertid();
    return idPath;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to addpath (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

int CProgramDatabase::GetProgramId(const std::string& strFilenameAndPath)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get())
      return -1;
    if (NULL == m_pDS.get())
      return -1;

    int idProgram = -1;

    strSQL = PrepareSQL("select idProgram from program where c%02d='%s'", PROGRAMDB_ID_PATH, strFilenameAndPath.c_str());
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
      idProgram = m_pDS->fv("idProgram").get_asInt();

    m_pDS->close();
    return idProgram;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to getprogramid (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

int CProgramDatabase::AddProgram(const std::string& strFilenameAndPath, const int idPath)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get())
      return -1;
    if (NULL == m_pDS.get())
      return -1;

    int idProgram = GetProgramId(strFilenameAndPath);
    if (idProgram > 0)
      return idProgram;

    strSQL=PrepareSQL("insert into program (idProgram, idPath, c%02d, playCount, dateAdded) values (NULL, %i, '%s', 0, '%s')", PROGRAMDB_ID_PATH, idPath, strFilenameAndPath.c_str(), CDateTime::GetCurrentDateTime().GetAsDBDateTime().c_str());

    m_pDS->exec(strSQL);
    idProgram = (int)m_pDS->lastinsertid();
    return idProgram;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to addprogram (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

int CProgramDatabase::SetDetailsForItem(const CFileItem &item)
{
  int idProgram = GetProgramId(item.GetPath());
  if (idProgram < 0)
    return -1;

  try
  {
    if (NULL == m_pDB.get())
      return false;
    if (NULL == m_pDS.get())
      return false;

    std::string value;
    std::vector<std::string> conditions;

    value = item.GetProperty("type").asString();
    if (!value.empty())
      conditions.push_back(PrepareSQL("c%02d='%s'", PROGRAMDB_ID_TYPE, value.c_str()));

    value = item.GetProperty("uniqueid").asString();
    if (!value.empty())
      conditions.push_back(PrepareSQL("c%02d='%s'", PROGRAMDB_ID_UNIQUE_ID, value.c_str()));

    value = item.GetProperty("title").asString();
    if (!value.empty())
      conditions.push_back(PrepareSQL("c%02d='%s'", PROGRAMDB_ID_TITLE, value.c_str()));

    value = item.GetProperty("overview").asString();
    if (!value.empty())
      conditions.push_back(PrepareSQL("c%02d='%s'", PROGRAMDB_ID_PLOT, value.c_str()));

    value = item.GetProperty("trailer").asString();
    if (!value.empty())
      conditions.push_back(PrepareSQL("c%02d='%s'", PROGRAMDB_ID_TRAILER, value.c_str()));

    value = item.GetArt("poster");
    if (!value.empty())
      conditions.push_back(PrepareSQL("c%02d='%s'", PROGRAMDB_ID_POSTER, value.c_str()));

    value = item.GetArt("fanart");
    if (!value.empty())
      conditions.push_back(PrepareSQL("c%02d='%s'", PROGRAMDB_ID_FANART, value.c_str()));

    conditions.push_back(PrepareSQL("c%02d=%I64u", PROGRAMDB_ID_SIZE, item.GetProperty("size").asInteger()));

    std::string sql = "UPDATE program SET " + StringUtils::Join(conditions, ",") + PrepareSQL(" where idProgram=%i", idProgram);
    m_pDS->exec(sql);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, item.GetPath().c_str());
  }
  return -1;
}

bool CProgramDatabase::GetPathContent(const std::string& strPath, CFileItemList &items)
{
  int idPath = GetPathId(strPath);
  if (idPath < 0)
    return false;

  return GetPathContent(idPath, items);
}

bool CProgramDatabase::GetPathContent(const int idPath, CFileItemList &items)
{
  try
  {
    if (NULL == m_pDB.get())
      return false;
    if (NULL == m_pDS.get())
      return false;

    std::string strSQL = PrepareSQL("select * from program where idPath=%i", idPath);
    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return false;

    // store the total value of items as a property
    items.SetProperty("total", iRowsFound);

    while (!m_pDS->eof())
    {
      CFileItemPtr pItem(new CFileItem());
      std::string path = m_pDS->fv(PROGRAMDB_ID_PATH + 2).get_asString();
      std::string title = m_pDS->fv(PROGRAMDB_ID_TITLE + 2).get_asString();
      std::string plot = m_pDS->fv(PROGRAMDB_ID_PLOT + 2).get_asString();
      std::string poster = m_pDS->fv(PROGRAMDB_ID_POSTER + 2).get_asString();
      std::string fanart = m_pDS->fv(PROGRAMDB_ID_FANART + 2).get_asString();
      std::string trailer = m_pDS->fv(PROGRAMDB_ID_TRAILER + 2).get_asString();
      pItem->SetPath(path);
      pItem->SetLabel(title);
      pItem->SetProperty("title", title);
      pItem->SetLabel2(plot);
      pItem->SetProperty("overview", plot);
      pItem->SetProperty("trailer", trailer);
      pItem->SetArt("poster", poster);
      pItem->SetArt("fanart", fanart);
      pItem->m_dwSize = m_pDS->fv(PROGRAMDB_ID_SIZE + 2).get_asInt64();

      items.Add(pItem);
      m_pDS->next();
    }

    // cleanup
    m_pDS->close();
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s unable to retrieve items (%s)", __FUNCTION__);
  }
  return false;
}

void CProgramDatabase::DeleteProgram(const std::string& strFilenameAndPath)
{
  int idProgram = GetProgramId(strFilenameAndPath);
  if (idProgram < 0)
    return;

  try
  {
    if (NULL == m_pDB.get())
      return;
    if (NULL == m_pDS.get())
      return;

    std::string strSQL = PrepareSQL("delete from program where idProgram=%i", idProgram);
    m_pDS->exec(strSQL);
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CProgramDatabase::RemoveContentForPath(const std::string& strPath)
{
  int idPath = GetPathId(strPath);
  if (idPath < 0)
    return;

  try
  {
    if (NULL == m_pDB.get())
      return;
    if (NULL == m_pDS.get())
      return;

    std::string strSQL = PrepareSQL("delete from program where idPath=%i", idPath);
    m_pDS->exec(strSQL);
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
}

bool CProgramDatabase::SetProgramSettings(const std::string& strFileNameAndPath, const std::string& strSettings)
{
  std::string strSQL = "";
  try
  {
    if (NULL == m_pDB.get())
      return false;
    if (NULL == m_pDS.get())
      return false;

    int idProgram = GetProgramId(strFileNameAndPath);
    if (idProgram < 0)
      return false;

    strSQL = PrepareSQL("UPDATE program SET strSettings='%s' WHERE idProgram=%i", strSettings.c_str(), idProgram);
    m_pDS->exec(strSQL);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::GetProgramSettings(const std::string& strFileNameAndPath, std::string& strSettings)
{
  std::string strSQL = "";
  try
  {
    if (NULL == m_pDB.get())
      return false;
    if (NULL == m_pDS.get())
      return false;

    int idProgram = GetProgramId(strFileNameAndPath);
    if (idProgram < 0)
      return false;

    strSQL = PrepareSQL("SELECT strSettings FROM program WHERE idProgram=%i", idProgram);
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
      strSettings = m_pDS->fv("strSettings").get_asString();
    m_pDS->close();
    return !strSettings.empty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

void CProgramDatabase::UpdateLastPlayed(const std::string& strFilenameAndPath)
{
  int idProgram = GetProgramId(strFilenameAndPath);
  if (idProgram < 0)
    return;

  try
  {
    if (NULL == m_pDB.get())
      return;
    if (NULL == m_pDS.get())
      return;

    std::string strSQL = PrepareSQL("UPDATE program SET lastPlayed='%s', playCount=playCount + 1 WHERE idProgram=%i", CDateTime::GetCurrentDateTime().GetAsDBDateTime().c_str(), idProgram);
    m_pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

std::string CProgramDatabase::GetXBEPathByTitleId(const std::string& idTitle)
{
  try
  {
    if (NULL == m_pDB.get())
      return "";
    if (NULL == m_pDS.get())
      return "";

    std::string strPath;

    std::string strSQL = PrepareSQL("select c%02d from program where c%02d='%s'", PROGRAMDB_ID_PATH, PROGRAMDB_ID_UNIQUE_ID, idTitle.c_str());
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
      strPath = m_pDS->fv(StringUtils::Format("c%02d", PROGRAMDB_ID_PATH).c_str()).get_asString();

    m_pDS->close();
    return strPath;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idTitle);
  }
  return "";
}
