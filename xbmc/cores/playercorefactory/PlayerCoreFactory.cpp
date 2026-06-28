/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerCoreFactory.h"
#include "FileItem.h"
#include "PlayerCoreConfig.h"
#include "PlayerSelectionRule.h"
#include "URL.h"
#include "cores/IPlayerCallback.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfileManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingsManager.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <sstream>

#define PLAYERCOREFACTORY_XML "playercorefactory.xml"

CPlayerCoreFactory::CPlayerCoreFactory(const CProfileManager& profileManager)
  : m_settings(CServiceBroker::GetSettingsComponent()->GetSettings()),
    m_profileManager(profileManager)
{
  if (m_settings->IsLoaded())
    OnSettingsLoaded();

  m_settings->GetSettingsManager()->RegisterSettingsHandler(this);
}

CPlayerCoreFactory::~CPlayerCoreFactory()
{
  m_settings->GetSettingsManager()->UnregisterSettingsHandler(this);
}

void CPlayerCoreFactory::OnSettingsLoaded()
{
  LoadConfiguration("special://xbmc/system/" PLAYERCOREFACTORY_XML, true);
  LoadConfiguration(m_profileManager.GetUserDataItem(PLAYERCOREFACTORY_XML), false);
}

boost::shared_ptr<IPlayer> CPlayerCoreFactory::CreatePlayer(const std::string& nameId,
                                                          IPlayerCallback& callback) const
{
  CSingleLock lock(m_section);
  size_t idx = GetPlayerIndex(nameId);

  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return boost::shared_ptr<IPlayer>();

  return m_vecPlayerConfigs[idx]->CreatePlayer(callback);
}

void CPlayerCoreFactory::GetPlayers(std::vector<std::string>&players) const
{
  CSingleLock lock(m_section);
  players.clear();
  for (std::vector<boost::shared_ptr<CPlayerCoreConfig> >::const_iterator conf = m_vecPlayerConfigs.begin(); conf != m_vecPlayerConfigs.end(); ++conf)
  {
    if ((*conf)->m_bPlaysAudio || (*conf)->m_bPlaysVideo)
      players.push_back((*conf)->m_name);
  }
}

void CPlayerCoreFactory::GetPlayers(std::vector<std::string>&players, const bool audio, const bool video) const
{
  CSingleLock lock(m_section);
  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: for video=%i, audio=%i", video, audio);

  for (std::vector<boost::shared_ptr<CPlayerCoreConfig> >::const_iterator conf = m_vecPlayerConfigs.begin(); conf != m_vecPlayerConfigs.end(); ++conf)
  {
    if (audio == (*conf)->m_bPlaysAudio && video == (*conf)->m_bPlaysVideo)
    {
      if (std::find(players.begin(), players.end(), (*conf)->m_name) != players.end())
        continue;

      CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding player: %s", (*conf)->m_name.c_str());
      players.push_back((*conf)->m_name);
    }
  }
}

void CPlayerCoreFactory::GetPlayers(const CFileItem& item, std::vector<std::string>&players) const
{
  CURL url(item.GetDynPath());

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers(%s)", CURL::GetRedacted(item.GetDynPath()).c_str());

  std::vector<std::string>validPlayers;
  GetPlayers(validPlayers);

  // Process rules
  for (std::list<boost::shared_ptr<CPlayerSelectionRule> >::const_iterator rule = m_vecCoreSelectionRules.begin(); rule != m_vecCoreSelectionRules.end(); ++rule)
    (*rule)->GetPlayers(item, validPlayers, players);

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: matched %" PRIuS "rules with players", players.size());

  // Process defaults

  // Set video default player. Check whether it's video first (overrule audio and
  // game check). Also push these players in case it is NOT audio or game either.
  //
  // If an inputstream add-on is used, first check if we have an override to use
  // "videodefaultplayer"
  if (item.IsVideo() || !item.IsAudio())
  {
    int idx = GetPlayerIndex("videodefaultplayer");
    if (idx > -1)
    {
      const std::string videoDefault = GetPlayerName(idx);
      if (std::find(players.begin(), players.end(), videoDefault) == players.end())
      {
        players.push_back(videoDefault);
        CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding videodefaultplayer (%s)",
                  videoDefault.c_str());
      }
    }
    GetPlayers(players, false, true);  // Video-only players
    GetPlayers(players, true, true);   // Audio & video players
  }

  // Set audio default player
  // Pushback all audio players in case we don't know the type
  if (item.IsAudio())
  {
    int idx = GetPlayerIndex("audiodefaultplayer");
    if (idx > -1)
    {
      const std::string audioDefault = GetPlayerName(idx);
      if (std::find(players.begin(), players.end(), audioDefault) == players.end())
      {
        players.push_back(audioDefault);
        CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: adding audiodefaultplayer (%s)",
                  audioDefault.c_str());
      }
    }
    GetPlayers(players, true, false); // Audio-only players
    GetPlayers(players, true, true);  // Audio & video players
  }

  CLog::Log(LOGDEBUG, "CPlayerCoreFactory::GetPlayers: added %" PRIuS "players", players.size());
}

