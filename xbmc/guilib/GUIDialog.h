/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIDialog.h
\brief
*/

#include "GUIWindow.h"
#include "WindowIDs.h"

#ifdef TARGET_WINDOWS_STORE
#pragma pack(push, 8)
#endif
enum DialogModalityType
{
  MODELESS,
  MODAL
};
#ifdef TARGET_WINDOWS_STORE
#pragma pack(pop)
#endif

/*!
 \ingroup winmsg
 \brief
 */
class CGUIDialog :
      public CGUIWindow
{
public:
  CGUIDialog(int id, const std::string &xmlFile, DialogModalityType modalityType = MODAL);
  virtual ~CGUIDialog(void);

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();

  void Open(const std::string &param = "");
  void Open(bool bProcessRenderLoop, const std::string& param = "");

  virtual bool OnBack(int actionID);

  virtual bool IsDialogRunning() const { return m_active; }
  virtual bool IsDialog() const { return true; }
  virtual bool IsModalDialog() const { return m_modalityType == MODAL; }
  virtual DialogModalityType GetModalityType() const { return m_modalityType; }

  void SetAutoClose(unsigned int timeoutMs);
  void ResetAutoClose(void);
  void CancelAutoClose(void);
  bool IsAutoClosed(void) const { return m_bAutoClosed; }
  void SetSound(bool OnOff) { m_enableSound = OnOff; }
  virtual bool IsSoundEnabled() const { return m_enableSound; }

protected:
  virtual bool Load(TiXmlElement *pRootElement);
  virtual void SetDefaults();
  virtual void OnWindowLoaded();
  using CGUIWindow::UpdateVisibility;
  virtual void UpdateVisibility();

  virtual void Open_Internal(bool bProcessRenderLoop, const std::string &param = "");
  virtual void OnDeinitWindow(int nextWindowID);

  void ProcessRenderLoop(bool renderOnly = false);

  bool m_wasRunning; ///< \brief true if we were running during the last DoProcess()
  bool m_autoClosing;
  bool m_enableSound;
  unsigned int m_showStartTime;
  unsigned int m_showDuration;
  bool m_bAutoClosed;
  DialogModalityType m_modalityType;
};
