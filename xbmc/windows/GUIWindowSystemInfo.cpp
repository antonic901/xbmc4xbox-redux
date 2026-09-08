/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowSystemInfo.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"

const static int CONTROL_TEXT_START = 2;
const static int CONTROL_TEXT_END = 13; // 12 lines

#define CONTROL_BT_HDD      92
#define CONTROL_BT_DVD      93
#define CONTROL_BT_STORAGE  94
#define CONTROL_BT_DEFAULT  95
#define CONTROL_BT_NETWORK  96
#define CONTROL_BT_VIDEO    97
#define CONTROL_BT_HARDWARE 98

const static int CONTROL_GROUP_SYSTEM_BAR = 104;

const static int CONTROL_START = CONTROL_BT_HDD;
const static int CONTROL_END = CONTROL_BT_HARDWARE;

CGUIWindowSystemInfo::CGUIWindowSystemInfo(void) :
    CGUIWindow(WINDOW_SYSTEM_INFORMATION, "SettingsSystemInfo.xml")
{
  m_section = CONTROL_BT_DEFAULT;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowSystemInfo::~CGUIWindowSystemInfo(void) {}

bool CGUIWindowSystemInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      SET_CONTROL_LABEL(52, "Xodi " + StringUtils::Format("%s", SVN_REV));
      SET_CONTROL_LABEL(53, __DATE__);
      return true;
    }
    break;

    case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow::OnMessage(message);
      return true;
    }
    break;

    case GUI_MSG_FOCUSED:
    {
      CGUIWindow::OnMessage(message);
      int focusedControl = GetFocusedControlID();
      if (m_section != focusedControl && focusedControl >= CONTROL_START && focusedControl <= CONTROL_END)
      {
        ResetLabels();
        m_section = focusedControl;
      }
      if (m_section >= CONTROL_BT_STORAGE && m_section <= CONTROL_BT_HARDWARE)
      {
        SET_CONTROL_VISIBLE(CONTROL_GROUP_SYSTEM_BAR);
      }
      return true;
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSystemInfo::FrameMove()
{
  int i = CONTROL_TEXT_START;
  if (m_section == CONTROL_BT_DEFAULT)
  {
    SET_CONTROL_LABEL(40, g_localizeStrings.Get(20154));
    SetControlLabel(i++, "%s %s", 22011, SYSTEM_CPU_TEMPERATURE);
    SetControlLabel(i++, "%s %s", 22010, SYSTEM_GPU_TEMPERATURE);
    SetControlLabel(i++, "%s: %s", 13300, SYSTEM_FAN_SPEED);
    SetControlLabel(i++, "%s: %s", 158, SYSTEM_FREE_MEMORY);
    SetControlLabel(i++, "%s: %s", 150, NETWORK_IP_ADDRESS);
    SetControlLabel(i++, "%s %s", 13287, SYSTEM_SCREEN_RESOLUTION);
    SetControlLabel(i++, "%s %s", 13283, SYSTEM_OS_VERSION_INFO);
    SetControlLabel(i++, "%s: %s", 12390, SYSTEM_UPTIME);
    SetControlLabel(i++, "%s: %s", 12394, SYSTEM_TOTALUPTIME);
  }

  else if(m_section == CONTROL_BT_HDD)
  {
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20156));
    SetControlLabel(i++, "%s %s", 38725, SYSTEM_HDD_MODEL);
    SetControlLabel(i++, "%s %s", 38726, SYSTEM_HDD_SERIAL);
    SetControlLabel(i++, "%s %s", 38727, SYSTEM_HDD_FIRMWARE);
    SetControlLabel(i++, "%s %s", 38728, SYSTEM_HDD_PASSWORD);
    SetControlLabel(i++, "%s %s", 38729, SYSTEM_HDD_LOCKSTATE);
    SetControlLabel(i++, "%s %s", 38721, SYSTEM_HDD_LOCKKEY);
    SetControlLabel(i++, "%s %s", 13173, SYSTEM_HDD_BOOTDATE);
    SetControlLabel(i++, "%s %s", 13174, SYSTEM_HDD_CYCLECOUNT);
    SetControlLabel(i++, "%s %s", 38722, SYSTEM_HDD_TEMPERATURE);
  }

  else if(m_section == CONTROL_BT_DVD)
  {
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20157));
    SetControlLabel(i++, "%s %s", 38723, SYSTEM_DVD_MODEL);
    SetControlLabel(i++, "%s %s", 38724, SYSTEM_DVD_FIRMWARE);
    SetControlLabel(i++, "%s %s", 13294, SYSTEM_DVD_ZONE);
  }

  else if (m_section == CONTROL_BT_STORAGE)
  {
    SET_CONTROL_LABEL(40, g_localizeStrings.Get(20155));
    SET_CONTROL_LABEL(2, CServiceBroker::GetGUI()->GetInfoManager().GetLabel(SYSTEM_DVD_TRAY_STATE, INFO::DEFAULT_CONTEXT));
    SET_CONTROL_LABEL(3, CServiceBroker::GetGUI()->GetInfoManager().GetLabel(SYSTEM_FREE_SPACE_C, INFO::DEFAULT_CONTEXT));
    SET_CONTROL_LABEL(4, CServiceBroker::GetGUI()->GetInfoManager().GetLabel(SYSTEM_FREE_SPACE_E, INFO::DEFAULT_CONTEXT));
    SET_CONTROL_LABEL(5, CServiceBroker::GetGUI()->GetInfoManager().GetLabel(SYSTEM_FREE_SPACE_F, INFO::DEFAULT_CONTEXT));
    SET_CONTROL_LABEL(6, CServiceBroker::GetGUI()->GetInfoManager().GetLabel(SYSTEM_FREE_SPACE_G, INFO::DEFAULT_CONTEXT));
    SET_CONTROL_LABEL(7, CServiceBroker::GetGUI()->GetInfoManager().GetLabel(SYSTEM_FREE_SPACE_X, INFO::DEFAULT_CONTEXT));
    SET_CONTROL_LABEL(8, CServiceBroker::GetGUI()->GetInfoManager().GetLabel(SYSTEM_FREE_SPACE_Y, INFO::DEFAULT_CONTEXT));
    SET_CONTROL_LABEL(9, CServiceBroker::GetGUI()->GetInfoManager().GetLabel(SYSTEM_FREE_SPACE_Z, INFO::DEFAULT_CONTEXT));
    SetControlLabel(10, "%s: %s", 20161, SYSTEM_TOTAL_SPACE);
    SetControlLabel(11, "%s: %s", 20161, SYSTEM_USED_SPACE_PERCENT);
    SET_CONTROL_LABEL(12,CServiceBroker::GetGUI()->GetInfoManager().GetLabel(SYSTEM_FREE_SPACE_PERCENT, INFO::DEFAULT_CONTEXT));
  }

  else if (m_section == CONTROL_BT_NETWORK)
  {
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20158));
    SetControlLabel(i++, "%s %s", 146, NETWORK_IS_DHCP);
    SetControlLabel(i++, "%s %s", 151, NETWORK_LINK_STATE);
    SetControlLabel(i++, "%s: %s", 149, NETWORK_MAC_ADDRESS);
    SetControlLabel(i++, "%s: %s", 150, NETWORK_IP_ADDRESS);
    SetControlLabel(i++, "%s: %s", 13159, NETWORK_SUBNET_MASK);
    SetControlLabel(i++, "%s: %s", 13160, NETWORK_GATEWAY_ADDRESS);
    SetControlLabel(i++, "%s: %s", 13161, NETWORK_DNS1_ADDRESS);
    SetControlLabel(i++, "%s: %s", 20307, NETWORK_DNS2_ADDRESS);
    SetControlLabel(i++, "%s %s", 13295, SYSTEM_INTERNET_STATE);
  }

  else if (m_section == CONTROL_BT_VIDEO)
  {
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20159));
    SetControlLabel(i++, "%s %s", 13286, SYSTEM_VIDEO_ENCODER_INFO);
    SetControlLabel(i++, "%s %s", 13287, SYSTEM_SCREEN_RESOLUTION);
    SetControlLabel(i++, "%s %s", 13292, SYSTEM_AV_PACK_INFO);
    SetControlLabel(i++, "%s %s", 38742, SYSTEM_XBE_REGION);
  }

  else if (m_section == CONTROL_BT_HARDWARE)
  {
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20160));
    SetControlLabel(i++, "%s %s", 38738, SYSTEM_XBOX_VERSION);
    SetControlLabel(i++, "%s %s", 38739, SYSTEM_XBOX_SERIAL);
    SetControlLabel(i++, "%s %s", 13284, SYSTEM_CPUFREQUENCY);
    SetControlLabel(i++, "%s %s", 38737, SYSTEM_XBOX_BIOS);
    SET_CONTROL_LABEL(i++, CServiceBroker::GetGUI()->GetInfoManager().GetLabel(SYSTEM_XBOX_MODCHIP, INFO::DEFAULT_CONTEXT));
    SetControlLabel(i++, "%s %s", 38740, SYSTEM_XBOX_PRODUCE_INFO);
    SetControlLabel(i++, "%s 1: %s", 38736, SYSTEM_CONTROLLER_PORT_1);
    SetControlLabel(i++, "%s 2: %s", 38736, SYSTEM_CONTROLLER_PORT_2);
    SetControlLabel(i++, "%s 3: %s", 38736, SYSTEM_CONTROLLER_PORT_3);
    SetControlLabel(i++, "%s 4: %s", 38736, SYSTEM_CONTROLLER_PORT_4);
  }
  CGUIWindow::FrameMove();
}

void CGUIWindowSystemInfo::ResetLabels()
{
  for (int i = CONTROL_TEXT_START; i <= CONTROL_TEXT_END; ++i)
  {
    SET_CONTROL_LABEL(i, "");
  }
}

void CGUIWindowSystemInfo::SetControlLabel(int id, const char *format, int label, int info)
{
  std::string tmpStr = StringUtils::Format(
      format, g_localizeStrings.Get(label).c_str(),
      CServiceBroker::GetGUI()->GetInfoManager().GetLabel(info, INFO::DEFAULT_CONTEXT).c_str());
  SET_CONTROL_LABEL(id, tmpStr);
}
