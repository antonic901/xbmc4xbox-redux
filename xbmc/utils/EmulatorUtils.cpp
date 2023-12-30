
#include "EmulatorUtils.h"
#include "programs/ProgramDatabase.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "dialogs/GUIDialogSelect.h"
#include "utils/StringUtils2.h"
#include "utils/URIUtils.h"

#include "FileItem.h"

bool EmulatorUtils::GetSystemFromFilename(std::string strFilename, SystemMapping& system)
{
  std::string strExtension = URIUtils::GetExtension(strFilename);
  strExtension += "|";
  StringUtils2::ToLower(strExtension);
  for (unsigned int index = 0; index < sizeof(systems); ++index)
  {
    std::string strExt = systems[index].extension;
    strExt += "|";
    if ((size_t)strExt.find(strExtension) != -1)
    {
      system = systems[index];
      return true;
    }
  }
  return false;
}

bool EmulatorUtils::ChooseEmulatorAndLaunch(std::string strPath)
{
  SystemMapping system;
  if (!GetSystemFromFilename(strPath, system))
    return false;

  CProgramDatabase database;
  if (database.Open())
  {
    CFileItemList emulators;
    if (!database.GetEmulatorsForSystem(system.shortname, emulators))
      return false;

    CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDlgSelect->SetHeading(StringUtils2::Format("%s %s", g_localizeStrings.Get(35112).c_str(), system.name));
    pDlgSelect->Reset();
    pDlgSelect->Add(emulators);
    pDlgSelect->EnableButton(true, 222);
    pDlgSelect->DoModal();
    int choice = pDlgSelect->GetSelectedLabel();
    if (choice < 0)
      return false;
    return LaunchROM(strPath, emulators[choice]->GetPath());
  }

  return false;
}

bool EmulatorUtils::LaunchROM(std::string strRomPath, std::string strEmuPath)
{
  // TODO: figure out how to launch ROM
  return false;
}
