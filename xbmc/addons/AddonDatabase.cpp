/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonDatabase.h"

#include "XBDateTime.h"
#include "addons/AddonBuilder.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "dbwrappers/dataset.h"
#include "filesystem/SpecialProtocol.h"
#include "threads/SystemClock.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <utility>

using namespace ADDON;

std::string CAddonDatabaseSerializer::SerializeMetadata(const CAddonInfo& addon)
{
  CVariant variant;
  variant["author"] = addon.Author();
  variant["disclaimer"] = addon.Disclaimer();
  variant["lifecycletype"] = static_cast<unsigned int>(addon.LifecycleState());
  variant["lifecycledesc"] = addon.LifecycleStateDescription();
  variant["size"] = addon.PackageSize();

  variant["path"] = addon.Path();
  variant["icon"] = addon.Icon();

  variant["art"] = CVariant(CVariant::VariantTypeObject);
  for (ArtMap::const_iterator item = addon.Art().begin(); item != addon.Art().end(); ++item)
    variant["art"][item->first] = item->second;

  variant["screenshots"] = CVariant(CVariant::VariantTypeArray);
  for (std::vector<std::string>::const_iterator item = addon.Screenshots().begin(); item != addon.Screenshots().end(); ++item)
    variant["screenshots"].push_back(*item);

  variant["extensions"] = CVariant(CVariant::VariantTypeArray);
  variant["extensions"].push_back(SerializeExtensions(*addon.Type(addon.MainType())));

  variant["dependencies"] = CVariant(CVariant::VariantTypeArray);
  for (std::vector<DependencyInfo>::const_iterator dep = addon.GetDependencies().begin(); dep != addon.GetDependencies().end(); ++dep)
  {
    CVariant info(CVariant::VariantTypeObject);
    info["addonId"] = dep->id;
    info["version"] = dep->version.asString();
    info["minversion"] = dep->versionMin.asString();
    info["optional"] = dep->optional;
    variant["dependencies"].push_back(info);
  }

  variant["extrainfo"] = CVariant(CVariant::VariantTypeArray);
  for (InfoMap::const_iterator kv = addon.ExtraInfo().begin(); kv != addon.ExtraInfo().end(); ++kv)
  {
    CVariant info(CVariant::VariantTypeObject);
    info["key"] = kv->first;
    info["value"] = kv->second;
    variant["extrainfo"].push_back(info);
  }

  return CJSONVariantWriter::Write(variant, true);
}

CVariant CAddonDatabaseSerializer::SerializeExtensions(const CAddonExtensions& addonType)
{
  CVariant variant;
  variant["type"] = addonType.m_point;

  variant["values"] = CVariant(CVariant::VariantTypeArray);
  for (EXT_VALUES::const_iterator value = addonType.m_values.begin(); value != addonType.m_values.end(); ++value)
  {
    CVariant info(CVariant::VariantTypeObject);
    info["id"] = value->first;
    info["content"] = CVariant(CVariant::VariantTypeArray);

    for (CExtValues::const_iterator content = value->second.begin(); content != value->second.end(); ++content)
    {
      CVariant contentEntry(CVariant::VariantTypeObject);
      contentEntry["key"] = content->first;
      contentEntry["value"] = content->second.str;
      info["content"].push_back(contentEntry);
    }

    variant["values"].push_back(info);
  }

  variant["children"] = CVariant(CVariant::VariantTypeArray);
  for (EXT_ELEMENTS::const_iterator child = addonType.m_children.begin(); child != addonType.m_children.end(); ++child)
  {
    CVariant info(CVariant::VariantTypeObject);
    info["id"] = child->first;
    info["child"] = SerializeExtensions(child->second);
    variant["children"].push_back(info);
  }

  return variant;
}

