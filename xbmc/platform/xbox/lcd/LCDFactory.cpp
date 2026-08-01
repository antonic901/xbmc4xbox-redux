/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LCDFactory.h"

#include "LCD.h"
#include "libXenium/XeniumLCD.h"
#include "smartXX/smartxxlcd.h"
#include "x3lcd/x3lcd.h"

ILCD* CLCDFactory::Create(const LCD_MODCHIP& type)
{
  if (type == MODCHIP_XENIUM)
    return new CXeniumLCD();
  if (type == MODCHIP_SMARTXX)
    return new CSmartXXLCD();
  if (type == MODCHIP_XECUTER3)
    return new CX3LCD();

  return NULL;
}
