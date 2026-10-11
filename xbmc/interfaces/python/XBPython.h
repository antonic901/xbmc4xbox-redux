/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/IPlayerCallback.h"
#include "interfaces/IAnnouncer.h"
#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"

#include <memory>
#include <vector>

class CPythonInvoker;
class CVariant;

typedef struct
{
  int id;
  bool bDone;
  CPythonInvoker* pyThread;
} PyElem;

class LibraryLoader;

namespace XBMCAddon
{
namespace xbmc
{
class Monitor;
}
} // namespace XBMCAddon

template<class T>
struct LockableType : public T, public CCriticalSection
{
  bool hadSomethingRemoved;
};

typedef LockableType<std::vector<void*> > PlayerCallbackList;
typedef LockableType<std::vector<XBMCAddon::xbmc::Monitor*> > MonitorCallbackList;
typedef LockableType<std::vector<PyElem> > PyList;
typedef std::vector<LibraryLoader*> PythonExtensionLibraries;

class XBPython : public IPlayerCallback,
                 public ANNOUNCEMENT::IAnnouncer,
                 public ILanguageInvocationHandler
{
public:
  XBPython();
  virtual ~XBPython();
  virtual void OnPlayBackEnded();
  virtual void OnPlayBackStarted(const CFileItem& file);
  virtual void OnPlayBackPaused();
  virtual void OnPlayBackResumed();
  virtual void OnPlayBackStopped();
  virtual void OnPlayBackError();
  virtual void OnPlayBackSpeedChanged(int iSpeed);
  virtual void OnPlayBackSeek(int64_t iTime, int64_t seekOffset);
  virtual void OnPlayBackSeekChapter(int iChapter);
  virtual void OnQueueNextItem();

  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data);
  void RegisterPythonPlayerCallBack(IPlayerCallback* pCallback);
  void UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback);
  void RegisterPythonMonitorCallBack(XBMCAddon::xbmc::Monitor* pCallback);
  void UnregisterPythonMonitorCallBack(XBMCAddon::xbmc::Monitor* pCallback);
  void OnSettingsChanged(const std::string& strings);
  void OnScreensaverActivated();
  void OnScreensaverDeactivated();
  void OnDPMSActivated();
  void OnDPMSDeactivated();
  void OnScanStarted(const std::string& library);
  void OnScanFinished(const std::string& library);
  void OnCleanStarted(const std::string& library);
  void OnCleanFinished(const std::string& library);
  void OnNotification(const std::string& sender,
                      const std::string& method,
                      const std::string& data);

  virtual void Process();
  virtual void PulseGlobalEvent();
  virtual void Uninitialize();
  virtual bool OnScriptInitialized(ILanguageInvoker* invoker);
  virtual void OnScriptStarted(ILanguageInvoker* invoker);
  virtual void NotifyScriptAborting(ILanguageInvoker* invoker);
  virtual void OnExecutionEnded(ILanguageInvoker* invoker);
  virtual void OnScriptFinalized(ILanguageInvoker* invoker);
  virtual ILanguageInvoker* CreateInvoker();

  bool WaitForEvent(CEvent& hEvent, unsigned int milliseconds);

private:
#ifdef _XBOX
  void UnloadExtensionLibs();
  void Finalize();

  unsigned int m_endtime;
  LibraryLoader* m_pDll;
#endif

  static bool m_bInitialized; // whether global python runtime was already initialized

  CCriticalSection m_critSection;
  void* m_mainThreadState;
  int m_iDllScriptCounter; // to keep track of the total scripts running that need the dll

  //Vector with list of threads used for running scripts
  PyList m_vecPyList;
  PlayerCallbackList m_vecPlayerCallbackList;
  MonitorCallbackList m_vecMonitorCallbackList;

  // any global events that scripts should be using
  CEvent m_globalEvent;

  // in order to finalize and unload the python library, need to save all the extension libraries that are
  // loaded by it and unload them first (not done by finalize)
  PythonExtensionLibraries m_extensions;
};
