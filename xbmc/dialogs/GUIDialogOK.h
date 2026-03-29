/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIDialogBoxBase.h"
#include "messaging/helpers/DialogOKHelper.h"

class CGUIMessage;
class CVariant;

using namespace KODI::MESSAGING;

class CGUIDialogOK :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogOK(void);
  virtual ~CGUIDialogOK(void);
  virtual bool OnMessage(CGUIMessage& message);
  static bool ShowAndGetInput(const CVariant& heading, const CVariant& text);
  static bool ShowAndGetInput(const CVariant& heading,
                              const CVariant& line0,
                              const CVariant& line1,
                              const CVariant& line2);
  /*!
  \brief Open a OK dialog and wait for input

  \param[in] options  a struct of type DialogOKMessage containing
  the options to set for this dialog.

  \sa KODI::MESSAGING::HELPERS::DialogOKMessage
  */
  bool ShowAndGetInput(const HELPERS::DialogOKMessage& options);
protected:
  virtual void OnInitWindow();
  virtual int GetDefaultLabelID(int controlId) const;
};
