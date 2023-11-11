#ifndef __XBMC_ADDON_H__
#define __XBMC_ADDON_H__

#ifdef _XBOX
#include <xbox.h>
#elif defined _WIN32
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

#include "xbmc_addon_types.h"

#ifdef __cplusplus
extern "C" { 
#endif

  ADDON_STATUS __declspec(dllexport) ADDON_Create(void *callbacks, void* props);
  void         __declspec(dllexport) ADDON_Stop();
  void         __declspec(dllexport) ADDON_Destroy();
  ADDON_STATUS __declspec(dllexport) ADDON_GetStatus();
  bool         __declspec(dllexport) ADDON_HasSettings();
  unsigned int __declspec(dllexport) ADDON_GetSettings(ADDON_StructSetting ***sSet);
  ADDON_STATUS __declspec(dllexport) ADDON_SetSetting(const char *settingName, const void *settingValue);
  void         __declspec(dllexport) ADDON_FreeSettings();

#ifdef __cplusplus
};
#endif

#endif
