/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIDialogBoxBase.h"
#include "IProgressCallback.h"
#include "threads/Event.h"

class CGUIDialogProgress :
      public CGUIDialogBoxBase, public IProgressCallback
{
public:
  CGUIDialogProgress(void);
  virtual ~CGUIDialogProgress(void);

  void Reset();
  void Open(const std::string &param = "");
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnBack(int actionID);
  virtual void OnWindowLoaded();
  void Progress();
  bool IsCanceled() const { return m_iChoice == CHOICE_CANCELED; }
  void SetPercentage(int iPercentage);
  int GetPercentage() const { return m_percentage; }
  void ShowProgressBar(bool bOnOff);

  void ShowChoice(int iChoice, const CVariant& label);

  static const int CHOICE_NONE = -2;
  static const int CHOICE_CANCELED = -1;
  int GetChoice() const;

  /*! \brief Wait for the progress dialog to be closed or canceled, while regularly
   rendering to allow for pointer movement or progress to be shown. Used when showing
   the progress of a process that is taking place on a separate thread and may be
   reporting progress infrequently.
   \param progresstime the time in ms to wait between rendering the dialog (defaults to 10ms)
   \return true if the dialog is closed, false if the user cancels early.
   */
  bool Wait(int progresstime = 10);

  /*! \brief Wait on an event or for the progress dialog to be canceled, while
  regularly rendering to allow for pointer movement or progress to be shown.
  \param event the CEvent to wait on.
  \return true if the event completed, false if cancelled.
  */
  bool WaitOnEvent(CEvent& event);

  // Implements IProgressCallback
  virtual void SetProgressMax(int iMax);
  virtual void SetProgressAdvance(int nSteps=1);
  virtual bool Abort();

  void SetCanCancel(bool bCanCancel);

protected:
  virtual void OnInitWindow();
  virtual int GetDefaultLabelID(int controlId) const;
  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);

  bool m_bCanCancel;

  int  m_iCurrent;
  int  m_iMax;
  int m_percentage;
  bool m_showProgress;

  bool m_supportedChoices[DIALOG_MAX_CHOICES];
  int m_iChoice;

private:
  void UpdateControls();
};
