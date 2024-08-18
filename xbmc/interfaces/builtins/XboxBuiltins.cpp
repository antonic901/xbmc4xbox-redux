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

#include "XboxBuiltins.h"

#include "Util.h"
#include "FileItem.h"
#include "settings/Settings.h"
#include "utils/SystemInfo.h"
#include "xbox/xbeheader.h"

/*! \brief Boot custom dashboard.
 *  \param params (ignored)
 */
static int RunDashboard(const std::vector<std::string>& params)
{
  if (CSettings::GetInstance().GetBool("myprograms.usedashpath"))
    CUtil::RunXBE(CSettings::GetInstance().GetString("myprograms.dashboard").c_str());
  else
    CUtil::BootToDash();

  return 0;
}

/*! \brief Run specified XBE executable.
 *  \param params The parameters.
 *  \details params[0] = Path to XBE executable.
 */
static int RunXBE(const std::vector<std::string>& params)
{
  CFileItem item(params[0]);
  item.SetPath(params[0]);
  if (item.IsShortCut())
    CUtil::RunShortcut(params[0].c_str());
  else if (item.IsXBE())
  {
    int iRegion;
    if (CSettings::GetInstance().GetBool("myprograms.gameautoregion"))
    {
      CXBE xbe;
      iRegion = xbe.ExtractGameRegion(params[0]);
      if (iRegion < 1 || iRegion > 7)
        iRegion = 0;
      iRegion = xbe.FilterRegion(iRegion);
    }
    else
      iRegion = 0;

    CUtil::RunXBE(params[0].c_str(),NULL,F_VIDEO(iRegion));
  }

  return 0;
}

/*! \brief change power button RGB LEDs.
 *  \param params The parameters.
 *  \details params[0] = RGB A channel.
 *           params[1] = RGB B channel.
 *           params[2] = White A.
 *           params[3] = White B.
 *           params[4] = Name of transition.
 *           params[5] = Time (of transition maybe?).
 */
static int PWMControl(const std::vector<std::string>& params)
{
  int iTrTime = 0;
  std::string parameter = params.size() ? params[0] : "";
  std::vector<std::string> arSplit = StringUtils::Split(parameter,",");
  std::string strTemp ,strRgbA, strRgbB, strWhiteA, strWhiteB, strTran;

  if ((int)arSplit.size() >= 6)
  {
    strRgbA  = arSplit[0].c_str();
    strRgbB  = arSplit[1].c_str();
    strWhiteA= arSplit[2].c_str();
    strWhiteB= arSplit[3].c_str();
    strTran  = arSplit[4].c_str();
    iTrTime  = atoi(arSplit[5].c_str());
  }
  else if(parameter.size() > 6)
  {
    strRgbA = strRgbB = parameter;
    strWhiteA = strWhiteB = "#000000";
    strTran = "none";
  }
  CUtil::PWMControl(strRgbA,strRgbB,strWhiteA,strWhiteB,strTran, iTrTime);

  return 0;
}

/*! \brief Backup System Informations.
 *  \param params (ignored)
 */
static int Backup(const std::vector<std::string>& params)
{
  g_sysinfo.WriteTXTInfoFile();
  g_sysinfo.CreateBiosBackup();
  g_sysinfo.CreateEEPROMBackup();
  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_17 Xbox built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`Dashboard`</b>
///     ,
///     Boot dashboard defined in settings (default is MS Dashboard).
///   }
///   \table_row2_l{
///     <b>`RunXBE(path\to\xbe)`</b>
///     ,
///     Run XBE at specified path.
///     @param[in] path                  Path to XBE executable.
///   }
///   \table_row2_l{
///     <b>`PWMControl(rgbA, rgbB, whiteA, whiteB, transition, time)`</b>
///     ,
///     Control PWM RGB LEDs
///   }
///   \table_row2_l{
///     <b>`BackupSystemInfo`</b>
///     ,
///     Backup BIOS, EEPROM, HDD key and other systen informations 
///   }
/// \table_end
///

CBuiltins::CommandMap CXboxBuiltins::GetOperations() const
{
  CBuiltins::CommandMap commands;

  CBuiltins::BUILT_IN builtin1 = {"Run your dashboard", 0, RunDashboard};
  commands.insert(std::make_pair("dashboard", builtin1));

  CBuiltins::BUILT_IN builtin2 = {"Run the specified executable", 1, RunXBE};
  commands.insert(std::make_pair("runxbe", builtin2));

  CBuiltins::BUILT_IN builtin3 = {"Control PWM RGB LEDs", 6, PWMControl};
  commands.insert(std::make_pair("pwmcontrol", builtin3));

  CBuiltins::BUILT_IN builtin4 = {"Backup System Informations to local hdd", 0, Backup};
  commands.insert(std::make_pair("backupsysteminfo",  builtin4));

  return commands;
}
