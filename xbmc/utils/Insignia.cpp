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

#include "Insignia.h"

#include "Application.h"
#include "xbox/Network.h"
#include "filesystem/CurlFile.h"
#include "guilib/GUIBaseContainer.h"
#include "guilib/GUIStaticItem.h"
#include "guilib/GUIListItem.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "listproviders/StaticProvider.h"
#include "windows/GUIWindowInsignia.h"
#include "utils/StringUtils.h"
#include "utils/JSONVariantParser.h"
#include "utils/log.h"

#define INSIGNIA_API "https://insignia-notify-job-app.fly.dev/api/"

CInsigniaJob::CInsigniaJob()
{
}

bool MakeRequest(const std::string& strURL, std::string& strResponse)
{
  XFILE::CCurlFile httpUtil;
  bool result = httpUtil.Get(strURL, strResponse);
  httpUtil.Close();
  if (!result)
    CLog::Log(LOGWARNING, "Querying insignia data from %s failed", strURL.c_str());
  return result;
}

bool CInsigniaJob::DoWork()
{
  // wait for the network
  if (!g_application.getNetwork().IsAvailable())
    return false;

  std::string strResponse;
  std::string strURL = StringUtils::Format("%sgames", INSIGNIA_API);
  if (!MakeRequest(strURL, strResponse))
    return false;

  CVariant body = CJSONVariantParser::Parse((const unsigned char *)strResponse.c_str(), strResponse.size());
  if (body.isMember("games") && body["games"].isArray())
  {
    const CVariant &value = body["games"];
    for (CVariant::const_iterator_array it = value.begin_array(); it != value.end_array(); it++)
    {
      const CVariant &val = *it;
      game_info game;
      if (val.isMember("code") && val["code"].isString())
        game.code = val["code"].asString();

      if (val.isMember("name") && val["name"].isString())
        game.name = val["name"].asString();

      if (val.isMember("serial") && val["serial"].isString())
        game.serial = val["serial"].asString();

      if (val.isMember("url") && val["url"].isString())
        game.url = val["url"].asString();

      if (val.isMember("last_active_sessions") && val["last_active_sessions"].isInteger())
        game.last_active_sessions = val["last_active_sessions"].asInteger();

      if (val.isMember("last_active_users") && val["last_active_users"].isInteger())
        game.last_active_users = val["last_active_users"].asInteger();

      if (val.isMember("active_users") && val["active_users"].isInteger())
        game.active_users = val["active_users"].asInteger();

      if (val.isMember("active_sessions") && val["active_sessions"].isInteger())
        game.active_sessions = val["active_sessions"].asInteger();

      if (val.isMember("thumbnail") && val["thumbnail"].isString())
        game.thumbnail = val["thumbnail"].asString();

      if (val.isMember("online_users") && val["online_users"].isInteger())
        game.online_users = val["online_users"].asInteger();

      if (val.isMember("has_live_aware_feature") && val["has_live_aware_feature"].isBoolean())
        game.has_live_aware_feature = val["has_live_aware_feature"].asBoolean();

      if (val.isMember("has_matchmaking_feature") && val["has_matchmaking_feature"].isBoolean())
        game.has_matchmaking_feature = val["has_matchmaking_feature"].asBoolean();

      if (val.isMember("has_leaderboards_feature") && val["has_leaderboards_feature"].isBoolean())
        game.has_leaderboards_feature = val["has_leaderboards_feature"].asBoolean();

      if (val.isMember("has_user_generated_content_feature") && val["has_user_generated_content_feature"].isBoolean())
        game.has_user_generated_content_feature = val["has_user_generated_content_feature"].asBoolean();

      m_info.m_games.push_back(game);
    }
  }

  strURL = StringUtils::Format("%sstats", INSIGNIA_API);
  if (!MakeRequest(strURL, strResponse))
    return false;

  body = CJSONVariantParser::Parse((const unsigned char *)strResponse.c_str(), strResponse.size());
  if (body.isMember("stats") && body["stats"].isObject())
  {
    const CVariant &obj = body["stats"];
    if (obj.isMember("games_supported") && obj["games_supported"].isString())
      m_info.games_supported = obj["games_supported"].asString();

    if (obj.isMember("registered_users") && obj["registered_users"].isString())
      m_info.registered_users = obj["registered_users"].asString();

    if (obj.isMember("users_online_now") && obj["users_online_now"].isString())
      m_info.users_online_now = obj["users_online_now"].asString();
  }

  CDateTime time = CDateTime::GetCurrentDateTime();
  m_info.lastUpdateTime = time.GetAsLocalizedDateTime(false, false);

  SetWindowProperties();

  // send a message that we're done
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_INSIGNIA_FETCHED);
  g_windowManager.SendThreadMessage(msg);

  return true;
}

