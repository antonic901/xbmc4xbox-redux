////////////////////////////////////////////////////////////////////////////
//
// PingPong Screensaver for XBox Media Center
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
#include "pingpong.h"
#include "XmlDocument.h"
#include "../../../xbmc/addons/include/xbmc_scr_dll.h"
#include "../../../xbmc/addons/include/xbmc_addon_cpp_dll.h"
#include "timer.h"
#include <time.h>

#define CONFIG_FILE "special://home/addons/screensaver.pingpong/config.xml"

static char gScrName[1024];

CPingPong*		gPingPong = null;
CRenderD3D		gRender;
CTimer*			gTimer = null;
CRGBA			gCol[3];

extern "C" void ADDON_Stop();

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

  return ADDON_STATUS_OK;
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us we should get ready to start rendering. This function
// is called once when the screensaver is activated by XBMC.
//
extern "C" void Start()
{
	srand((u32)time(null));
	gPingPong = new CPingPong();
	if (!gPingPong)
		return;

	gPingPong->m_Paddle[0].m_Col = gCol[0];
	gPingPong->m_Paddle[1].m_Col = gCol[1];
	gPingPong->m_Ball.m_Col = gCol[2];

	gTimer = new CTimer();
	gTimer->Init();
	if (!gPingPong->RestoreDevice(&gRender))
		ADDON_Stop();
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us to render a frame of our screensaver. This is called on
// each frame render in XBMC, you should render a single frame only - the DX
// device will already have been cleared.
//
extern "C" void Render()
{
	if (!gPingPong)
		return;
	gTimer->Update();
	gPingPong->Update(gTimer->GetDeltaTime());
	gPingPong->Draw(&gRender);
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us to stop the screensaver we should free any memory and release
// any resources we have created.
//
extern "C" void ADDON_Stop()
{
	if (!gPingPong)
		return;
	gPingPong->InvalidateDevice(&gRender);
	SAFE_DELETE(gPingPong);
	SAFE_DELETE(gTimer);
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

			if (childNode = doc.GetChildNode(node,"ColorPaddle1"))	sscanf(doc.GetNodeText(childNode), "%f %f %f", &gCol[0].r, &gCol[0].g, &gCol[0].b);
			if (childNode = doc.GetChildNode(node,"ColorPaddle2"))	sscanf(doc.GetNodeText(childNode), "%f %f %f", &gCol[1].r, &gCol[1].g, &gCol[1].b);
			if (childNode = doc.GetChildNode(node,"ColorBall"))		sscanf(doc.GetNodeText(childNode), "%f %f %f", &gCol[2].r, &gCol[2].g, &gCol[2].b);

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
	for (int i=0; i<3; i++)
		gCol[i].Set(1.0f, 1.0f, 1.0f, 1.0f);
	return;
}

////////////////////////////////////////////////////////////////////////////
// not used, but can be used to pass info back to XBMC if required in the future
//
extern "C" void GetInfo(SCR_INFO* pInfo)
{
	return;
}
