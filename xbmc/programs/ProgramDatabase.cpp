/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ProgramDatabase.h"

#include "dbwrappers/dataset.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace dbiplus;

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
