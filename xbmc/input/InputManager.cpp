/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputManager.h"

#include "ButtonTranslator.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationXbox.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/Keyboard.h"
#include "input/keyboard/KeyIDs.h"
#include "input/keyboard/XBMC_vkeys.h"
#include "network/EventServer.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/ExecString.h"
#include "utils/log.h"

using EVENTSERVER::CEventServer;

using namespace KODI::MESSAGING;

CInputManager::CInputManager() : m_ctrDpad(220, 220)
{
}

CInputManager::~CInputManager()
{
}

void CInputManager::InitializeInputs()
{
  if (FAILED(XBInput_CreateGamepads(&m_Gamepad)))
  {
    CLog::Log(LOGFATAL, "%s - failed to initialize gamepads!");
  }
  if (FAILED(XBInput_CreateIR_Remotes()))
  {
    CLog::Log(LOGFATAL, "%s - failed to initialize remotes!");
  }

  m_Keyboard.Initialize(NULL);

  m_ctrDpad.SetDelays(100, 500);
}

float DeadZone(float &f)
{
  const boost::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  if (f > advancedSettings->m_controllerDeadzone)
    return (f - advancedSettings->m_controllerDeadzone) / (1.0f - advancedSettings->m_controllerDeadzone);
  if (f < -advancedSettings->m_controllerDeadzone)
    return (f + advancedSettings->m_controllerDeadzone) / (1.0f - advancedSettings->m_controllerDeadzone);
  return 0.0f;
}

float MaxTrigger(XBGAMEPAD &gamepad)
{
  float max = fabs(gamepad.fX1);
  if (fabs(gamepad.fX2) > max)
    max = fabs(gamepad.fX2);
  if (fabs(gamepad.fY1) > max)
    max = fabs(gamepad.fY1);
  if (fabs(gamepad.fY2) > max)
    max = fabs(gamepad.fY2);
  return max;
}

void CInputManager::ReadInput()
{
  // Read the input from IR remote
  XBInput_GetInput(m_IR_Remote);
  ZeroMemory(&m_DefaultIR_Remote, sizeof(m_DefaultIR_Remote));
  for (int i = 0; i < 4; i++)
  {
    if (m_IR_Remote[i].hDevice)
    {
      m_DefaultIR_Remote.wButtons = m_IR_Remote[i].wButtons;
    }
  }

  // Read the input from all connected gamepads
  XBInput_GetInput(m_Gamepad);
  ZeroMemory(&m_DefaultGamepad, sizeof(m_DefaultGamepad));
  float maxTrigger = 0.0f;
  for (int i = 0; i < 4; i++)
  {
    if (m_Gamepad[i].hDevice)
    {
      if (maxTrigger < MaxTrigger(m_Gamepad[i]))
      {
        maxTrigger = MaxTrigger(m_Gamepad[i]);
        m_DefaultGamepad.fX1 = m_Gamepad[i].fX1;
        m_DefaultGamepad.fY1 = m_Gamepad[i].fY1;
        m_DefaultGamepad.fX2 = m_Gamepad[i].fX2;
        m_DefaultGamepad.fY2 = m_Gamepad[i].fY2;
      }
      m_DefaultGamepad.wButtons |= m_Gamepad[i].wButtons;
      m_DefaultGamepad.wPressedButtons |= m_Gamepad[i].wPressedButtons;
      m_DefaultGamepad.wLastButtons |= m_Gamepad[i].wLastButtons;

      for (int b = 0; b < 8; b++)
      {
        m_DefaultGamepad.bAnalogButtons[b] |= m_Gamepad[i].bAnalogButtons[b];
        m_DefaultGamepad.bPressedAnalogButtons[b] |= m_Gamepad[i].bPressedAnalogButtons[b];
        m_DefaultGamepad.bLastAnalogButtons[b] |= m_Gamepad[i].bLastAnalogButtons[b];
      }
    }
  }
  // Secure the deadzones of the analog sticks
  m_DefaultGamepad.fX1 = DeadZone(m_DefaultGamepad.fX1);
  m_DefaultGamepad.fY1 = DeadZone(m_DefaultGamepad.fY1);
  m_DefaultGamepad.fX2 = DeadZone(m_DefaultGamepad.fX2);
  m_DefaultGamepad.fY2 = DeadZone(m_DefaultGamepad.fY2);

  // Read the input from keyboard
  m_Keyboard.Update();
}

