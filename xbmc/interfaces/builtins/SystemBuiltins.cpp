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

#include "SystemBuiltins.h"

#include "messaging/ApplicationMessenger.h"
#include "utils/StringUtils.h"

using namespace KODI::MESSAGING;

/*! \brief Execute a system executable.
 *  \param params The parameters.
 *  \details params[0] = The path to the executable.
 *
 *  Set the template parameter Wait to true to wait for execution exit.
 */
  template<int Wait>
static int Exec(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().PostMsg(TMSG_MINIMIZE);
  CApplicationMessenger::Get().PostMsg(TMSG_EXECUTE_OS, Wait, -1, nullptr, params[0]);

  return 0;
}

/*! \brief Hibernate system.
 *  \param params (ignored)
 */
static int Hibernate(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().PostMsg(TMSG_HIBERNATE);

  return 0;
}

/*! \brief Inhibit idle shutdown timer.
 *  \param params The parameters.
 *  \details params[0] = "true" to inhibit shutdown timer (optional).
 */
static int InhibitIdle(const std::vector<std::string>& params)
{
  bool inhibit = (params.size() == 1 && StringUtils::EqualsNoCase(params[0], "true"));
  CApplicationMessenger::Get().PostMsg(TMSG_INHIBITIDLESHUTDOWN, inhibit);

  return 0;
}

/*! \brief Minimize application.
 *  \param params (ignored)
 */
static int Minimize(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().PostMsg(TMSG_MINIMIZE);

  return 0;
}

/*! \brief Powerdown system.
 *  \param params (ignored)
 */
static int Powerdown(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().PostMsg(TMSG_POWERDOWN);

  return 0;
}

/*! \brief Quit application.
 *  \param params (ignored)
 */
static int Quit(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().PostMsg(TMSG_QUIT);

  return 0;
}

/*! \brief Reboot system.
 *  \param params (ignored)
 */
static int Reboot(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().PostMsg(TMSG_RESTART);

  return 0;
}

/*! \brief Soft reset system.
 *  \param params (ignored)
 */
static int Reset(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().PostMsg(TMSG_RESET);

  return 0;
}

/*! \brief Restart application.
 *  \param params (ignored)
 */
static int RestartApp(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().PostMsg(TMSG_RESTARTAPP);

  return 0;
}

/*! \brief Activate screensaver.
 *  \param params (ignored)
 */
static int Screensaver(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().PostMsg(TMSG_ACTIVATESCREENSAVER);

  return 0;
}

/*! \brief Shutdown system.
 *  \param params (ignored)
 */
static int Shutdown(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().PostMsg(TMSG_SHUTDOWN);

  return 0;
}

/*! \brief Suspend system.
 *  \param params (ignored)
 */
static int Suspend(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().PostMsg(TMSG_SUSPEND);

  return 0;
}


// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_15 System built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`ActivateScreensaver`</b>
///     ,
///     Starts the screensaver
///   }
///   \table_row2_l{
///     <b>`Hibernate`</b>
///     ,
///     Hibernate (S4) the System
///   }
///   \table_row2_l{
///     <b>`InhibitIdleShutdown(true/false)`</b>
///     ,
///     Prevent the system to shutdown on idle.
///     @param[in] value                 "true" to inhibit shutdown timer (optional).
///   }
///   \table_row2_l{
///     <b>`Minimize`</b>
///     ,
///     Minimizes Kodi
///   }
///   \table_row2_l{
///     <b>`Powerdown`</b>
///     ,
///     Powerdown system
///   }
///   \table_row2_l{
///     <b>`Quit`</b>
///     ,
///     Quits Kodi
///   }
///   \table_row2_l{
///     <b>`Reboot`</b>
///     ,
///     Cold reboots the system (power cycle)
///   }
///   \table_row2_l{
///     <b>`Reset`</b>
///     ,
///     Soft reset the system
///   }
///   \table_row2_l{
///     <b>`Restart`</b>
///     ,
///     Restart the system (same as reboot)
///   }
///   \table_row2_l{
///     <b>`RestartApp`</b>
///     ,
///     Restarts Kodi (only implemented under Windows and Linux)
///   }
///   \table_row2_l{
///     <b>`ShutDown`</b>
///     ,
///     Trigger default Shutdown action defined in System Settings
///   }
///   \table_row2_l{
///     <b>`Suspend`</b>
///     ,
///     Suspends (S3 / S1 depending on bios setting) the System
///   }
///   \table_row2_l{
///     <b>`System.Exec(exec)`</b>
///     ,
///     Execute shell commands
///     @param[in] exec                  The path to the executable
///   }
///   \table_row2_l{
///     <b>`System.ExecWait(exec)`</b>
///     ,
///     Execute shell commands and freezes Kodi until shell is closed
///     @param[in] exec                  The path to the executable
///   }
/// \table_end
///

CBuiltins::CommandMap CSystemBuiltins::GetOperations() const
{
  CBuiltins::CommandMap commands;

  CBuiltins::BUILT_IN builtin1 = {"Activate Screensaver", 0, Screensaver};
  commands.insert(std::make_pair("activatescreensaver", builtin1));

  CBuiltins::BUILT_IN builtin2 = {"Hibernates the system", 0, Hibernate};
  commands.insert(std::make_pair("hibernate", builtin2));

  CBuiltins::BUILT_IN builtin3 = {"Inhibit idle shutdown", 0, InhibitIdle};
  commands.insert(std::make_pair("inhibitidleshutdown", builtin3));

  CBuiltins::BUILT_IN builtin4 = {"Minimize Kodi", 0, Minimize};
  commands.insert(std::make_pair("minimize", builtin4));

  CBuiltins::BUILT_IN builtin5 = {"Powerdown system", 0, Powerdown};
  commands.insert(std::make_pair("powerdown", builtin5));

  CBuiltins::BUILT_IN builtin6 = {"Quit Kodi", 0, Quit};
  commands.insert(std::make_pair("quit", builtin6));

  CBuiltins::BUILT_IN builtin7 = {"Reboot the system", 0, Reboot};
  commands.insert(std::make_pair("reboot", builtin7));
  commands.insert(std::make_pair("restart", builtin7));

  CBuiltins::BUILT_IN builtin8 = {"Soft reset the system", 0, Reset};
  commands.insert(std::make_pair("reset", builtin8));

  CBuiltins::BUILT_IN builtin9 = {"Restart Kodi", 0, RestartApp};
  commands.insert(std::make_pair("restartapp", builtin9));

  CBuiltins::BUILT_IN builtin10 = {"Shutdown the system", 0, Shutdown};
  commands.insert(std::make_pair("shutdown", builtin10));

  CBuiltins::BUILT_IN builtin11 = {"Suspends the system", 0, Suspend};
  commands.insert(std::make_pair("suspend", builtin11));

  CBuiltins::BUILT_IN builtin12 = {"Execute shell commands", 1, Exec<0>};
  commands.insert(std::make_pair("system.exec", builtin12));

  CBuiltins::BUILT_IN builtin13 = {"Execute shell commands and freezes Kodi until shell is closed", 1, Exec<1>};
  commands.insert(std::make_pair("system.execwait", builtin13));

  return commands;
}
