
#include <xtl.h>
#include "../../../xbmc/addons/include/xbmc_scr_dll.h"
#include "../../../xbmc/addons/include/xbmc_addon_cpp_dll.h"

#pragma comment (lib, "lib/xbox_dx8.lib")

extern long FXW;
extern long FXH;
int g_width;
int g_height;
LPDIRECT3DDEVICE8 g_pd3dDevice;

void DrempelsInit();
void DrempelsRender();
void DrempelsExit();

//-- Create -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  SCR_PROPS* scrprops = (SCR_PROPS*)props;

  FXW = 512;
  FXH = 512;
  g_width = scrprops->width;
  g_height = scrprops->height;
  g_pd3dDevice = (LPDIRECT3DDEVICE8)scrprops->device;

  return ADDON_STATUS_OK;
} // Create

//-- Start --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void Start()
{
	srand(::GetTickCount());
  DrempelsInit();

} // Start

//-- Render -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  DrempelsRender();

} // Render

//-- Stop ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void ADDON_Stop()
{
  DrempelsExit();

} // Stop

//-- Destroy-------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Destroy()
{
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" bool ADDON_HasSettings()
{
  return false;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
//-----------------------------------------------------------------------------

extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
//-----------------------------------------------------------------------------
extern "C" void ADDON_FreeSettings()
{
}

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_SetSetting(const char* id, const void* value)
{
  return ADDON_STATUS_UNKNOWN;
}

//-- GetInfo ------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void GetInfo(SCR_INFO* pInfo)
{
	// not used, but can be used to pass info
	// back to XBMC if required in the future
	return;
}
