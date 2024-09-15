#pragma once

/*
 *      Copyright (C) 2023-2024 Nikola Antonic
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

#include "InfoLoader.h"
#include "utils/GlobalsHandling.h"

#include <vector>
#include <string>

#define INSIGNIA_LABEL_GAMES_SUPPORTED  10
#define INSIGNIA_LABEL_REGISTERED_USERS 11
#define INSIGNIA_LABEL_ONLINE_USERS     12

struct game_info
{
  std::string code;
  std::string name;
  std::string serial;
  std::string url;
  std::string thumbnail;
  int last_active_sessions;
  int last_active_users;
  int active_users;
  int active_sessions;
  int online_users;
  bool has_live_aware_feature;
  bool has_matchmaking_feature;
  bool has_leaderboards_feature;
  bool has_user_generated_content_feature;
};

class CInsigniaInfo
{
public:
  std::vector<game_info> m_games;

  void Reset()
  {
    m_games.clear();
    lastUpdateTime.clear();
    games_supported.clear();
    registered_users.clear();
    users_online_now.clear();
  };

  std::string lastUpdateTime;
  std::string games_supported;
  std::string registered_users;
  std::string users_online_now;
};

class CInsigniaJob : public CJob
{
public:
  CInsigniaJob();

  virtual bool DoWork();

  const CInsigniaInfo &GetInfo() const;

private:
  void SetWindowProperties();

  CInsigniaInfo m_info;
};

class CInsignia : public CInfoLoader
{
public:
  CInsignia(void);
  virtual ~CInsignia(void);

  const std::string &GetLastUpdateTime() const { return m_info.lastUpdateTime; };
  bool IsFetched();
  void Reset();

protected:
  virtual CJob *GetJob() const;
  virtual std::string TranslateInfo(int info) const;
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

private:
  CInsigniaInfo m_info;
};

XBMC_GLOBAL_REF(CInsignia, g_insigniaManager);
#define g_insigniaManager XBMC_GLOBAL_USE(CInsignia)