int CPlayerCoreFactory::GetPlayerIndex(const std::string& strCoreName) const
{
  CSingleLock lock(m_section);
  if (!strCoreName.empty())
  {
    // Dereference "*default*player" aliases
    std::string strRealCoreName;
    if (StringUtils::EqualsNoCase(strCoreName, "audiodefaultplayer"))
      strRealCoreName = CServiceBroker::GetSettingsComponent()->GetSettings()->GetDefaultAudioPlayerName();
    else if (StringUtils::EqualsNoCase(strCoreName, "videodefaultplayer"))
      strRealCoreName = CServiceBroker::GetSettingsComponent()->GetSettings()->GetDefaultVideoPlayerName();
    else
      strRealCoreName = strCoreName;

    for(size_t i = 0; i < m_vecPlayerConfigs.size(); i++)
    {
      if (StringUtils::EqualsNoCase(m_vecPlayerConfigs[i]->GetName(), strRealCoreName))
        return i;
    }
    CLog::Log(LOGWARNING, "CPlayerCoreFactory::GetPlayer(%s): no such player: %s", strCoreName.c_str(),
              strRealCoreName.c_str());
  }
  return -1;
}

std::string CPlayerCoreFactory::GetPlayerName(size_t idx) const
{
  CSingleLock lock(m_section);
  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return "";

  return m_vecPlayerConfigs[idx]->m_name;
}

void CPlayerCoreFactory::GetPlayers(std::vector<std::string>&players, std::string &type) const
{
  CSingleLock lock(m_section);
  for (std::vector<boost::shared_ptr<CPlayerCoreConfig> >::const_iterator config = m_vecPlayerConfigs.begin(); config != m_vecPlayerConfigs.end(); ++config)
  {
    if ((*config)->m_type != type)
      continue;
    players.push_back((*config)->m_name);
  }
}

std::string CPlayerCoreFactory::GetPlayerType(const std::string& player) const
{
  CSingleLock lock(m_section);
  size_t idx = GetPlayerIndex(player);

  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return "";

  return m_vecPlayerConfigs[idx]->m_type;
}

bool CPlayerCoreFactory::PlaysAudio(const std::string& player) const
{
  CSingleLock lock(m_section);
  size_t idx = GetPlayerIndex(player);

  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return false;

  return m_vecPlayerConfigs[idx]->m_bPlaysAudio;
}

bool CPlayerCoreFactory::PlaysVideo(const std::string& player) const
{
  CSingleLock lock(m_section);
  size_t idx = GetPlayerIndex(player);

  if (m_vecPlayerConfigs.empty() || idx > m_vecPlayerConfigs.size())
    return false;

  return m_vecPlayerConfigs[idx]->m_bPlaysVideo;
}

std::string CPlayerCoreFactory::GetDefaultPlayer(const CFileItem& item) const
{
  std::vector<std::string>players;
  GetPlayers(item, players);

  //If we have any players return the first one
  if (!players.empty())
    return players.at(0);

  return "";
}

std::string CPlayerCoreFactory::SelectPlayerDialog(const std::vector<std::string>&players, float posX, float posY) const
{
  CContextButtons choices;
  if (players.size())
  {
    //Add default player
    std::string strCaption = players[0];
    strCaption += " (";
    strCaption += g_localizeStrings.Get(13278);
    strCaption += ")";
    choices.Add(0, strCaption);

    //Add all other players
    for (unsigned int i = 1; i < players.size(); i++)
      choices.Add(i, players[i]);

    int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if (choice >= 0)
      return players[choice];
  }
  return "";
}

std::string CPlayerCoreFactory::SelectPlayerDialog(float posX, float posY) const
{
  std::vector<std::string>players;
  GetPlayers(players);
  return SelectPlayerDialog(players, posX, posY);
}