void CAddonDatabaseSerializer::DeserializeMetadata(const std::string& document,
                                                   CAddonInfoBuilderFromDB& builder)
{
  CVariant variant = CJSONVariantParser::Parse(document);

  builder.SetAuthor(variant["author"].asString());
  builder.SetDisclaimer(variant["disclaimer"].asString());
  builder.SetLifecycleState(
      static_cast<AddonLifecycleState::Type>(variant["lifecycletype"].asUnsignedInteger()),
      variant["lifecycledesc"].asString());
  builder.SetPackageSize(variant["size"].asUnsignedInteger());

  builder.SetPath(variant["path"].asString());
  builder.SetIcon(variant["icon"].asString());

  std::map<std::string, std::string> art;
  for (CVariant::iterator_map it = variant["art"].begin_map(); it != variant["art"].end_map(); ++it)
    art.insert(std::make_pair(it->first, it->second.asString()));
  builder.SetArt(boost::move(art));

  std::vector<std::string> screenshots;
  for (CVariant::iterator_array it = variant["screenshots"].begin_array(); it != variant["screenshots"].end_array(); ++it)
    screenshots.push_back(it->asString());
  builder.SetScreenshots(boost::move(screenshots));

  CAddonType addonType;
  DeserializeExtensions(variant["extensions"][0], addonType);
  addonType.m_type = CAddonInfo::TranslateType(addonType.m_point);
  builder.SetExtensions(boost::move(addonType));

  {
    std::vector<DependencyInfo> deps;
    for (CVariant::iterator_array it = variant["dependencies"].begin_array(); it != variant["dependencies"].end_array(); ++it)
    {
      deps.push_back(DependencyInfo((*it)["addonId"].asString(), CAddonVersion((*it)["minversion"].asString()),
                        CAddonVersion((*it)["version"].asString()), (*it)["optional"].asBoolean()));
    }
    builder.SetDependencies(boost::move(deps));
  }

  InfoMap extraInfo;
  for (CVariant::iterator_array it = variant["extrainfo"].begin_array(); it != variant["extrainfo"].end_array(); ++it)
    extraInfo.insert(std::make_pair((*it)["key"].asString(), (*it)["value"].asString()));
  builder.SetExtrainfo(boost::move(extraInfo));
}

void CAddonDatabaseSerializer::DeserializeExtensions(const CVariant& variant,
                                                     CAddonExtensions& addonType)
{
  addonType.m_point = variant["type"].asString();

  for (CVariant::const_iterator_array value = variant["values"].begin_array(); value != variant["values"].end_array();
       ++value)
  {
    std::string id = (*value)["id"].asString();
    std::vector<std::pair<std::string, SExtValue> > extValues;
    for (CVariant::const_iterator_array content = (*value)["content"].begin_array();
         content != (*value)["content"].end_array(); ++content)
    {
      extValues.push_back(std::make_pair((*content)["key"].asString(), SExtValue((*content)["value"].asString())));
    }

    addonType.m_values.push_back(std::make_pair(id, extValues));
  }

  for (CVariant::const_iterator_array child = variant["children"].begin_array(); child != variant["children"].end_array();
       ++child)
  {
    CAddonExtensions childExt;
    DeserializeExtensions((*child)["child"], childExt);
    std::string id = (*child)["id"].asString();
    addonType.m_children.push_back(std::make_pair(id, childExt));
  }

  return;
}

CAddonDatabase::CAddonDatabase() {}

CAddonDatabase::~CAddonDatabase() {}

bool CAddonDatabase::Open()
{
  return CDatabase::Open();
}

int CAddonDatabase::GetMinSchemaVersion() const
{
  return 21;
}

int CAddonDatabase::GetSchemaVersion() const
{
  return 33;
}

void CAddonDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create addons table");
  m_pDS->exec("CREATE TABLE addons ("
      "id INTEGER PRIMARY KEY,"
      "metadata BLOB,"
      "addonID TEXT NOT NULL,"
      "version TEXT NOT NULL,"
      "name TEXT NOT NULL,"
      "summary TEXT NOT NULL,"
      "news TEXT NOT NULL,"
      "description TEXT NOT NULL)");

  CLog::Log(LOGINFO, "create repo table");
  m_pDS->exec("CREATE TABLE repo (id integer primary key, addonID text,"
              "checksum text, lastcheck text, version text, nextcheck TEXT)\n");

  CLog::Log(LOGINFO, "create addonlinkrepo table");
  m_pDS->exec("CREATE TABLE addonlinkrepo (idRepo integer, idAddon integer)\n");

  CLog::Log(LOGINFO, "create update_rules table");
  m_pDS->exec(
      "CREATE TABLE update_rules (id integer primary key, addonID TEXT, updateRule INTEGER)\n");

  CLog::Log(LOGINFO, "create package table");
  m_pDS->exec("CREATE TABLE package (id integer primary key, addonID text, filename text, hash text)\n");

  CLog::Log(LOGINFO, "create installed table");
  m_pDS->exec("CREATE TABLE installed (id INTEGER PRIMARY KEY, addonID TEXT UNIQUE, "
              "enabled BOOLEAN, installDate TEXT, lastUpdated TEXT, lastUsed TEXT, "
              "origin TEXT NOT NULL DEFAULT '', disabledReason INTEGER NOT NULL DEFAULT 0) \n");
}

void CAddonDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "{} creating indices", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxAddons ON addons(addonID)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_addonlinkrepo_1 ON addonlinkrepo ( idAddon, idRepo )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_addonlinkrepo_2 ON addonlinkrepo ( idRepo, idAddon )\n");
  m_pDS->exec("CREATE UNIQUE INDEX idxUpdate_rules ON update_rules(addonID, updateRule)");
  m_pDS->exec("CREATE UNIQUE INDEX idxPackage ON package(filename)");
}

void CAddonDatabase::UpdateTables(int version)
{
}

void CAddonDatabase::SyncInstalled(const std::set<std::string>& ids,
                                   const std::set<std::string>& system,
                                   const std::set<std::string>& optional)
{
  try
  {
    if (!m_pDB)
      return;
    if (!m_pDS)
      return;

    std::set<std::string> db;
    m_pDS->query(PrepareSQL("SELECT addonID FROM installed"));
    while (!m_pDS->eof())
    {
      db.insert(m_pDS->fv("addonID").get_asString());
      m_pDS->next();
    }
    m_pDS->close();

    std::set<std::string> added;
    std::set<std::string> removed;
    std::set_difference(ids.begin(), ids.end(), db.begin(), db.end(), std::inserter(added, added.end()));
    std::set_difference(db.begin(), db.end(), ids.begin(), ids.end(), std::inserter(removed, removed.end()));

    for (std::set<std::string>::const_iterator id = added.begin(); id != added.end(); ++id)
      CLog::Log(LOGDEBUG, "CAddonDatabase: %s has been installed.", id->c_str());

    for (std::set<std::string>::const_iterator id = removed.begin(); id != removed.end(); ++id)
      CLog::Log(LOGDEBUG, "CAddonDatabase: %s has been uninstalled.", id->c_str());

    std::string now = CDateTime::GetCurrentDateTime().GetAsDBDateTime();
    BeginTransaction();
    for (std::set<std::string>::const_iterator id = added.begin(); id != added.end(); ++id)
    {
      int enable = 0;

      if (system.find(*id) != system.end() || optional.find(*id) != optional.end())
        enable = 1;

      m_pDS->exec(PrepareSQL("INSERT INTO installed(addonID, enabled, installDate) "
        "VALUES('%s', %d, '%s')", id->c_str(), enable, now.c_str()));
    }

    for (std::set<std::string>::const_iterator id = removed.begin(); id != removed.end(); ++id)
    {
      m_pDS->exec(PrepareSQL("DELETE FROM installed WHERE addonID='%s'", id->c_str()));
      RemoveAllUpdateRulesForAddon(*id);
      DeleteRepository(*id);
    }

    for (std::set<std::string>::const_iterator id = system.begin(); id != system.end(); ++id)
    {
      m_pDS->exec(PrepareSQL("UPDATE installed SET enabled=1 WHERE addonID='%s'", id->c_str()));
      // Make sure system addons always have ORIGIN_SYSTEM origin
      m_pDS->exec(PrepareSQL("UPDATE installed SET origin='%s' WHERE addonID='%s'", ORIGIN_SYSTEM,
                              id->c_str()));
    }

    for (std::set<std::string>::const_iterator id = optional.begin(); id != optional.end(); ++id)
    {
      // Make sure optional system addons always have ORIGIN_SYSTEM origin
      m_pDS->exec(PrepareSQL("UPDATE installed SET origin='%s' WHERE addonID='%s'", ORIGIN_SYSTEM,
                              id->c_str()));
    }

    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed", __FUNCTION__);
    RollbackTransaction();
  }
}

bool CAddonDatabase::SetLastUpdated(const std::string& addonId, const CDateTime& dateTime)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    m_pDS->exec(PrepareSQL("UPDATE installed SET lastUpdated='%s' WHERE addonID='%s'",
        dateTime.GetAsDBDateTime().c_str(), addonId.c_str()));
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on addon '{}'", __FUNCTION__, addonId);
  }
  return false;
}

