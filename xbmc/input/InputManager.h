/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/actions/Action.h"
#include "input/keyboard/Key.h"

#include "XBInput.h"
#include "XBInputEx.h"
#include "input/Keyboard.h"
#include "utils/DelayController.h"

#include <string>
#include <vector>

class CInputManager
{
public:
  CInputManager();
  CInputManager(const CInputManager&);
  CInputManager const& operator=(CInputManager const&);
  virtual ~CInputManager();

  /*! \brief decode an input event from remote controls.

   \param windowId Currently active window
   \return true if event is handled, false otherwise
  */
  bool ProcessRemote(int windowId, float frameTime);

  /*! \brief decode a gamepad or joystick event, reset idle timers.

  \param windowId Currently active window
  \return true if event is handled, false otherwise
  */
  bool ProcessGamepad(int windowId, float frameTime);

  /*! \brief decode an input event from remote controls.

   \return true if event is handled, false otherwise
  */
  bool ProcessKeyboard();

  /*! \brief Process all inputs
   *
   * \param windowId Currently active window
   * \param frameTime Time in seconds since last call
   * \return true on success, false otherwise
   */
  bool Process(int windowId, float frameTime);

  /*!
   * \brief Call once during application startup to initialize peripherals that need it
   */
  void InitializeInputs();

private:

  /*! \brief Process keyboard event and translate into an action
  *
  * \param CKey keypress details
  * \return true on succesfully handled event
  * \sa CKey
  */
  bool OnKey(const CKey& key);

  /*! \brief Determine if an action should be processed or just
  *   cancel the screensaver
  *
  * \param action Action that is about to be processed
  * \return true on any poweractions such as shutdown/reboot/sleep/suspend, false otherwise
  * \sa CAction
  */
  bool AlwaysProcess(const CAction& action);

  /*! \brief Send the Action to CApplication for further handling,
  *   play a sound before or after sending the action.
  *
  * \param action Action to send to CApplication
  * \return result from CApplication::OnAction
  * \sa CAction
  */
  bool ExecuteInputAction(const CAction &action);

  CKey m_LastKey;

  void ReadInput();

  CDelayController m_ctrDpad;

  XBGAMEPAD* m_Gamepad;
  XBGAMEPAD m_DefaultGamepad;

  XBIR_REMOTE m_IR_Remote[4];
  XBIR_REMOTE m_DefaultIR_Remote;

  CKeyboard m_Keyboard;
};