bool CPlayerCoreFactory::LoadConfiguration(const std::string &file, bool clear)
{
  CSingleLock lock(m_section);

  CLog::Log(LOGINFO, "Loading player core factory settings from %s.", file.c_str());
  if (!CFileUtils::Exists(file))
  { // tell the user it doesn't exist
    CLog::Log(LOGINFO, "%s does not exist. Skipping.", file.c_str());
    return false;
  }

  CXBMCTinyXML playerCoreFactoryXML;
  if (!playerCoreFactoryXML.LoadFile(file))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %i (%s)", file.c_str(), playerCoreFactoryXML.ErrorRow(),
              playerCoreFactoryXML.ErrorDesc());
    return false;
  }

  TiXmlElement *pConfig = playerCoreFactoryXML.RootElement();
  if (pConfig == NULL)
  {
    CLog::Log(LOGERROR, "Error loading %s, Bad structure", file.c_str());
    return false;
  }

  if (clear)
  {
    m_vecPlayerConfigs.clear();
    m_vecCoreSelectionRules.clear();

    // Builtin players
    boost::shared_ptr<CPlayerCoreConfig> VideoPlayer = boost::make_shared<CPlayerCoreConfig>("VideoPlayer", "video", static_cast<const TiXmlElement*>(NULL));
    VideoPlayer->m_bPlaysAudio = true;
    VideoPlayer->m_bPlaysVideo = true;
    m_vecPlayerConfigs.push_back(VideoPlayer);

    boost::shared_ptr<CPlayerCoreConfig> mplayer = boost::make_shared<CPlayerCoreConfig>("MPlayer", "video", static_cast<const TiXmlElement*>(NULL));
    VideoPlayer->m_bPlaysAudio = true;
    VideoPlayer->m_bPlaysVideo = true;
    m_vecPlayerConfigs.push_back(mplayer);

    boost::shared_ptr<CPlayerCoreConfig> paplayer = boost::make_shared<CPlayerCoreConfig>("PAPlayer", "music", static_cast<const TiXmlElement*>(NULL));
    paplayer->m_bPlaysAudio = true;
    m_vecPlayerConfigs.push_back(paplayer);
  }

  if (StringUtils::CompareNoCase(pConfig->Value(), "playercorefactory") != 0)
  {
    CLog::Log(LOGERROR, "Error loading configuration, no <playercorefactory> node");
    return false;
  }

  TiXmlElement *pPlayers = pConfig->FirstChildElement("players");
  if (pPlayers)
  {
    TiXmlElement* pPlayer = pPlayers->FirstChildElement("player");
    while (pPlayer)
    {
      std::string name = XMLUtils::GetAttribute(pPlayer, "name");
      std::string type = XMLUtils::GetAttribute(pPlayer, "type");
      if (type.empty()) type = name;
      StringUtils::ToLower(type);

      std::string internaltype;
      if (type == "videoplayer" || type == "mplayer")
        internaltype = "video";
      else if (type == "paplayer")
        internaltype = "music";

      int count = 0;
      std::string playername = name;
      while (GetPlayerIndex(playername) >= 0)
      {
        count++;
        std::stringstream itoa;
        itoa << count;
        playername = name + itoa.str();
      }

      if (!internaltype.empty())
      {
        m_vecPlayerConfigs.push_back(
            boost::make_shared<CPlayerCoreConfig>(playername, internaltype, pPlayer));
      }

      pPlayer = pPlayer->NextSiblingElement("player");
    }
  }

  TiXmlElement *pRule = pConfig->FirstChildElement("rules");
  while (pRule)
  {
    const char* szAction = pRule->Attribute("action");
    if (szAction)
    {
      if (StringUtils::CompareNoCase(szAction, "append") == 0)
      {
        m_vecCoreSelectionRules.push_back(boost::make_shared<CPlayerSelectionRule>(pRule));
      }
      else if (StringUtils::CompareNoCase(szAction, "prepend") == 0)
      {
        m_vecCoreSelectionRules.insert(m_vecCoreSelectionRules.begin(), 1, boost::make_shared<CPlayerSelectionRule>(pRule));
      }
      else
      {
        m_vecCoreSelectionRules.clear();
        m_vecCoreSelectionRules.push_back(boost::make_shared<CPlayerSelectionRule>(pRule));
      }
    }
    else
    {
      m_vecCoreSelectionRules.push_back(boost::make_shared<CPlayerSelectionRule>(pRule));
    }

    pRule = pRule->NextSiblingElement("rules");
  }

  // succeeded - tell the user it worked
  CLog::Log(LOGINFO, "Loaded playercorefactory configuration");

  return true;
}
