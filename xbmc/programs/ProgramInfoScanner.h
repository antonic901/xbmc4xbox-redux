#pragma once
/*
 *  Copyright (C) 2025-2025
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "InfoScanner.h"
#include "ProgramDatabase.h"
#include "addons/Scraper.h"

namespace PROGRAM
{
  class CProgramInfoScanner : public CInfoScanner
  {
  public:
    CProgramInfoScanner();
    virtual ~CProgramInfoScanner();

    void Start(const std::string& strDirectory);
    void Stop();

  protected:
    virtual void Process();
    virtual bool DoScan(const std::string& strDirectory);

    bool m_bStop;
    std::string m_strDirectory;
    CProgramDatabase m_database;

  private:
    bool DoScraping(const std::string& strDirectory, const ADDON::ScraperPtr& scraper, int idPath = - 1);
    void ScrapeProgram(const std::string& strPath, const ADDON::ScraperPtr& scraper, int idPath);
  };
}
