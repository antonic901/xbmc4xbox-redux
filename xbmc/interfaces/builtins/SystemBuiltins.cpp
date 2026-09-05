/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SystemBuiltins.h"

#include "ServiceBroker.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/StringUtils.h"

/*! \brief Execute a system executable.
 *  \param params The parameters.
 *  \details params[0] = The path to the executable.
 *
 *  Set the template parameter Wait to true to wait for execution exit.
 */
  template<int Wait>
static int Exec(const std::vector<std::string>& params)
{
  return 0;
}

/*! \brief Inhibit idle shutdown timer.
 *  \param params The parameters.
 *  \details params[0] = "true" to inhibit shutdown timer (optional).
 */
static int InhibitIdle(const std::vector<std::string>& params)
{
  bool inhibit = (params.size() == 1 && StringUtils::EqualsNoCase(params[0], "true"));
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_INHIBITIDLESHUTDOWN, inhibit);

  return 0;
}

/*! \brief Powerdown system.
 *  \param params (ignored)
 */
static int Powerdown(const std::vector<std::string>& params)
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_POWERDOWN);

  return 0;
}

/*! \brief Quit application.
 *  \param params (ignored)
 */
static int Quit(const std::vector<std::string>& params)
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_QUIT);

  return 0;
}

/*! \brief Reboot system.
 *  \param params (ignored)
 */
static int Reboot(const std::vector<std::string>& params)
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_RESTART);

  return 0;
}

/*! \brief Restart application.
 *  \param params (ignored)
 */
static int RestartApp(const std::vector<std::string>& params)
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_RESTARTAPP);

  return 0;
}

/*! \brief Activate screensaver.
 *  \param params (ignored)
 */
static int ActivateScreensaver(const std::vector<std::string>& params)
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_ACTIVATESCREENSAVER);

  return 0;
}

/*! \brief Reset screensaver.
 *  \param params (ignored)
 */
static int ResetScreensaver(const std::vector<std::string>& params)
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_RESETSCREENSAVER);

  return 0;
}

/*! \brief Inhibit screensaver.
 *  \param params The parameters.
 *  \details params[0] = "true" to inhibit screensaver (optional).
 */
static int InhibitScreenSaver(const std::vector<std::string>& params)
{
  bool inhibit = (params.size() == 1 && StringUtils::EqualsNoCase(params[0], "true"));
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_INHIBITSCREENSAVER, inhibit);

  return 0;
}

/*! \brief Shutdown system.
 *  \param params (ignored)
 */
static int Shutdown(const std::vector<std::string>& params)
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SHUTDOWN);

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
///     <b>`InhibitScreensaver(yesNo)`</b>
///     ,
///     Inhibit the screensaver
///     @param[in] yesNo   value with "true" or "false" to inhibit or allow screensaver (leaving empty defaults to false)
///   }
///   \table_row2_l{
///     <b>`InhibitIdleShutdown(true/false)`</b>
///     ,
///     Prevent the system to shutdown on idle.
///     @param[in] value                 "true" to inhibit shutdown timer (optional).
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
///     Reset the system (same as reboot)
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

  CBuiltins::BUILT_IN builtin1 = {"Activate Screensaver", 0, ActivateScreensaver};
  commands.insert(std::make_pair("activatescreensaver", builtin1));

  CBuiltins::BUILT_IN builtin2 = {"Reset Screensaver", 0, ResetScreensaver};
  commands.insert(std::make_pair("resetscreensaver", builtin2));

  CBuiltins::BUILT_IN builtin4 = {"Inhibit idle shutdown", 0, InhibitIdle};
  commands.insert(std::make_pair("inhibitidleshutdown", builtin4));

  CBuiltins::BUILT_IN builtin5 = {"Inhibit Screensaver", 0, InhibitScreenSaver};
  commands.insert(std::make_pair("inhibitscreensaver", builtin5));

  CBuiltins::BUILT_IN builtin7 = {"Powerdown system", 0, Powerdown};
  commands.insert(std::make_pair("powerdown", builtin7));

  CBuiltins::BUILT_IN builtin8 = {"Quit Kodi", 0, Quit};
  commands.insert(std::make_pair("quit", builtin8));

  CBuiltins::BUILT_IN builtin9 = {"Reboot the system", 0, Reboot};
  commands.insert(std::make_pair("reboot", builtin9));

  CBuiltins::BUILT_IN builtin10 = {"Reset the system (same as reboot)", 0, Reboot};
  commands.insert(std::make_pair("reset", builtin10));

  CBuiltins::BUILT_IN builtin11 = {"Restart the system (same as reboot)", 0, Reboot};
  commands.insert(std::make_pair("restart", builtin11));

  CBuiltins::BUILT_IN builtin12 = {"Restart Kodi", 0, RestartApp};
  commands.insert(std::make_pair("restartapp", builtin12));

  CBuiltins::BUILT_IN builtin13 = {"Shutdown the system", 0, Shutdown};
  commands.insert(std::make_pair("shutdown", builtin13));

  CBuiltins::BUILT_IN builtin15 = {"Execute shell commands", 1, Exec<0>};
  commands.insert(std::make_pair("system.exec", builtin15));

  CBuiltins::BUILT_IN builtin16 = {"Execute shell commands and freezes Kodi until shell is closed", 1, Exec<1>};
  commands.insert(std::make_pair("system.execwait", builtin16));

  return commands;
}
