/*
 * Pyro Screensaver for XBox Media Center
 * Copyright (c) 2004 Team XBMC
 *
 * Ver 1.0 15 Nov 2004	Chris Barnett (Forza)
 *
 * Adapted from the Pyro screen saver by
 *
 *  Jamie Zawinski <jwz@jwz.org>
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

#include "Stars.h"
#include "StarField.h"
#include "XmlDocument.h"
#include "../../../xbmc/addons/include/xbmc_scr_dll.h"
#include "../../../xbmc/addons/include/xbmc_addon_cpp_dll.h"

#include <stdio.h>
#include <math.h>

#define CONFIG_FILE "special://home/addons/screensaver.stars/config.xml"

#pragma comment (lib, "lib/xbox_dx8.lib" )

CStarField* g_pStarField = NULL;

struct ST_SETTINGS
{
	char szScrName[1024];
	int iWidth;
	int iHeight;
	
	int   iNumStars;
	float fGamma;
	float fBrightness;
	float fSpeed;
	float fZoom;
	float fExpanse;
};

struct ST_SETTINGS g_Settings = 
{
	"", 0, 0, 1000, 1.f, 0.2f, 10.0f, 1.5f, 1.5f
};

LPDIRECT3DDEVICE8 g_pd3dDevice;


//////////////////////////////////////////////////////////////////////
// This is a quick and dirty hack to show a simple screensaver ...
//////////////////////////////////////////////////////////////////////

extern "C" ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  SCR_PROPS* scrprops = (SCR_PROPS*)props;

	g_Settings.iWidth = scrprops->width;
	g_Settings.iHeight = scrprops->height;
	strcpy(g_Settings.szScrName, scrprops->name);
	
	g_pd3dDevice = (LPDIRECT3DDEVICE8)scrprops->device;

	LoadSettings();

  return ADDON_STATUS_OK;
}

extern "C" void Start()
{
	srand(::GetTickCount());

	g_pStarField = new CStarField(g_Settings.iNumStars,
								  g_Settings.fGamma,
								  g_Settings.fBrightness,
								  g_Settings.fSpeed,
								  g_Settings.fZoom,
								  g_Settings.fExpanse);
	if (g_pStarField)
	{
		g_pStarField->SetD3DDevice(g_pd3dDevice);
		g_pStarField->Create(g_Settings.iWidth, g_Settings.iHeight);
	}

	return;
}

extern "C" void Render()
{	
	if (g_pStarField)
	{
		g_pStarField->RenderFrame();
	}
}

extern "C" void ADDON_Stop()
{
	delete g_pStarField;
	g_pStarField = NULL;
}

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

void LoadSettings(void)
{
	XmlNode node, childNode;
	CXmlDocument doc;

	char szXMLFile[1024];
  strcpy(szXMLFile, CONFIG_FILE);

	// Load the config file
	if (doc.Load(szXMLFile) >= 0)
	{
		node = doc.GetNextNode(XML_ROOT_NODE);

		while(node > 0)
		{
			if (strcmpi(doc.GetNodeTag(node), "screensaver"))
			{
				node = doc.GetNextNode(node);
				continue;
			}

			if (childNode = doc.GetChildNode(node, "NumStars"))
			{
				g_Settings.iNumStars = atoi(doc.GetNodeText(childNode));
			}

			if (childNode = doc.GetChildNode(node, "Gamma"))
			{
				g_Settings.fGamma = (float)atof(doc.GetNodeText(childNode));
			}

			if (childNode = doc.GetChildNode(node, "Brightness"))
			{
				g_Settings.fBrightness = (float)atof(doc.GetNodeText(childNode));
			}

			if (childNode = doc.GetChildNode(node, "Speed"))
			{
				g_Settings.fSpeed = (float)atof(doc.GetNodeText(childNode));
			}
			
			if (childNode = doc.GetChildNode(node, "Zoom"))
			{
				g_Settings.fZoom = (float)atof(doc.GetNodeText(childNode));
			}
	
			if (childNode = doc.GetChildNode(node, "Expanse"))
			{
				g_Settings.fExpanse = (float)atof(doc.GetNodeText(childNode));
			}
				
			node = doc.GetNextNode(node);
		}
		doc.Close();
	}
}

extern "C" void GetInfo(SCR_INFO* pInfo)
{
	return;
}