bool CAddonDatabase::SetOrigin(const std::string& addonId, const std::string& origin)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    m_pDS->exec(PrepareSQL("UPDATE installed SET origin='%s' WHERE addonID='%s'", origin.c_str(), addonId.c_str()));
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on addon '{}'", __FUNCTION__, addonId);
  }
  return false;
}

bool CAddonDatabase::SetLastUsed(const std::string& addonId, const CDateTime& dateTime)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    unsigned int start = XbmcThreads::SystemClockMillis();
    m_pDS->exec(PrepareSQL("UPDATE installed SET lastUsed='%s' WHERE addonID='%s'",
        dateTime.GetAsDBDateTime().c_str(), addonId.c_str()));

    CLog::Log(LOGDEBUG, "CAddonDatabase::SetLastUsed[%s] took %u ms", addonId.c_str(), XbmcThreads::SystemClockMillis() - start);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on addon '{}'", __FUNCTION__, addonId);
  }
  return false;
}

bool CAddonDatabase::FindByAddonId(const std::string& addonId, ADDON::VECADDONS& result) const
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    std::string sql = PrepareSQL(
        "SELECT addons.version, addons.name, addons.summary, addons.description, addons.metadata, addons.news,"
        "repo.addonID AS repoID FROM addons "
        "JOIN addonlinkrepo ON addonlinkrepo.idAddon=addons.id "
        "JOIN repo ON repo.id=addonlinkrepo.idRepo "
        "WHERE "
        "repo.checksum IS NOT NULL AND repo.checksum != '' "
        "AND EXISTS (SELECT * FROM installed WHERE installed.addonID=repoID AND installed.enabled=1) "
        "AND addons.addonID='%s'", addonId.c_str());

    m_pDS->query(sql);

    VECADDONS addons;
    addons.reserve(m_pDS->num_rows());

    while (!m_pDS->eof())
    {
      CAddonInfoBuilderFromDB builder;
      builder.SetId(addonId);
      builder.SetVersion(CAddonVersion(m_pDS->fv("version").get_asString()));
      builder.SetName(m_pDS->fv("name").get_asString());
      builder.SetSummary(m_pDS->fv("summary").get_asString());
      builder.SetDescription(m_pDS->fv("description").get_asString());
      CAddonDatabaseSerializer::DeserializeMetadata(m_pDS->fv("metadata").get_asString(), builder);
      builder.SetChangelog(m_pDS->fv("news").get_asString());
      builder.SetOrigin(m_pDS->fv("repoID").get_asString());

      ADDON::AddonPtr addon = CAddonBuilder::Generate(builder.get(), AddonType::UNKNOWN);
      if (addon)
        addons.push_back(boost::move(addon));
      else
        CLog::Log(LOGERROR, "CAddonDatabase: failed to build {}", addonId);
      m_pDS->next();
    }
    m_pDS->close();
    result = boost::move(addons);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on addon {}", __FUNCTION__, addonId);
  }
  return false;
}

bool CAddonDatabase::GetAddon(const std::string& addonID,
                              const CAddonVersion& version,
                              const std::string& repoId,
                              AddonPtr& addon)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    const std::string sql =
        PrepareSQL("SELECT addons.id FROM addons "
                   "JOIN addonlinkrepo ON addonlinkrepo.idAddon=addons.id "
                   "JOIN repo ON repo.id=addonlinkrepo.idRepo "
                   "WHERE addons.addonID='%s' AND addons.version='%s' AND repo.addonID='%s'",
                   addonID.c_str(), version.asString().c_str(), repoId.c_str());

    m_pDS->query(sql);
    if (m_pDS->eof())
      return false;

    return GetAddon(m_pDS->fv("id").get_asInt(), addon);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on addon {}", __FUNCTION__, addonID);
  }
  return false;

}

