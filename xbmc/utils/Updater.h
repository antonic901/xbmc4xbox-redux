/*
 *  Copyright (C) 2025-2025
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "utils/Job.h"


class CUpdaterJob : public CJob
{
  // implementation of CJob
  virtual bool DoWork();
  virtual const char *GetType() const { return "AutoUpdater"; }
  virtual bool operator==(const CJob* job) const;

  virtual bool ShouldCancel(unsigned int progress, unsigned int total) const { return false; }
};
