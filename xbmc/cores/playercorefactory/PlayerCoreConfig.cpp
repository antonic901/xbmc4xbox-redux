/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerCoreConfig.h"

#include "cores/IPlayer.h"
#include "cores/dvdplayer/DVDPlayer.h"
#include "cores/mplayer/MPlayer.h"
#include "cores/paplayer/PAPlayer.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

CPlayerCoreConfig::CPlayerCoreConfig(std::string name,
                                     std::string type,
                                     const TiXmlElement* pConfig,
                                     const std::string& id /* = "" */)
  : m_name(name), m_id(id), m_type(type), m_bPlaysAudio(false), m_bPlaysVideo(false)
{
  if (pConfig)
  {
    m_config.reset(static_cast<TiXmlElement*>(pConfig->Clone()));
    const char* sAudio = pConfig->Attribute("audio");
    const char* sVideo = pConfig->Attribute("video");
    m_bPlaysAudio = sAudio && StringUtils::CompareNoCase(sAudio, "true") == 0;
    m_bPlaysVideo = sVideo && StringUtils::CompareNoCase(sVideo, "true") == 0;
  }

  CLog::Log(LOGDEBUG, "CPlayerCoreConfig::<ctor>: created player %s", m_name.c_str());
}

boost::shared_ptr<IPlayer> CPlayerCoreConfig::CreatePlayer(IPlayerCallback& callback) const
{
  boost::shared_ptr<IPlayer> player;

  if (m_type.compare("video") == 0)
  {
    if (m_name == "MPlayer")
      player.reset(new CMPlayer(callback));
    else
      player.reset(new CDVDPlayer(callback));
  }
  else if (m_type.compare("music") == 0)
  {
    player.reset(new PAPlayer(callback));
  }
  else
    return boost::shared_ptr<IPlayer>();

  player->m_name = m_name;
  player->m_type = m_type;

  if (player->Initialize(m_config.get()))
    return player;

  return boost::shared_ptr<IPlayer>();
}
