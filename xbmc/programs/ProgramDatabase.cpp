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

  columns += ")";
  m_pDS->exec(columns);
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

    strSQL=PrepareSQL("insert into program (idProgram, idPath, c%02d) values (NULL, %i, '%s')", PROGRAMDB_ID_PATH, idPath, strFilenameAndPath.c_str());

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