bool CInputManager::ProcessGamepad(int windowId, float frameTime)
{
  // Handle the gamepad button presses.  We check for button down,
  // then call OnKey() which handles the translation to actions, and sends the
  // action to our window manager's OnAction() function, which filters the messages
  // to where they're supposed to end up, returning true if the message is successfully
  // processed.  If OnKey() returns false, then the key press wasn't processed at all,
  // and we can safely process the next key (or next check on the same key in the
  // case of the analog sticks which can produce more than 1 key event.)

  WORD wButtons = m_DefaultGamepad.wButtons;
  WORD wDpad = wButtons & (XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT);

  BYTE bLeftTrigger = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER];
  BYTE bRightTrigger = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER];
  BYTE bButtonA = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_A];
  BYTE bButtonB = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_B];
  BYTE bButtonX = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_X];
  BYTE bButtonY = m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_Y];

  // pass them through the delay
  WORD wDir = m_ctrDpad.DpadInput(wDpad, 0 != bLeftTrigger, 0 != bRightTrigger);

  // map all controller & remote actions to their keys
  if (m_DefaultGamepad.fX1 || m_DefaultGamepad.fY1)
  {
    CKey key(KEY_BUTTON_LEFT_THUMB_STICK, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (m_DefaultGamepad.fX2 || m_DefaultGamepad.fY2)
  {
    CKey key(KEY_BUTTON_RIGHT_THUMB_STICK, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  // direction specific keys (for defining different actions for each direction)
  // We need to be able to know when it last had a direction, so that we can
  // post the reset direction code the next time around (to reset scrolling,
  // fastforwarding and other analog actions)

  // For the sticks, once it is pushed in one direction (eg up) it will only
  // detect movement in that direction of movement (eg up or down) - the other
  // direction (eg left and right) will not be registered until the stick has
  // been recentered for at least 2 frames.

  // first the right stick
  static lastRightStickKey = 0;
  int newRightStickKey = 0;
  if (lastRightStickKey == KEY_BUTTON_RIGHT_THUMB_STICK_UP || lastRightStickKey == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
  {
    if (m_DefaultGamepad.fY2 > 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
    else if (m_DefaultGamepad.fY2 < 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
    else if (m_DefaultGamepad.fX2 != 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
  }
  else if (lastRightStickKey == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT || lastRightStickKey == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
  {
    if (m_DefaultGamepad.fX2 > 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
    else if (m_DefaultGamepad.fX2 < 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
    else if (m_DefaultGamepad.fY2 != 0)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
  }
  else
  {
    if (m_DefaultGamepad.fY2 > 0 && m_DefaultGamepad.fX2*2 < m_DefaultGamepad.fY2 && -m_DefaultGamepad.fX2*2 < m_DefaultGamepad.fY2)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
    else if (m_DefaultGamepad.fY2 < 0 && m_DefaultGamepad.fX2*2 < -m_DefaultGamepad.fY2 && -m_DefaultGamepad.fX2*2 < -m_DefaultGamepad.fY2)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
    else if (m_DefaultGamepad.fX2 > 0 && m_DefaultGamepad.fY2*2 < m_DefaultGamepad.fX2 && -m_DefaultGamepad.fY2*2 < m_DefaultGamepad.fX2)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
    else if (m_DefaultGamepad.fX2 < 0 && m_DefaultGamepad.fY2*2 < -m_DefaultGamepad.fX2 && -m_DefaultGamepad.fY2*2 < -m_DefaultGamepad.fX2)
      newRightStickKey = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
  }
  if (lastRightStickKey && newRightStickKey != lastRightStickKey)
  { // was held down last time - and we have a new key now
    // post old key reset message...
    CKey key(lastRightStickKey, 0, 0, 0, 0, 0, 0);
    lastRightStickKey = newRightStickKey;
    if (OnKey(key))
      return true;
  }
  lastRightStickKey = newRightStickKey;
  // post the new key's message
  if (newRightStickKey)
  {
    CKey key(newRightStickKey, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }

  // now the left stick
  static lastLeftStickKey = 0;
  int newLeftStickKey = 0;
  if (lastLeftStickKey == KEY_BUTTON_LEFT_THUMB_STICK_UP || lastLeftStickKey == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
  {
    if (m_DefaultGamepad.fY1 > 0)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_UP;
    else if (m_DefaultGamepad.fY1 < 0)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
  }
  else if (lastLeftStickKey == KEY_BUTTON_LEFT_THUMB_STICK_LEFT || lastLeftStickKey == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
  {
    if (m_DefaultGamepad.fX1 > 0)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
    else if (m_DefaultGamepad.fX1 < 0)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
  }
  else
  {
    // check for a new control movement
    if (m_DefaultGamepad.fY1 > 0 && m_DefaultGamepad.fX1 < m_DefaultGamepad.fY1 && -m_DefaultGamepad.fX1 < m_DefaultGamepad.fY1)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_UP;
    else if (m_DefaultGamepad.fY1 < 0 && m_DefaultGamepad.fX1 < -m_DefaultGamepad.fY1 && -m_DefaultGamepad.fX1 < -m_DefaultGamepad.fY1)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
    else if (m_DefaultGamepad.fX1 > 0 && m_DefaultGamepad.fY1 < m_DefaultGamepad.fX1 && -m_DefaultGamepad.fY1 < m_DefaultGamepad.fX1)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
    else if (m_DefaultGamepad.fX1 < 0 && m_DefaultGamepad.fY1 < -m_DefaultGamepad.fX1 && -m_DefaultGamepad.fY1 < -m_DefaultGamepad.fX1)
      newLeftStickKey = KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
  }

  if (lastLeftStickKey && newLeftStickKey != lastLeftStickKey)
  { // was held down last time - and we have a new key now
    // post old key reset message...
    CKey key(lastLeftStickKey, 0, 0, 0, 0, 0, 0);
    lastLeftStickKey = newLeftStickKey;
    if (OnKey(key))
      return true;
  }
  lastLeftStickKey = newLeftStickKey;
  // post the new key's message
  if (newLeftStickKey)
  {
    CKey key(newLeftStickKey, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }

  // Trigger detection
  static lastTriggerKey = 0;
  int newTriggerKey = 0;
  if (bLeftTrigger)
    newTriggerKey = KEY_BUTTON_LEFT_ANALOG_TRIGGER;
  else if (bRightTrigger)
    newTriggerKey = KEY_BUTTON_RIGHT_ANALOG_TRIGGER;
  if (lastTriggerKey && newTriggerKey != lastTriggerKey)
  { // was held down last time - and we have a new key now
    // post old key reset message...
    CKey key(lastTriggerKey, 0, 0, 0, 0, 0, 0);
    lastTriggerKey = newTriggerKey;
    if (OnKey(key))
      return true;
  }
  lastTriggerKey = newTriggerKey;
  // post the new key's message
  if (newTriggerKey)
  {
    CKey key(newTriggerKey, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }

  // Now the digital buttons...
  if (wDir & DC_LEFTTRIGGER)
  {
    CKey key(KEY_BUTTON_LEFT_TRIGGER, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (wDir & DC_RIGHTTRIGGER)
  {
    CKey key(KEY_BUTTON_RIGHT_TRIGGER, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (wDir & DC_LEFT )
  {
    CKey key(KEY_BUTTON_DPAD_LEFT, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (wDir & DC_RIGHT)
  {
    CKey key(KEY_BUTTON_DPAD_RIGHT, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (wDir & DC_UP )
  {
    CKey key(KEY_BUTTON_DPAD_UP, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (wDir & DC_DOWN )
  {
    CKey key(KEY_BUTTON_DPAD_DOWN, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }

  if (m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_BACK )
  {
    CKey key(KEY_BUTTON_BACK, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_START)
  {
    CKey key(KEY_BUTTON_START, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_LEFT_THUMB)
  {
    CKey key(KEY_BUTTON_LEFT_THUMB_BUTTON, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
  {
    CKey key(KEY_BUTTON_RIGHT_THUMB_BUTTON, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_A])
  {
    CKey key(KEY_BUTTON_A, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_B])
  {
    CKey key(KEY_BUTTON_B, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }

  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_X])
  {
    CKey key(KEY_BUTTON_X, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_Y])
  {
    CKey key(KEY_BUTTON_Y, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK])
  {
    CKey key(KEY_BUTTON_BLACK, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  if (m_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE])
  {
    CKey key(KEY_BUTTON_WHITE, bLeftTrigger, bRightTrigger, m_DefaultGamepad.fX1, m_DefaultGamepad.fY1, m_DefaultGamepad.fX2, m_DefaultGamepad.fY2, frameTime);
    if (OnKey(key))
      return true;
  }
  return false;
}

bool CInputManager::ProcessRemote(int windowId, float frameTime)
{
  if (m_DefaultIR_Remote.wButtons)
  {
    // time depends on whether the movement is repeated (held down) or not.
    // If it is, we use the FPS timer to get a repeatable speed.
    // If it isn't, we use 20 to get repeatable jumps.
    float time = m_DefaultIR_Remote.bHeldDown ? frameTime : 0.020f;
    CKey key(m_DefaultIR_Remote.wButtons, 0, 0, 0, 0, 0, 0, time);
    return OnKey(key);
  }
  return false;
}

bool CInputManager::ProcessKeyboard()
{
  BYTE vkey = m_Keyboard.GetKey();
  WCHAR unicode = m_Keyboard.GetUnicode();
  if (vkey || unicode)
  {
    // got a valid keypress - convert to a key code
    int keyID = KEY_UNICODE;
    if (vkey) // FIXME, every ascii has a vkey so vkey would always and ascii would never be processed, but fortunately OnKey uses wkeyID only to detect keyboard use and the real key is recalculated correctly.
      keyID = vkey | KEY_VKEY;

    CKey key(keyID);
    key.SetHeld(m_Keyboard.KeyHeld());
    return OnKey(key);
  }
  return false;
}

bool CInputManager::ProcessEventServer(int windowId, float frameTime)
{
  CEventServer* es = CEventServer::GetInstance();
  if (!es || !es->Running() || es->GetNumberOfClients() == 0)
    return false;

  // process any queued up actions
  if (es->ExecuteNextAction())
  {
    // reset idle timers
    CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<CApplicationPowerHandling> appPower = components.GetComponent<CApplicationPowerHandling>();
    appPower->ResetSystemIdleTimer();
    appPower->ResetScreenSaver();
    appPower->WakeUpScreenSaverAndDPMS();
  }

  // now handle any buttons or axis
  std::string strMapName;
  bool isAxis = false;
  float fAmount = 0.0;
  bool isJoystick = false;

  // es->ExecuteNextAction() invalidates the ref to the CEventServer instance
  // when the action exits XBMC
  es = CEventServer::GetInstance();
  if (!es || !es->Running() || es->GetNumberOfClients() == 0)
    return false;
  unsigned int wKeyID = es->GetButtonCode(strMapName, isAxis, fAmount, isJoystick);

  if (wKeyID)
  {
    if (strMapName.length() > 0)
    {
      // joysticks are not supported via eventserver
      if (isJoystick)
      {
        return false;
      }
      else // it is a customcontroller
      {
        int actionID;
        std::string actionName;
        bool fullRange = false;

        // Translate using custom controller translator.
        if (CButtonTranslator::GetInstance().TranslateJoystickString(
                windowId, strMapName.c_str(), wKeyID, isAxis, actionID, actionName, fullRange))
        {
          // break screensaver
          CApplicationComponents &components = CServiceBroker::GetAppComponents();
          const boost::shared_ptr<CApplicationPowerHandling> appPower = components.GetComponent<CApplicationPowerHandling>();
          appPower->ResetSystemIdleTimer();
          appPower->ResetScreenSaver();

          // in case we wokeup the screensaver or screen - eat that action...
          if (appPower->WakeUpScreenSaverAndDPMS())
            return true;

          CLog::Log(LOGDEBUG, "EventServer: key %u translated to action %s", wKeyID, actionName.c_str());

          return ExecuteInputAction(CAction(actionID, fAmount, 0.0f, actionName));
        }
        else
        {
          CLog::Log(LOGDEBUG, "ERROR mapping customcontroller action. CustomController: %s %u",
                    strMapName.c_str(), wKeyID);
        }
      }
    }
    else
    {
      CKey key;

      if (wKeyID == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
        key = CKey(wKeyID, static_cast<uint8_t>(255 * fAmount), 0, 0.0, 0.0, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
        key = CKey(wKeyID, 0, static_cast<uint8_t>(255 * fAmount), 0.0, 0.0, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_LEFT)
        key = CKey(wKeyID, 0, 0, -fAmount, 0.0, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
        key = CKey(wKeyID, 0, 0, fAmount, 0.0, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_UP)
        key = CKey(wKeyID, 0, 0, 0.0, fAmount, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
        key = CKey(wKeyID, 0, 0, 0.0, -fAmount, 0.0, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, -fAmount, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, fAmount, 0.0, frameTime);
      else if (wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_UP)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, 0.0, fAmount, frameTime);
      else if (wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, 0.0, -fAmount, frameTime);
      else
        key = CKey(wKeyID);
      return OnKey(key);
    }
  }

  return false;
}

bool CInputManager::Process(int windowId, float frameTime)
{
  // read raw input
  ReadInput();

  // process input actions
  ProcessRemote(windowId, frameTime);
  ProcessGamepad(windowId, frameTime);
  ProcessKeyboard();
  ProcessEventServer(windowId, frameTime);

  return true;
}

// OnKey() translates the key into a CAction which is sent on to our Window Manager.
// The window manager will return true if the event is processed, false otherwise.
// If not already processed, this routine handles global keypresses.  It returns
// true if the key has been processed, false otherwise.

bool CInputManager::OnKey(const CKey& key)
{
  // get the current active window
  int iWin = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog();

  // this will be checked for certain keycodes that need
  // special handling if the screensaver is active
  CAction action = CButtonTranslator::GetInstance().GetAction(iWin, key);

  // a key has been pressed.
  // reset Idle Timer
  CApplicationComponents &components = CServiceBroker::GetAppComponents();
  const boost::shared_ptr<CApplicationPowerHandling> appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->ResetSystemIdleTimer();
  bool processKey = AlwaysProcess(action);

  appPower->ResetScreenSaver();

  const boost::shared_ptr<CApplicationXbox> appXbox = components.GetComponent<CApplicationXbox>();
  appXbox->SetSpinDown(false);
  appXbox->SetNetworkSpinDown(false);

  // allow some keys to be processed while the screensaver is active
  if (appPower->WakeUpScreenSaverAndDPMS() && !processKey)
  {
    CLog::Log(LOGDEBUG, "%i pressed, screen saver/dpms woken up",
               (int)key.GetButtonCode());
    return true;
  }

  if (iWin != WINDOW_FULLSCREEN_VIDEO)
  {
    // current active window isnt the fullscreen window
    // just use corresponding section from keymap.xml
    // to map key->action

    // first determine if we should use keyboard input directly
    bool useKeyboard =
        key.FromKeyboard() && (iWin == WINDOW_DIALOG_KEYBOARD || iWin == WINDOW_DIALOG_NUMERIC);
    CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(iWin);
    if (window)
    {
      CGUIControl* control = window->GetFocusedControl();
      if (control)
      {
        // If this is an edit control set usekeyboard to true. This causes the
        // keypress to be processed directly not through the key mappings.
        if (control->GetControlType() == CGUIControl::GUICONTROL_EDIT)
          useKeyboard = true;

        // If the key pressed is shift-A to shift-Z set usekeyboard to true.
        // This causes the keypress to be used for list navigation.
        if (control->IsContainer() && m_Keyboard.GetShift() &&
            key.GetUnicode())
          useKeyboard = true;
      }
    }
    if (useKeyboard)
    {
      action = CAction(0); // reset our action
      if (!action.GetID())
      {
        if (key.GetFromHttpApi())
          action = CAction(key.GetButtonCode() != KEY_INVALID ? key.GetButtonCode() : 0,
                           key.GetUnicode());
        else
        {
          // Check for paste keypress
          if (m_Keyboard.GetKey() == XBMCVK_V && m_Keyboard.GetCtrl())
            action = CAction(ACTION_PASTE);
          // If the unicode is non-zero the keypress is a non-printing character
          else if (m_Keyboard.GetUnicode())
            action = CAction(KEY_UNICODE, m_Keyboard.GetUnicode());
          // The keypress is a non-printing character
          else
            action = CAction(m_Keyboard.GetKey() | KEY_VKEY);
        }
      }

      CLog::Log(LOGDEBUG, "%i pressed, trying keyboard action %x",
                 (int)key.GetButtonCode(), action.GetID());

      if (g_application.OnAction(action))
        return true;
      // failed to handle the keyboard action, drop down through to standard action
    }
    if (key.GetFromHttpApi())
    {
      if (key.GetButtonCode() != KEY_INVALID)
        action = CButtonTranslator::GetInstance().GetAction(iWin, key);
    }
    else
      action = CButtonTranslator::GetInstance().GetAction(iWin, key);
  }
  if (!key.IsAnalogButton())
    CLog::Log(LOGDEBUG, "%i pressed, window %i, action is %s",
               (int)key.GetButtonCode(), iWin, action.GetName().c_str());

  return ExecuteInputAction(action);
}

bool CInputManager::AlwaysProcess(const CAction& action)
{
  // check if this button is mapped to a built-in function
  if (!action.GetName().empty())
  {
    const CExecString exec(action.GetName());
    if (exec.IsValid())
    {
      const std::string builtInFunction = exec.GetFunction();

      // should this button be handled normally or just cancel the screensaver?
      if (builtInFunction == "powerdown" || builtInFunction == "reboot" ||
          builtInFunction == "restart" || builtInFunction == "restartapp" ||
          builtInFunction == "suspend" || builtInFunction == "hibernate" ||
          builtInFunction == "quit" || builtInFunction == "shutdown" ||
          builtInFunction == "volumeup" || builtInFunction == "volumedown" ||
          builtInFunction == "mute" || builtInFunction == "RunAppleScript" ||
          builtInFunction == "RunAddon" || builtInFunction == "RunPlugin" ||
          builtInFunction == "RunScript" || builtInFunction == "System.Exec" ||
          builtInFunction == "System.ExecWait")
      {
        return true;
      }
    }
  }

  return false;
}

bool CInputManager::ExecuteInputAction(const CAction &action)
{
  bool bResult = false;
  CGUIComponent* gui = CServiceBroker::GetGUI();

  // play sound before the action unless the button is held,
  // where we execute after the action as held actions aren't fired every time.
  if (action.GetHoldTime())
  {
    bResult = g_application.OnAction(action);
    if (bResult && gui)
      gui->GetAudioManager().PlayActionSound(action);
  }
  else
  {
    if (gui)
      gui->GetAudioManager().PlayActionSound(action);

    bResult = g_application.OnAction(action);
  }
  return bResult;
}
