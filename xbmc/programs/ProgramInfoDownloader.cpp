/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "ProgramInfoDownloader.h"
#include "Util.h"
#include "utils/HTMLUtil.h"
#include "guilib/XMLUtils.h"
#include "utils/RegExp.h"
#include "utils/ScraperParser.h"
#include "NfoFile.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogOK.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace std;
using namespace HTML;

#ifndef __GNUC__
#pragma warning (disable:4018)
#endif

// return value: 0 = we failed, -1 = we failed and reported an error, 1 = success
int CProgramInfoDownloader::InternalFindGame(const CStdString &strGame,
                                            GAMELIST& gamelist,
                                            bool cleanChars /* = true */)
{
  try
  {
    gamelist = m_info->FindMovie(m_http, strGame, cleanChars);
  }
  catch (const ADDON::CScraperError &sce)
  {
    ShowErrorDialog(sce);
    return sce.FAborted() ? 0 : -1;
  }
  return 1;  // success
}

void CProgramInfoDownloader::ShowErrorDialog(const ADDON::CScraperError &sce)
{
  if (!sce.Title().empty())
  {
    CGUIDialogOK *pdlg = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    pdlg->SetHeading(sce.Title());
    pdlg->SetLine(0, sce.Message());
    g_application.getApplicationMessenger().DoModal(pdlg, WINDOW_DIALOG_OK);
  }
}

// threaded functions
void CProgramInfoDownloader::Process()
{
  // note here that we're calling our external functions but we're calling them with
  // no progress bar set, so they're effectively calling our internal functions directly.
  m_found = 0;
  if (m_state == FIND_GAME)
  {
    if (!(m_found=FindGame(m_strGame, m_gameList)))
      CLog::Log(LOGERROR, "%s: Error looking up item %s", __FUNCTION__, m_strGame.c_str());
    m_state = DO_NOTHING;
    return;
  }

  if (m_url.m_url.empty())
  {
    // empty url when it's not supposed to be..
    // this might happen if the previously scraped item was removed from the site (see ticket #10537)
    CLog::Log(LOGERROR, "%s: Error getting details for %s due to an empty url", __FUNCTION__, m_strGame.c_str());
  }
  else if (m_state == GET_DETAILS)
  {
    if (!GetDetails(m_url, m_gameDetails))
      CLog::Log(LOGERROR, "%s: Error getting details from %s", __FUNCTION__,m_url.m_url[0].m_url.c_str());
  }
  m_found = 1;
  m_state = DO_NOTHING;
}

int CProgramInfoDownloader::FindGame(const CStdString &strGame,
                                    GAMELIST& gameList,
                                    CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CProgramInfoDownloader::FindGame(%s)", strGame.c_str());

  if (pProgress)
  { // threaded version
    m_state = FIND_GAME;
    m_strGame = strGame;
    m_found = 0;
    if (ThreadHandle())
      StopThread();
    Create();
    while (m_state != DO_NOTHING)
    {
      pProgress->Progress();
      if (pProgress->IsCanceled())
      {
        CloseThread();
        return 0;
      }
      Sleep(1);
    }
    // transfer to our gamelist
    m_gameList.swap(gameList);
    int found=m_found;
    CloseThread();
    return found;
  }

  // unthreaded
  int success = InternalFindGame(strGame, gameList);
  // NOTE: this might be improved by rescraping if the match quality isn't high?
  if (success == 1 && gameList.empty())
  { // no results. try without cleaning chars like '.' and '_'
    success = InternalFindGame(strGame, gameList, false);
  }
  return success;
}

bool CProgramInfoDownloader::GetDetails(const CScraperUrl &url,
                                      CProgramInfoTag &gameDetails,
                                      CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CProgramInfoDownloader::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_gameDetails = gameDetails;

  // fill in the defaults
  gameDetails.Reset();
  if (pProgress)
  { // threaded version
    m_state = GET_DETAILS;
    m_found = 0;
    if (ThreadHandle())
      StopThread();
    Create();
    while (!m_found)
    {
      pProgress->Progress();
      if (pProgress->IsCanceled())
      {
        CloseThread();
        return false;
      }
      Sleep(1);
    }
    gameDetails = m_gameDetails;
    CloseThread();
    return true;
  }
  else  // unthreaded
    return m_info->GetProgramDetails(m_http, url, gameDetails);
}

void CProgramInfoDownloader::CloseThread()
{
  m_http.Cancel();
  StopThread();
  m_http.Reset();
  m_state = DO_NOTHING;
  m_found = 0;
}

