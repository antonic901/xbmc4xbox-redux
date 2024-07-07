/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://xbmc.org
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

#include "OpticalBuiltins.h"

#include "system.h"

#ifdef HAS_DVD_DRIVE
#include "storage/MediaManager.h"
#include "xbox/IoSupport.h"
#endif

#ifdef HAS_CDDA_RIPPER
#include "cdrip/CDDARipper.h"
#endif

/*! \brief Eject the tray of an optical drive.
 *  \param params (ignored)
 */
static int Eject(const std::vector<std::string>& params)
{
#ifdef HAS_DVD_DRIVE
  CIoSupport::ToggleTray();
#endif

  return 0;
}

/*! \brief Rip currently inserted CD.
 *  \param params (ignored)
 */
static int RipCD(const std::vector<std::string>& params)
{
#ifdef HAS_CDDA_RIPPER
  CCDDARipper::GetInstance().RipCD();
#endif

  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_9 Optical container built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`EjectTray`</b>
///     ,
///     Either opens or closes the DVD tray\, depending on its current state.
///   }
///   \table_row2_l{
///     <b>`RipCD`</b>
///     ,
///     Will rip the inserted CD from the DVD-ROM drive.
///   }
/// \table_end
///

CBuiltins::CommandMap COpticalBuiltins::GetOperations() const
{
  CBuiltins::CommandMap commands;

  CBuiltins::BUILT_IN builtin1 = {"Close or open the DVD tray", 0, Eject};
  commands.insert(std::make_pair("ejecttray", builtin1));

  CBuiltins::BUILT_IN builtin2 = {"Rip the currently inserted audio CD", 0, RipCD};
  commands.insert(std::make_pair("ripcd", builtin2));

  return commands;
}

