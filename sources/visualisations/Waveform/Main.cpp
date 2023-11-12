// Waveform.vis
// A simple visualisation example by MrC

#include <xtl.h>
#include "../../../xbmc/addons/include/xbmc_vis_dll.h"

#pragma comment (lib, "lib/xbox_dx8.lib" )

// exports for d3d hacks
extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);

LPDIRECT3DDEVICE8 g_device;
float g_fWaveform[2][512];
D3DVIEWPORT8  g_viewport;

struct Vertex_t
{
  float x, y, z, w;
  D3DCOLOR  col;
};

#define VERTEX_FORMAT     (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)


//-- Create -------------------------------------------------------------------
// Called once when the visualisation is created by XBMC. Do any setup here.
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  VIS_PROPS* visprops = (VIS_PROPS*)props;

  g_device = (LPDIRECT3DDEVICE8)visprops->device;;
  g_viewport.X = visprops->x;
  g_viewport.Y = visprops->y;
  g_viewport.Width = visprops->width;
  g_viewport.Height = visprops->height;
  g_viewport.MinZ = 0;
  g_viewport.MaxZ = 1;

  return ADDON_STATUS_OK;
}

//-- Start --------------------------------------------------------------------
// Called when a new soundtrack is played
//-----------------------------------------------------------------------------
extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{

}

//-- Stop ---------------------------------------------------------------------
// Called when the visualisation is closed by XBMC
//-----------------------------------------------------------------------------
extern "C" void ADDON_Stop()
{

}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
extern "C" void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{

  // Convert the audio data into a floating -1 to +1 range
  int ipos=0;
  while (ipos < 512)
  {
    for (int i=0; i < iAudioDataLength; i+=2)
    {
      g_fWaveform[0][ipos] = pAudioData[i] / 32768.0f;    // left channel
      g_fWaveform[1][ipos] = pAudioData[i+1] / 32768.0f;  // right channel
      ipos++;
      if (ipos >= 512) break;
    }
  }
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  Vertex_t  verts[512];

  g_device->SetVertexShader(VERTEX_FORMAT);
  g_device->SetPixelShader(NULL);

  // Left channel
  for (int i = 0; i < 256; i++)
  {
    verts[i].col = 0xffffffff;
    verts[i].x = g_viewport.X + ((i / 255.0f) * g_viewport.Width);
    verts[i].y = g_viewport.Y + g_viewport.Height * 0.33f + (g_fWaveform[0][i] * g_viewport.Height * 0.15f);
    verts[i].z = 1;
    verts[i].w = 1;
  }

  g_device->DrawPrimitiveUP(D3DPT_LINESTRIP, 255, verts, sizeof(Vertex_t));

  // Right channel
  for (int i = 0; i < 256; i++)
  {
    verts[i].col = 0xffffffff;
    verts[i].x = g_viewport.X + ((i / 255.0f) * g_viewport.Width);
    verts[i].y = g_viewport.Y + g_viewport.Height * 0.66f + (g_fWaveform[1][i] * g_viewport.Height * 0.15f);
    verts[i].z = 1;
    verts[i].w = 1;
  }

  g_device->DrawPrimitiveUP(D3DPT_LINESTRIP, 255, verts, sizeof(Vertex_t));

}

//-- GetInfo ------------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
extern "C" void GetInfo(VIS_INFO* pInfo)
{
  pInfo->bWantsFreq = false;
  pInfo->iSyncDelay = 0;
}

//-- OnAction -----------------------------------------------------------------
// Handle XBMC actions such as next preset, lock preset, album art changed etc
//-----------------------------------------------------------------------------
extern "C" bool OnAction(long flags, const void *param)
{
  return false;
}

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to XBMC for display
//-----------------------------------------------------------------------------
extern "C" unsigned int GetPresets(char ***presets)
{
  return 0;
}

//-- GetPreset ----------------------------------------------------------------
// Return the index of the current playing preset
//-----------------------------------------------------------------------------
extern "C" unsigned GetPreset()
{
  return 0;
}

//-- IsLocked -----------------------------------------------------------------
// Returns true if this add-on use settings
//-----------------------------------------------------------------------------
extern "C" bool IsLocked()
{
  return false;
}

//-- Destroy-------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Destroy()
{
  ADDON_Stop();
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
{}

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_SetSetting(const char* id, const void* value)
{
  return ADDON_STATUS_OK;
}

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSubModules(char ***names)
{
  return 0; // this vis supports 0 sub modules
}
