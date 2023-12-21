#pragma once

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

#include "utils/Thread.h"
#include "programs/ProgramInfoTag.h"
#include "addons/Scraper.h"
#include "DateTime.h"
#include "filesystem/CurlFile.h"

// forward declarations
class TiXmlDocument;
class CGUIDialogProgress;

namespace ADDON
{
class CScraperError;
}

typedef std::vector<CScraperUrl> GAMELIST;

class CProgramInfoDownloader : public CThread
{
public:
  CProgramInfoDownloader(const ADDON::ScraperPtr &scraper) : m_info(scraper) {}
  virtual ~CProgramInfoDownloader() {}

  // threaded lookup functions

  /*! \brief Do a search for matching program items (possibly asynchronously) with our scraper
   \param strGame name of the program item to look for
   \param gamelist [out] list of results to fill. May be empty on success.
   \param pProgress progress bar to update as we go. If NULL we run on thread, if non-NULL we run off thread.
   \return 1 on success, -1 on a scraper-specific error, 0 on some other error
   */
  int FindGame(const CStdString& strGame, GAMELIST& gamelist, CGUIDialogProgress *pProgress = NULL);
  bool GetDetails(const CScraperUrl& url, CProgramInfoTag &gameDetails, CGUIDialogProgress *pProgress = NULL);

  static void ShowErrorDialog(const ADDON::CScraperError &sce);

protected:
  enum LOOKUP_STATE { DO_NOTHING = 0,
                      FIND_GAME = 1,
                      GET_DETAILS = 2 };

  XFILE::CCurlFile  m_http;
  CStdString        m_strGame;
  GAMELIST          m_gameList;
  CProgramInfoTag   m_gameDetails;
  CScraperUrl       m_url;
  LOOKUP_STATE      m_state;
  int               m_found;
  ADDON::ScraperPtr m_info;

  // threaded stuff
  void Process();
  void CloseThread();

  int InternalFindGame(const CStdString& strGame, GAMELIST& gamelist, bool cleanChars = true);
};

