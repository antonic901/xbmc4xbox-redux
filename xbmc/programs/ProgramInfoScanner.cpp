/*
 *  Copyright (C) 2025-2025 Team XBMC
 *  This file is part of XBMC - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ProgramInfoScanner.h"

#include "dialogs/GUIDialogExtendedProgressBar.h"

namespace PROGRAM
{
  CProgramInfoScanner::CProgramInfoScanner()
  {
    m_bStop = false;
  }

  CProgramInfoScanner::~CProgramInfoScanner()
  {
  }

  void CProgramInfoScanner::Process()
  {
    m_bStop = false;

    // TODO: do scanning here

    m_bRunning = false;

    if (m_handle)
      m_handle->MarkFinished();
    m_handle = NULL;
  }

  void CProgramInfoScanner::Start(const std::string& strDirectory)
  {
    m_bRunning = true;
    Process();
  }

  void CProgramInfoScanner::Stop()
  {
    m_bStop = true;
  }

  bool CProgramInfoScanner::DoScan(const std::string& strDirectory)
  {
    // TODO: implement scanning logic here
    return false;
  }
}
