#include "LCDFactory.h"

#include "smartxx/smartxxlcd.h"
#include "libXenium/XeniumLCD.h"
#include "x3lcd/x3lcd.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

ILCD* CLCDFactory::Create()
{
  switch (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("lcd.modchip"))
  {
  case MODCHIP_XENIUM:
    return new CXeniumLCD();

  case MODCHIP_SMARTXX:
    return new CSmartXXLCD();

  case MODCHIP_XECUTER3:
    return new CX3LCD();
  }

  return NULL;
}
