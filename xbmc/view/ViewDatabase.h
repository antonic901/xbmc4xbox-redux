/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dbwrappers/Database.h"

#include <string>

class CViewState;

class CViewDatabase : public CDatabase
{
public:
  CViewDatabase();
  virtual ~CViewDatabase();
  virtual bool Open();

  bool GetViewState(const std::string &path, int windowID, CViewState &state, const std::string &skin);
  bool SetViewState(const std::string &path, int windowID, const CViewState &state, const std::string &skin);
  bool ClearViewStates(int windowID);

protected:
  virtual void CreateTables();
  virtual void CreateAnalytics();
  virtual void UpdateTables(int version);
  virtual int GetSchemaVersion() const { return 6; }
  virtual const char *GetBaseDBName() const { return "ViewModes"; }
};