bool CAddonDatabase::GetAddon(int id, AddonPtr &addon)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS2)
      return false;

    m_pDS2->query(PrepareSQL("SELECT addons.*, repo.addonID as origin FROM addons "
                             "JOIN addonlinkrepo ON addonlinkrepo.idAddon=addons.id "
                             "JOIN repo ON repo.id=addonlinkrepo.idRepo "
                             "WHERE addons.id=%i",
                             id));
    if (m_pDS2->eof())
      return false;

    CAddonInfoBuilderFromDB builder;
    builder.SetId(m_pDS2->fv("addonID").get_asString());
    builder.SetOrigin(m_pDS2->fv("origin").get_asString());
    builder.SetVersion(CAddonVersion(m_pDS2->fv("version").get_asString()));
    builder.SetName(m_pDS2->fv("name").get_asString());
    builder.SetSummary(m_pDS2->fv("summary").get_asString());
    builder.SetDescription(m_pDS2->fv("description").get_asString());
    CAddonDatabaseSerializer::DeserializeMetadata(m_pDS2->fv("metadata").get_asString(), builder);

    addon = CAddonBuilder::Generate(builder.get(), AddonType::UNKNOWN);
    return addon != NULL;

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on addon {}", __FUNCTION__, id);
  }
  return false;
}

bool CAddonDatabase::GetRepositoryContent(VECADDONS& addons) const
{
  return GetRepositoryContent("", addons);
}

bool CAddonDatabase::GetRepositoryContent(const std::string& id, VECADDONS& addons) const
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    unsigned int start = XbmcThreads::SystemClockMillis();

    // Ensure that the repositories we fetch from are enabled and valid.
    std::vector<std::string> repoIds;
    {
      std::string sql = PrepareSQL(
          " SELECT repo.id FROM repo"
          " WHERE repo.checksum IS NOT NULL AND repo.checksum != ''"
          " AND EXISTS (SELECT * FROM installed WHERE installed.addonID=repo.addonID AND"
          " installed.enabled=1)");

      if (!id.empty())
        sql += PrepareSQL(" AND repo.addonId='%s'", id.c_str());

      m_pDS->query(sql);
      while (!m_pDS->eof())
      {
        repoIds.push_back(m_pDS->fv("id").get_asString());
        m_pDS->next();
      }
    }

    CLog::Log(LOGDEBUG, "CAddonDatabase: SELECT repo.id FROM repo .. took %u ms", XbmcThreads::SystemClockMillis() - start);

    if (repoIds.empty())
    {
      if (id.empty())
      {
        CLog::Log(LOGDEBUG, "CAddonDatabase: no valid repository, continuing");
        addons.clear();
        return true;
      }

      CLog::Log(LOGDEBUG, "CAddonDatabase: no valid repository matching '{}'", id);
      return false;
    }

    {
      std::string sql = PrepareSQL(" SELECT addons.*, repo.addonID AS repoID FROM addons"
                                   " JOIN addonlinkrepo ON addons.id=addonlinkrepo.idAddon"
                                   " JOIN repo ON repo.id=addonlinkrepo.idRepo"
                                   " WHERE addonlinkrepo.idRepo IN (%s)"
                                   " ORDER BY repo.addonID, addons.addonID",
                                   StringUtils::Join(repoIds, ",").c_str());

      start = XbmcThreads::SystemClockMillis();
      m_pDS->query(sql);

      CLog::Log(LOGDEBUG, "CAddonDatabase: query %s returned %i rows in %u ms", sql.c_str(),
                m_pDS->num_rows(), XbmcThreads::SystemClockMillis() - start);
    }

    VECADDONS result;
    result.reserve(m_pDS->num_rows());

    while (!m_pDS->eof())
    {
      std::string addonId = m_pDS->fv("addonID").get_asString();
      CAddonVersion version(m_pDS->fv("version").get_asString());

      CAddonInfoBuilderFromDB builder;
      builder.SetId(addonId);
      builder.SetVersion(version);
      builder.SetName(m_pDS->fv("name").get_asString());
      builder.SetSummary(m_pDS->fv("summary").get_asString());
      builder.SetDescription(m_pDS->fv("description").get_asString());
      builder.SetOrigin(m_pDS->fv("repoID").get_asString());
      CAddonDatabaseSerializer::DeserializeMetadata(m_pDS->fv("metadata").get_asString(), builder);

      ADDON::AddonPtr addon = CAddonBuilder::Generate(builder.get(), AddonType::UNKNOWN);
      if (addon)
      {
        result.push_back(boost::move(addon));
      }
      else
        CLog::Log(LOGWARNING, "CAddonDatabase: failed to build {}", addonId);
      m_pDS->next();
    }
    m_pDS->close();
    addons = boost::move(result);

    CLog::Log(LOGDEBUG, "CAddonDatabase::GetAddons took %u ms", XbmcThreads::SystemClockMillis() - start);

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed", __FUNCTION__);
  }
  return false;
}

