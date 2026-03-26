/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirtyRegionSolver.h"

class CUnionDirtyRegionSolver : public IDirtyRegionSolver
{
public:
  virtual void Solve(const CDirtyRegionList &input, CDirtyRegionList &output);
};

class CFillViewportAlwaysRegionSolver : public IDirtyRegionSolver
{
public:
  virtual void Solve(const CDirtyRegionList &input, CDirtyRegionList &output);
};

class CFillViewportOnChangeRegionSolver : public IDirtyRegionSolver
{
public:
  virtual void Solve(const CDirtyRegionList &input, CDirtyRegionList &output);
};

class CGreedyDirtyRegionSolver : public IDirtyRegionSolver
{
public:
  CGreedyDirtyRegionSolver();
  virtual void Solve(const CDirtyRegionList &input, CDirtyRegionList &output);
private:
  float m_costNewRegion;
  float m_costPerArea;
};
