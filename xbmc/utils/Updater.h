/*
 *  Copyright (C) 2025-2025
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "utils/Job.h"

#include <string>

enum AUTO_UPDATER
{
  AUTO_UPDATER_NOTIFY = 1,
  AUTO_UPDATER_NEVER
};

class CUpdaterJob : public CJob
{
public:
  CUpdaterJob(bool notify = false, bool bPromptInstall = false);

  // implementation of CJob
  virtual bool DoWork();
  virtual const char *GetType() const { return "AutoUpdater"; }
  virtual bool operator==(const CJob* job) const;

  virtual bool ShouldCancel(unsigned int progress, unsigned int total) const { return false; }

private:
  virtual void DoInstall(const std::string& strCurrentVersion, const std::string& strCurrentRevision, const std::string& strUpdateChannel);

  bool m_notify;
  bool m_install;
};