const CInsigniaInfo &CInsigniaJob::GetInfo() const
{
  return m_info;
}

void CInsigniaJob::SetWindowProperties()
{
  CGUIWindowInsignia* window = (CGUIWindowInsignia*)g_windowManager.GetWindow(WINDOW_INSIGNIA);
  if (window)
  {
    CGUIBaseContainer* gamesContainer = window->GetGamesContainer();
    if (!gamesContainer)
    {
      CGUIControl *control = window->GetControl(CONTROL_GAMES_LIST);
      if (control && (control->GetControlType() == CGUIControl::GUICONTAINER_LIST || control->GetControlType() == CGUIControl::GUICONTAINER_WRAPLIST ||
                      control->GetControlType() == CGUIControl::GUICONTAINER_FIXEDLIST || control->GetControlType() == CGUIControl::GUICONTAINER_PANEL))
      {
        gamesContainer = (CGUIBaseContainer*)control;
        window->InitializeGamesContainer(gamesContainer);
      }
      else
        return;
    }

    std::vector<CGUIStaticItemPtr> items;
    for (std::vector<game_info>::const_iterator it = m_info.m_games.begin(); it != m_info.m_games.end(); ++it)
    {
      CFileItemPtr item(new CFileItem(it->name));
      item->SetIconImage(it->thumbnail);
      item->SetProperty("code", it->code);
      item->SetProperty("name", it->name);
      item->SetProperty("serial", it->serial);
      item->SetProperty("url", it->url);
      item->SetProperty("last_active_sessions", it->last_active_sessions);
      item->SetProperty("last_active_users", it->last_active_users);
      item->SetProperty("active_users", it->last_active_users);
      item->SetProperty("active_sessions", it->active_sessions);
      item->SetProperty("online_users", it->active_users);
      item->SetProperty("has_live_aware_feature", it->has_live_aware_feature);
      item->SetProperty("has_matchmaking_feature", it->has_matchmaking_feature);
      item->SetProperty("has_leaderboards_feature", it->has_leaderboards_feature);
      item->SetProperty("has_user_generated_content_feature", it->has_user_generated_content_feature);
      CGUIStaticItemPtr staticItem(new CGUIStaticItem(*item));
      items.push_back(staticItem);
    }

    gamesContainer->SetListProvider(new CStaticListProvider(items));
  }
}

CInsignia::CInsignia(void) : CInfoLoader(30 * 60 * 1000) // 30 minutes
{
  Reset();
}

CInsignia::~CInsignia(void)
{
}

std::string CInsignia::TranslateInfo(int info) const
{
  if (info == INSIGNIA_LABEL_GAMES_SUPPORTED) return m_info.games_supported;
  else if (info == INSIGNIA_LABEL_REGISTERED_USERS) return m_info.registered_users;
  else if (info == INSIGNIA_LABEL_ONLINE_USERS) return m_info.users_online_now;
  return "";
}

void CInsignia::Reset()
{
  m_info.Reset();
}

bool CInsignia::IsFetched()
{
  // call GetInfo() to make sure that we actually start up
  GetInfo(0);
  return !m_info.lastUpdateTime.empty();
}

CJob *CInsignia::GetJob() const
{
  return new CInsigniaJob();
}

void CInsignia::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  m_info = ((CInsigniaJob *)job)->GetInfo();
  CInfoLoader::OnJobComplete(jobID, success, job);
}
