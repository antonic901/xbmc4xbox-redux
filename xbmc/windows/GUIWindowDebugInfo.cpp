/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowDebugInfo.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationXbox.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlFactory.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUITextLayout.h"
#include "guilib/GUIWindowManager.h"
#include "input/ButtonTranslator.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <inttypes.h>

CGUIWindowDebugInfo::CGUIWindowDebugInfo(void)
  : CGUIDialog(WINDOW_DEBUG_INFO, "", MODELESS)
{
  m_needsScaling = false;
  m_layout = NULL;
  m_renderOrder = RENDER_ORDER_WINDOW_DEBUG;
}

CGUIWindowDebugInfo::~CGUIWindowDebugInfo(void) {}

void CGUIWindowDebugInfo::UpdateVisibility()
{
  if (LOG_LEVEL_DEBUG_FREEMEM <= CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel || g_SkinInfo->IsDebugging())
    Open();
  else
    Close();
}

bool CGUIWindowDebugInfo::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    delete m_layout;
    m_layout = NULL;
  }
  else if (message.GetMessage() == GUI_MSG_REFRESH_TIMER)
    MarkDirtyRegion();

  return CGUIDialog::OnMessage(message);
}

void CGUIWindowDebugInfo::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(), false);

  static int yShift = 20;
  static int xShift = 40;
  static unsigned int lastShift = time(NULL);
  time_t now = time(NULL);
  if (now - lastShift > 10)
  {
    yShift *= -1;
    if (now % 5 == 0)
      xShift *= -1;
    lastShift = now;
    MarkDirtyRegion();
  }

  if (!m_layout)
  {
    CGUIFont *font13 = g_fontManager.GetDefaultFont();
    CGUIFont *font13border = g_fontManager.GetDefaultFont(true);
    if (font13)
      m_layout = new CGUITextLayout(font13, true, 0, font13border);
  }
  if (!m_layout)
    return;

  std::string info;
  if (LOG_LEVEL_DEBUG_FREEMEM <= CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel)
  {
    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);
    info = StringUtils::Format("LOG: %s%s.log\nMEM: %d/%d KB - FPS: %2.1f fps\nCPU: %2.0f%%",
                               CSpecialProtocol::TranslatePath("special://logpath").c_str(), "xodi",
                               stat.dwAvailPhys / 1024, stat.dwTotalPhys / 1024,
                               CServiceBroker::GetGUI()
                                   ->GetInfoManager()
                                   .GetInfoProviders()
                                   .GetSystemInfoProvider()
                                   .GetFPS(),
                               CServiceBroker::GetAppComponents().GetComponent<CApplicationXbox>()->GetCPUUsage());
  }

  // render the skin debug info
  if (g_SkinInfo->IsDebugging())
  {
    if (!info.empty())
      info += "\n";
    CGUIWindow *window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
    if (window)
    {
      std::string windowName = CButtonTranslator::TranslateWindow(window->GetID());
      if (!windowName.empty())
        windowName += " (" + window->GetProperty("xmlfile").asString() + ")";
      else
        windowName = window->GetProperty("xmlfile").asString();
      info += "Window: " + windowName + "\n";
    }
    if (window)
    {
      CGUIControl *control = window->GetFocusedControl();
      if (control)
        info += StringUtils::Format(
            "Focused: %i (%s)", control->GetID(),
            CGUIControlFactory::TranslateControlType(control->GetControlType()).c_str());
    }
  }

  float w, h;
  if (m_layout->Update(info))
    MarkDirtyRegion();
  m_layout->GetTextExtent(w, h);

  float x = xShift + 0.04f * CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
  float y = yShift + 0.04f * CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight();
  m_renderRegion.SetRect(x, y, x+w, y+h);
}

void CGUIWindowDebugInfo::Render()
{
  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(), false);
  if (m_layout)
    m_layout->RenderOutline(m_renderRegion.x1, m_renderRegion.y1, 0xffffffff, 0xff000000, 0, 0);
}
