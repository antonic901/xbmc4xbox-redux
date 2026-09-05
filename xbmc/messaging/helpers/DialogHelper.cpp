/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogHelper.h"

#include "ServiceBroker.h"
#include "messaging/ApplicationMessenger.h"

#include <cassert>
#include <utility>

namespace KODI
{
namespace MESSAGING
{
namespace HELPERS
{
DialogResponse ShowYesNoDialogText(CVariant heading, CVariant text, CVariant noLabel, CVariant yesLabel, uint32_t autoCloseTimeout)
{
  return ShowYesNoCustomDialog(heading, text, noLabel,
                               yesLabel, "", autoCloseTimeout);
}

DialogResponse ShowYesNoCustomDialog(CVariant heading, CVariant text, CVariant noLabel, CVariant yesLabel, CVariant customLabel, uint32_t autoCloseTimeout)
{
  DialogYesNoMessage options;
  options.heading = heading;
  options.text = text;
  options.noLabel = noLabel;
  options.yesLabel = yesLabel;
  options.customLabel = customLabel;
  options.autoclose = autoCloseTimeout;

  switch (CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_DIALOG_YESNO, -1, -1,
                                                     static_cast<void*>(&options)))
  {
  case -1:
    return CHOICE_CANCELLED;
  case 0:
    return CHOICE_NO;
  case 1:
    return CHOICE_YES;
  case 2:
    return CHOICE_CUSTOM;
  default:
    //If we get here someone changed the return values without updating this code
    assert(false);
  }
  //This is unreachable code but we need to return something to suppress warnings about
  //no return
  return CHOICE_CANCELLED;
}

DialogResponse ShowYesNoDialogLines(CVariant heading, CVariant line0, CVariant line1, CVariant line2,
  CVariant noLabel, CVariant yesLabel, uint32_t autoCloseTimeout)
{
  DialogYesNoMessage options;
  options.heading = heading;
  options.lines[0] = line0;
  options.lines[1] = line1;
  options.lines[2] = line2;
  options.noLabel = noLabel;
  options.yesLabel = yesLabel;
  options.customLabel = "";
  options.autoclose = autoCloseTimeout;

  switch (CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_DIALOG_YESNO, -1, -1,
                                                     static_cast<void*>(&options)))
  {
  case -1:
    return CHOICE_CANCELLED;
  case 0:
    return CHOICE_NO;
  case 1:
    return CHOICE_YES;
  case 2:
    return CHOICE_CUSTOM;
  default:
    //If we get here someone changed the return values without updating this code
    assert(false);
  }
  //This is unreachable code but we need to return something to suppress warnings about
  //no return
  return CHOICE_CANCELLED;
}

}
}
}
