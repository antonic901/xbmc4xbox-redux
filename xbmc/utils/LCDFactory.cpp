
#include "LCDFactory.h"
#include "smartxx/smartxxlcd.h"
#include "libXenium/XeniumLCD.h"
#include "x3lcd/x3lcd.h"
#include "settings/Settings.h"

ILCD* g_lcd = NULL;
CLCDFactory::CLCDFactory(void)
{}

CLCDFactory::~CLCDFactory(void)
{}

ILCD* CLCDFactory::Create()
{
  switch (CSettings::GetInstance().GetInt("lcd.modchip"))
  {
  case MODCHIP_XENIUM:
    return new CXeniumLCD();
    break;

  case MODCHIP_SMARTXX:
    return new CSmartXXLCD();
    break;

  case MODCHIP_XECUTER3:
    return new CX3LCD();
    break;

  }
  return new CSmartXXLCD();
}
