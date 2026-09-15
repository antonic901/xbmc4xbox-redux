#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

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
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual void OnSelect(int idx);

private:
  void SetVisualisation(ADDON::CVisualisation* addon);
  ADDON::CVisualisation* m_viz;
};
