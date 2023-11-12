////////////////////////////////////////////////////////////////////////////
//
// Planestate Screensaver for XBox Media Center
// Copyright (c) 2005 Joakim Eriksson <je@plane9.com>
//
// Thanks goes to Warren for his 'TestXBS' program!
//
////////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "planestate.h"
#include "XmlDocument.h"
#include "../../../xbmc/addons/include/xbmc_scr_dll.h"
#include "../../../xbmc/addons/include/xbmc_addon_cpp_dll.h"
#include "timer.h"
#include <time.h>

#define CONFIG_FILE "special://home/addons/screensaver.planestate/config.xml"

static char gScrName[1024];

CPlanestate*	gPlanestate = null;
CRenderD3D		gRender;
CTimer*			gTimer = null;
f32				gCfgProbability[NUMCFGS] = { 0.35f, 0.35f,0.15f, 0.15f };	// The probability that we pick a specific configuration. Should sum up to 1.0

////////////////////////////////////////////////////////////////////////////
// XBMC has loaded us into memory, we should set our core values
// here and load any settings we may have from our config file
//
extern "C" ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  SCR_PROPS* scrprops = (SCR_PROPS*)props;

	strcpy(gScrName,scrprops->name);
	LoadSettings();

	gRender.m_D3dDevice = (LPDIRECT3DDEVICE8)scrprops->device;
	gRender.m_Width	= scrprops->width;
	gRender.m_Height= scrprops->height;

	gPlanestate = null;
	gTimer = null;

  return ADDON_STATUS_OK;
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us we should get ready to start rendering. This function
// is called once when the screensaver is activated by XBMC.
//
extern "C" void Start()
{
	srand((u32)time(null));
	gPlanestate = new CPlanestate(gCfgProbability);
	if (!gPlanestate)
		return;
	gTimer = new CTimer();
	gTimer->Init();
	if (!gPlanestate->RestoreDevice(&gRender))
		ADDON_Stop();
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us to render a frame of our screensaver. This is called on
// each frame render in XBMC, you should render a single frame only - the DX
// device will already have been cleared.
//
extern "C" void Render()
{
	if (!gPlanestate)
		return;
	gTimer->Update();
	gPlanestate->Update(gTimer->GetDeltaTime());
	gPlanestate->Draw(&gRender);
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us to stop the screensaver we should free any memory and release
// any resources we have created.
//
extern "C" void ADDON_Stop()
{
	if (gPlanestate)
		gPlanestate->InvalidateDevice(&gRender);
	SAFE_DELETE( gPlanestate );
	SAFE_DELETE( gTimer );

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

////////////////////////////////////////////////////////////////////////////
// Load settings from the [screensavername].xml configuration file
// the name of the screensaver (filename) is used as the name of
// the xml file - this is sent to us by XBMC when the Init func is called.
//
void LoadSettings()
{
	XmlNode node, childNode; //, grandChild;
	CXmlDocument doc;
	
	// Set up the defaults
	SetDefaults();

	char szXMLFile[1024];
  strcpy(szXMLFile, CONFIG_FILE);

	// Load the config file
	if (doc.Load(szXMLFile) >= 0)
	{
		node = doc.GetNextNode(XML_ROOT_NODE);
		while(node > 0)
		{
			if (strcmpi(doc.GetNodeTag(node),"screensaver"))
			{
				node = doc.GetNextNode(node);
				continue;
			}

			if (childNode = doc.GetChildNode(node,"CfgProbability1"))	gCfgProbability[0] = (f32)atof(doc.GetNodeText(childNode));
			if (childNode = doc.GetChildNode(node,"CfgProbability2"))	gCfgProbability[1] = (f32)atof(doc.GetNodeText(childNode));
			if (childNode = doc.GetChildNode(node,"CfgProbability3"))	gCfgProbability[2] = (f32)atof(doc.GetNodeText(childNode));
			if (childNode = doc.GetChildNode(node,"CfgProbability4"))	gCfgProbability[3] = (f32)atof(doc.GetNodeText(childNode));

			node = doc.GetNextNode(node);
		}
		doc.Close();
	}
}

////////////////////////////////////////////////////////////////////////////
// set any default values for your screensaver's parameters
//
void SetDefaults()
{
	return;
}

////////////////////////////////////////////////////////////////////////////
// not used, but can be used to pass info back to XBMC if required in the future
//
extern "C" void GetInfo(SCR_INFO* pInfo)
{
	return;
}
