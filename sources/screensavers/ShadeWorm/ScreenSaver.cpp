/*
* Shade Worm Screen Saver for XBox Media Center
* Copyright (c) 2005 MrC
*
* Ver 1.0 27 Feb 2005	MrC
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "ShadeWorm.h"
#include "../../../xbmc/addons/include/xbmc_scr_dll.h"
#include "../../../xbmc/addons/include/xbmc_addon_cpp_dll.h"

#pragma comment (lib, "lib/xbox_dx8.lib")

ShadeWorm_c*		g_shadeWorm = NULL;

//-- Create -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  SCR_PROPS* scrprops = (SCR_PROPS*)props;

	g_shadeWorm = new ShadeWorm_c;
	g_shadeWorm->Create((LPDIRECT3DDEVICE8)scrprops->device, scrprops->width, scrprops->height, scrprops->name);

  return ADDON_STATUS_OK;
} // Create

//-- Start --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void Start()
{
	srand(::GetTickCount());

	if (g_shadeWorm)
	{
		if (!g_shadeWorm->Start())
		{
			// Creation failure
			g_shadeWorm->Stop();
			delete g_shadeWorm;
			g_shadeWorm = NULL;
		}
	}

} // Start

//-- Render -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void Render()
{
	if (g_shadeWorm)
		g_shadeWorm->Render();

} // Render

//-- Stop ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void ADDON_Stop()
{
	if (g_shadeWorm)
	{
		g_shadeWorm->Stop();
		delete g_shadeWorm;
		g_shadeWorm = NULL;
	}

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
