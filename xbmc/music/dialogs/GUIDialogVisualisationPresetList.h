/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIDialog.h"

namespace ADDON
{
  class CVisualisation;
}
class CFileItemList;

class CGUIDialogVisualisationPresetList : public CGUIDialogSelect
{
public:
  CGUIDialogVisualisationPresetList();
  virtual bool OnMessage(CGUIMessage &message);

protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual void OnSelect(int idx);

private:
  void ClearVisualisation();
  void SetVisualisation(ADDON::CVisualisation *addon);
  ADDON::CVisualisation* m_viz;
};
