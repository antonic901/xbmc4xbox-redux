/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"
#include "SortFileItem.h"
#include "addons/IAddon.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "threads/Atomics.h"

#include <map>
#include <string>

#include "PlatformDefs.h"

class CURL;
class CFileItem;
class CFileItemList;

namespace XFILE
{

class CPluginDirectory : public IDirectory
{
public:
  CPluginDirectory();
  ~CPluginDirectory(void);
  virtual bool GetDirectory(const CURL& url, CFileItemList& items);
  virtual bool AllowAll() const { return true; }
  virtual bool Exists(const CURL& url) { return true; }
  virtual float GetProgress() const;
  virtual void CancelDirectory();
  static bool RunScriptWithParams(const std::string& strPath, bool resume);
  static bool GetPluginResult(const std::string& strPath, CFileItem &resultItem, bool resume);

  // callbacks from python
  static bool AddItem(int handle, const CFileItem *item, int totalItems);
  static bool AddItems(int handle, const CFileItemList *items, int totalItems);
  static void EndOfDirectory(int handle, bool success, bool replaceListing, bool cacheToDisc);
  static void AddSortMethod(int handle, SORT_METHOD sortMethod, const std::string &label2Mask);
  static std::string GetSetting(int handle, const std::string &key);
  static void SetSetting(int handle, const std::string &key, const std::string &value);
  static void SetContent(int handle, const std::string &strContent);
  static void SetProperty(int handle, const std::string &strProperty, const std::string &strValue);
  static void SetResolvedUrl(int handle, bool success, const CFileItem* resultItem);
  static void SetLabel2(int handle, const std::string& ident);

private:
  ADDON::AddonPtr m_addon;
  bool StartScript(const std::string& strPath, bool retrievingDir, bool resume);
  bool WaitOnScriptResult(const std::string &scriptPath, int scriptId, const std::string &scriptName, bool retrievingDir);

  static std::map<int,CPluginDirectory*> globalHandles;
  static int getNewHandle(CPluginDirectory *cp);
  static void reuseHandle(int handle, CPluginDirectory *cp);

  static void removeHandle(int handle);
  static CPluginDirectory *dirFromHandle(int handle);
  static CCriticalSection m_handleLock;
  static int handleCounter;

  CFileItemList* m_listItems;
  CFileItem*     m_fileResult;
  CEvent         m_fetchComplete;

  atomic<bool> m_cancelled;
  bool          m_success;      // set by script in EndOfDirectory
  int    m_totalItems;   // set by script in AddDirectoryItem

  class CScriptObserver : public CThread
  {
  public:
    CScriptObserver(int scriptId, CEvent &event);
    void Abort();
  protected:
    virtual void Process();
    int m_scriptId;
    CEvent &m_event;
  };
};
}