void CAddonDatabase::DeleteRepository(const std::string& id)
{
  try
  {
    if (!m_pDB)
      return;
    if (!m_pDS)
      return;

    int idRepo = GetRepositoryId(id);
    if (idRepo < 0)
      return;

    m_pDS->exec(PrepareSQL("DELETE FROM repo WHERE id=%i", idRepo));
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on repo '{}'", __FUNCTION__, id);
  }
}

void CAddonDatabase::DeleteRepositoryContents(const std::string& id)
{
  try
  {
    if (!m_pDB)
      return;
    if (!m_pDS)
      return;

    int idRepo = GetRepositoryId(id);
    if (idRepo < 0)
      return;

    m_pDS->exec(PrepareSQL("DELETE FROM addons WHERE id IN (SELECT idAddon FROM addonlinkrepo WHERE idRepo=%i)", idRepo));
    m_pDS->exec(PrepareSQL("DELETE FROM addonlinkrepo WHERE idRepo=%i", idRepo));
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on repo '{}'", __FUNCTION__, id);
  }
}

int CAddonDatabase::GetRepositoryId(const std::string& addonId)
{
  if (!m_pDB)
    return -1;
  if (!m_pDS)
    return -1;

  m_pDS->query(PrepareSQL("SELECT id FROM repo WHERE addonID='%s'", addonId.c_str()));
  if (m_pDS->eof())
    return -1;

  return m_pDS->fv("id").get_asInt();
}

bool CAddonDatabase::UpdateRepositoryContent(const std::string& repository,
                                             const CAddonVersion& version,
                                             const std::string& checksum,
                                             const std::vector<AddonInfoPtr>& addons)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    DeleteRepositoryContents(repository);
    int idRepo = GetRepositoryId(repository);
    if (idRepo < 0)
      return false;

    assert(idRepo > 0);

    m_pDB->start_transaction();
    m_pDS->exec(
        PrepareSQL("UPDATE repo SET checksum='%s' WHERE id='%i'", checksum.c_str(), idRepo));
    for (std::vector<AddonInfoPtr>::const_iterator addon = addons.begin(); addon != addons.end(); ++addon)
    {
      m_pDS->exec(PrepareSQL(
          "INSERT INTO addons (id, metadata, addonID, version, name, summary, description, news) "
          "VALUES (NULL, '%s', '%s', '%s', '%s','%s', '%s','%s')",
          CAddonDatabaseSerializer::SerializeMetadata(*(*addon)).c_str(), (*addon)->ID().c_str(),
          (*addon)->Version().asString().c_str(), (*addon)->Name().c_str(), (*addon)->Summary().c_str(),
          (*addon)->Description().c_str(), (*addon)->ChangeLog().c_str()));

      int idAddon = static_cast<int>(m_pDS->lastinsertid());
      if (idAddon <= 0)
      {
        CLog::Log(LOGERROR, "%s insert failed on addon '%s'", __FUNCTION__, (*addon)->ID().c_str());
        RollbackTransaction();
        return false;
      }

      m_pDS->exec(PrepareSQL("INSERT INTO addonlinkrepo (idRepo, idAddon) VALUES (%i, %i)", idRepo, idAddon));
    }

    m_pDB->commit_transaction();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on repo '{}'", __FUNCTION__, repository);
    RollbackTransaction();
  }
  return false;
}

