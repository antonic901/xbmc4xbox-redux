//-----------------------------------------------------------------------------
// File: XBApplicationEx.cpp
//
// Desc: Application class for the XBox samples.
//
// Hist: 11.01.00 - New for November XDK release
//       12.15.00 - Changes for December XDK release
//       12.19.01 - Changes for March XDK release
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "system.h"
#include "XBApplicationEx.h"
#include "XBVideoConfig.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

//-----------------------------------------------------------------------------
// Global access to common members
//-----------------------------------------------------------------------------
CXBApplicationEx* g_pXBApp = NULL;
static LPDIRECT3DDEVICE8 g_pd3dDevice = NULL;

// Deadzone for the gamepad inputs
const SHORT XINPUT_DEADZONE = (SHORT)( 0.24f * FLOAT(0x7FFF) );




//-----------------------------------------------------------------------------
// Name: CXBApplication()
// Desc: Constructor
//-----------------------------------------------------------------------------
CXBApplicationEx::CXBApplicationEx()
{
  // Initialize member variables
  g_pXBApp = this;

  // Direct3D variables
  m_pD3D = NULL;
  m_pd3dDevice = NULL;
  //    m_pDepthBuffer    = NULL;
  m_pBackBuffer = NULL;

  // Variables to perform app timing
  m_bStop = false;

  // Set up the presentation parameters for a double-buffered, 640x480,
  // 32-bit display using depth-stencil. Override these parameters in
  // your derived class as your app requires.
  ZeroMemory( &m_d3dpp, sizeof(m_d3dpp) );
  m_d3dpp.BackBufferWidth = 720;
  m_d3dpp.BackBufferHeight = 576;
  m_d3dpp.BackBufferFormat = D3DFMT_LIN_A8R8G8B8;
  m_d3dpp.BackBufferCount = 1;
  m_d3dpp.EnableAutoDepthStencil = FALSE;
  m_d3dpp.AutoDepthStencilFormat = D3DFMT_LIN_D16;
  m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  m_d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

  // Specify number and type of input devices this app will be using. By
  // default, you can use 0 and NULL, which triggers XInputDevices() to
  // pre-alloc the default number and types of devices. To use chat or
  // other devices, override these variables in your derived class.
#ifdef HAS_XBOX_HARDWARE
  m_dwNumInputDeviceTypes = 0;
  m_InputDeviceTypes = NULL;
#endif
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Create the app
//-----------------------------------------------------------------------------
HRESULT CXBApplicationEx::Create(HWND hWnd)
{
  HRESULT hr;

  // Initialize the app's device-dependent objects
  if ( FAILED( hr = Initialize() ) )
  {
    CLog::Log(LOGERROR, "XBAppEx: Call to Initialize() failed!" );
    return hr;
  }

  return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Destroy()
// Desc: Cleanup objects
//-----------------------------------------------------------------------------
VOID CXBApplicationEx::Destroy()
{
  // Perform app-specific cleanup
  Cleanup();

  // Release display objects
  SAFE_RELEASE( m_pd3dDevice );
  SAFE_RELEASE( m_pD3D );
}




//-----------------------------------------------------------------------------
// Name: Run()
// Desc:
//-----------------------------------------------------------------------------
INT CXBApplicationEx::Run()
{
  CLog::Log(LOGNOTICE, "Running the application..." );

  // Run the game loop, animating and rendering frames
  while (!m_bStop)
  {

    //-----------------------------------------
    // Animate and render a frame
    //-----------------------------------------
#ifndef _DEBUG
    try
    {
#endif
      Process();

#ifndef _DEBUG

    }
    catch (...)
    {
      CLog::Log(LOGERROR, "exception in CApplication::Process()");
    }
#endif
    // Frame move the scene
#ifndef _DEBUG
    try
    {
#endif
      FrameMove(true);

#ifndef _DEBUG

    }
    catch (...)
    {
      CLog::Log(LOGERROR, "exception in CApplication::FrameMove()");
    }
#endif

    // Render the scene
#ifndef _DEBUG
    try
    {
#endif
      Render();

#ifndef _DEBUG

    }
    catch (...)
    {
      CLog::Log(LOGERROR, "exception in CApplication::Render()");
    }
#endif

  }
  Destroy();

  CLog::Log(LOGNOTICE, "application stopped..." );
  return 0;
}




inline float DeadZone(float &f)
{
  if (f > CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_controllerDeadzone)
    return (f - CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_controllerDeadzone)/(1.0f - CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_controllerDeadzone);
  else if (f < -CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_controllerDeadzone)
    return (f + CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_controllerDeadzone)/(1.0f - CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_controllerDeadzone);
  else
    return 0.0f;
}

#ifdef HAS_GAMEPAD
inline float MaxTrigger(XBGAMEPAD &gamepad)
{
  float max = fabs(gamepad.fX1);
  if (fabs(gamepad.fX2) > max) max = fabs(gamepad.fX2);
  if (fabs(gamepad.fY1) > max) max = fabs(gamepad.fY1);
  if (fabs(gamepad.fY2) > max) max = fabs(gamepad.fY2);
  return max;
}
#endif
void CXBApplicationEx::ReadInput()
{
  //-----------------------------------------
  // Handle input
  //-----------------------------------------

  // Read the input from the IR remote
#ifdef HAS_IR_REMOTE
  XBInput_GetInput( m_IR_Remote );
  ZeroMemory( &m_DefaultIR_Remote, sizeof(m_DefaultIR_Remote) );

  for ( DWORD i = 0; i < 4; i++ )
  {
    if ( m_IR_Remote[i].hDevice)
    {
      m_DefaultIR_Remote.wButtons = m_IR_Remote[i].wButtons;
    }
  }
#endif

  // Read the input from the keyboard
  g_Keyboard.Update();

#ifdef HAS_GAMEPAD
  // Read the input for all connected gampads
  XBInput_GetInput( m_Gamepad );

  // Lump inputs of all connected gamepads into one common structure.
  // This is done so apps that need only one gamepad can function with
  // any gamepad.
  ZeroMemory( &m_DefaultGamepad, sizeof(m_DefaultGamepad) );

  float maxTrigger = 0.0f;
  for ( DWORD i = 0; i < 4; i++ )
  {
    if ( m_Gamepad[i].hDevice )
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

      for ( DWORD b = 0; b < 8; b++ )
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
#endif
}

void CXBApplicationEx::Process()
{}