int CAddonDatabase::GetRepoChecksum(const std::string& id, std::string& checksum)
{
  try
  {
    if (!m_pDB)
      return -1;
    if (!m_pDS)
      return -1;

    std::string strSQL = PrepareSQL("select * from repo where addonID='%s'",id.c_str());
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
    {
      checksum = m_pDS->fv("checksum").get_asString();
      return m_pDS->fv("id").get_asInt();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on repo '{}'", __FUNCTION__, id);
  }
  checksum.clear();
  return -1;
}

CAddonDatabase::RepoUpdateData CAddonDatabase::GetRepoUpdateData(const std::string& id)
{
  RepoUpdateData result;
  try
  {
    if (m_pDB && m_pDS)
    {
      std::string strSQL = PrepareSQL("select * from repo where addonID='%s'",id.c_str());
      m_pDS->query(strSQL);
      if (!m_pDS->eof())
      {
        result.lastCheckedAt.SetFromDBDateTime(m_pDS->fv("lastcheck").get_asString());
        result.lastCheckedVersion = CAddonVersion(m_pDS->fv("version").get_asString());
        result.nextCheckAt.SetFromDBDateTime(m_pDS->fv("nextcheck").get_asString());
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on repo '{}'", __FUNCTION__, id);
  }
  return result;
}

int CAddonDatabase::SetRepoUpdateData(const std::string& id, const RepoUpdateData& updateData)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    int retId = -1;
    std::string sql = PrepareSQL("SELECT * FROM repo WHERE addonID='%s'", id.c_str());
    m_pDS->query(sql);

    if (m_pDS->eof())
    {
      sql = PrepareSQL("INSERT INTO repo (id, addonID, lastcheck, version, nextcheck) "
                       "VALUES (NULL, '%s', '%s', '%s', '%s')",
                       id.c_str(), updateData.lastCheckedAt.GetAsDBDateTime().c_str(),
                       updateData.lastCheckedVersion.asString().c_str(),
                       updateData.nextCheckAt.GetAsDBDateTime().c_str());
      m_pDS->exec(sql);
      retId = static_cast<int>(m_pDS->lastinsertid());
    }
    else
    {
      retId = m_pDS->fv("id").get_asInt();
      sql = PrepareSQL(
          "UPDATE repo SET lastcheck='%s', version='%s', nextcheck='%s' WHERE addonID='%s'",
          updateData.lastCheckedAt.GetAsDBDateTime().c_str(),
          updateData.lastCheckedVersion.asString().c_str(),
          updateData.nextCheckAt.GetAsDBDateTime().c_str(), id.c_str());
      m_pDS->exec(sql);
    }

    return retId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on repo '{}'", __FUNCTION__, id);
  }
  return -1;
}

bool CAddonDatabase::Search(const std::string& search, VECADDONS& addons)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    std::string strSQL;
    strSQL = PrepareSQL("SELECT id FROM addons WHERE name LIKE '%%%s%%' OR summary LIKE '%%%s%%' "
                  "OR description LIKE '%%%s%%'", search.c_str(), search.c_str(), search.c_str());

    CLog::Log(LOGDEBUG, "{} query: {}", __FUNCTION__, strSQL);

    if (!m_pDS->query(strSQL)) return false;
    if (m_pDS->num_rows() == 0) return false;

    while (!m_pDS->eof())
    {
      AddonPtr addon;
      GetAddon(m_pDS->fv("id").get_asInt(), addon);
      if (static_cast<int>(addon->Type()) >= static_cast<int>(AddonType::UNKNOWN) + 1 &&
          static_cast<int>(addon->Type()) < static_cast<int>(AddonType::SCRAPER_LIBRARY))
        addons.push_back(addon);
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed", __FUNCTION__);
  }
  return false;
}

bool CAddonDatabase::DisableAddon(const std::string& addonID, AddonDisabledReason::Type disabledReason)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    const std::string sql =
        PrepareSQL("UPDATE installed SET enabled=0, disabledReason=%d WHERE addonID='%s'",
                   static_cast<int>(disabledReason), addonID.c_str());
    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on addon '{}'", __FUNCTION__, addonID);
  }
  return false;
}

bool CAddonDatabase::EnableAddon(const std::string& addonID)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    const std::string sql = PrepareSQL(
        "UPDATE installed SET enabled=1, disabledReason=0 WHERE addonID='%s'", addonID.c_str());
    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on addon '{}'", __FUNCTION__, addonID);
  }
  return false;
}

bool CAddonDatabase::GetDisabled(std::map<std::string, AddonDisabledReason::Type>& addons)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    const std::string sql =
        PrepareSQL("SELECT addonID, disabledReason FROM installed WHERE enabled=0");
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      addons.insert(std::make_pair(m_pDS->fv("addonID").get_asString(),
                     static_cast<AddonDisabledReason::Type>(m_pDS->fv("disabledReason").get_asInt())));
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed", __FUNCTION__);
  }
  return false;
}

bool CAddonDatabase::GetAddonUpdateRules(
    std::map<std::string, std::vector<AddonUpdateRule::Type> >& rulesMap) const
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    std::string sql = PrepareSQL("SELECT * FROM update_rules");
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      rulesMap[m_pDS->fv("addonID").get_asString()].push_back(
          static_cast<AddonUpdateRule::Type>(m_pDS->fv("updateRule").get_asInt()));
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed", __FUNCTION__);
  }
  return false;
}

bool CAddonDatabase::AddUpdateRuleForAddon(const std::string& addonID, AddonUpdateRule::Type updateRule)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    std::string sql =
        PrepareSQL("INSERT INTO update_rules(id, addonID, updateRule) VALUES(NULL, '%s', %d)",
                   addonID.c_str(), static_cast<int>(updateRule));
    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on addon '{}'", __FUNCTION__, addonID);
  }
  return false;
}

bool CAddonDatabase::RemoveAllUpdateRulesForAddon(const std::string& addonID)
{
  return RemoveUpdateRuleForAddon(addonID, AddonUpdateRule::ANY);
}

bool CAddonDatabase::RemoveUpdateRuleForAddon(const std::string& addonID,
                                              AddonUpdateRule::Type updateRule)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    std::string sql = PrepareSQL("DELETE FROM update_rules WHERE addonID='%s'", addonID.c_str());

    if (updateRule != AddonUpdateRule::ANY)
    {
      sql += PrepareSQL(" AND updateRule = %d", static_cast<int>(updateRule));
    }

    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on addon '{}'", __FUNCTION__, addonID);
  }
  return false;
}

bool CAddonDatabase::AddPackage(const std::string& addonID,
                                const std::string& packageFileName,
                                const std::string& hash)
{
  std::string sql = PrepareSQL("insert or ignore into package(id, addonID, filename, hash)"
                              "values(NULL, '%s', '%s', '%s')",
                              addonID.c_str(), packageFileName.c_str(), hash.c_str());
  return ExecuteQuery(sql);
}

bool CAddonDatabase::GetPackageHash(const std::string& addonID,
                                    const std::string& packageFileName,
                                    std::string&       hash)
{
  std::string where = PrepareSQL("addonID='%s' and filename='%s'",
                                addonID.c_str(), packageFileName.c_str());
  hash = GetSingleValue("package", "hash", where);
  return !hash.empty();
}

bool CAddonDatabase::RemovePackage(const std::string& packageFileName)
{
  std::string sql = PrepareSQL("delete from package where filename='%s'", packageFileName.c_str());
  return ExecuteQuery(sql);
}

void CAddonDatabase::OnPostUnInstall(const std::string& addonId)
{
  RemoveAllUpdateRulesForAddon(addonId);
  DeleteRepository(addonId);

  //! @todo should be done before uninstall to avoid any race conditions
  try
  {
    if (!m_pDB)
      return;
    if (!m_pDS)
      return;
    m_pDS->exec(PrepareSQL("DELETE FROM installed WHERE addonID='%s'", addonId.c_str()));
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on addon {}", __FUNCTION__, addonId);
  }
}

void CAddonDatabase::GetInstallData(const AddonInfoPtr& addon)
{
  try
  {
    if (!m_pDB)
      return;
    if (!m_pDS)
      return;

    m_pDS->query(PrepareSQL("SELECT addonID, installDate, lastUpdated, lastUsed, "
                            "origin FROM installed WHERE addonID='%s'",
                            addon->ID().c_str()));
    if (!m_pDS->eof())
    {
      CAddonInfoBuilder::SetInstallData(
          addon, CDateTime::FromDBDateTime(m_pDS->fv("installDate").get_asString()),
          CDateTime::FromDBDateTime(m_pDS->fv("lastUpdated").get_asString()),
          CDateTime::FromDBDateTime(m_pDS->fv("lastUsed").get_asString()),
          m_pDS->fv("origin").get_asString());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CAddonDatabase::{}: failed", __FUNCTION__);
  }
}

bool CAddonDatabase::AddInstalledAddon(const boost::shared_ptr<CAddonInfo>& addon,
                                       const std::string& origin)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    m_pDS->query(PrepareSQL("SELECT * FROM installed WHERE addonID='%s'", addon->ID().c_str()));

    if (m_pDS->eof())
    {
      std::string now = CDateTime::GetCurrentDateTime().GetAsDBDateTime();

      m_pDS->exec(PrepareSQL("INSERT INTO installed(addonID, enabled, installDate, origin) "
                             "VALUES('%s', 1, '%s', '%s')",
                             addon->ID().c_str(), now.c_str(), origin.c_str()));
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed", __FUNCTION__);
    return false;
  }

  return true;
}
