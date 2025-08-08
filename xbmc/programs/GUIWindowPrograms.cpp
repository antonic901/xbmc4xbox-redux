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

#include "programs/GUIWindowPrograms.h"
#include "Util.h"
#include "Shortcut.h"
#include "filesystem/HDDirectory.h"
#include "GUIPassword.h"
#include "dialogs/GUIDialogTrainerSettings.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "xbox/xbeheader.h"
#include "utils/Trainer.h"
#include "utils/LabelFormatter.h"
#include "Autorun.h"
#include "GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIKeyboardFactory.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/RarManager.h"
#include "FileItem.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "utils/URIUtils.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "view/GUIViewState.h"

using namespace XFILE;

#define CONTROL_BTNVIEWASICONS 2
#define CONTROL_BTNSORTBY      3
#define CONTROL_BTNSORTASC     4
#define CONTROL_LIST          50
#define CONTROL_THUMBS        51
#define CONTROL_LABELFILES    12

CGUIWindowPrograms::CGUIWindowPrograms(void)
    : CGUIMediaWindow(WINDOW_PROGRAMS, "MyPrograms.xml")
{
  m_thumbLoader.SetObserver(this);
  m_dlgProgress = NULL;
  m_rootDir.AllowNonLocalSources(false); // no nonlocal shares for this window please
}


CGUIWindowPrograms::~CGUIWindowPrograms(void)
{
}

bool CGUIWindowPrograms::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();
      m_database.Close();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_iRegionSet = 0;
      m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      // is this the first time accessing this window?
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().empty())
        message.SetStringParam(CMediaSourceSettings::Get().GetDefaultSource("programs"));

      m_database.Open();

      return CGUIMediaWindow::OnMessage(message);
    }
  break;

  case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_BTNSORTBY)
      {
        // need to update shortcuts manually
        if (CGUIMediaWindow::OnMessage(message))
        {
          LABEL_MASKS labelMasks;
          m_guiState->GetSortMethodLabelMasks(labelMasks);
          CLabelFormatter formatter("", labelMasks.m_strLabel2File);
          for (int i=0;i<m_vecItems->Size();++i)
          {
            CFileItemPtr item = m_vecItems->Get(i);
            if (item->IsShortCut())
              formatter.FormatLabel2(item.get());
          }
          return true;
        }
        else
          return false;
      }
      if (m_viewControl.HasControl(message.GetSenderId()))  // list/thumb control
      {
        if (message.GetParam1() == ACTION_PLAYER_PLAY)
        {
          OnPlayMedia(m_viewControl.GetSelectedItem());
          return true;
        }
      }
    }
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowPrograms::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (item)
  {
    if ( m_vecItems->IsVirtualDirectoryRoot() || m_vecItems->GetPath() == "sources://programs/" )
    {
      CGUIDialogContextMenu::GetContextButtons("programs", item, buttons);
    }
    else
    {
      if (item->IsXBE() || item->IsShortCut())
      {
        CStdString strLaunch = g_localizeStrings.Get(518); // Launch
        if (CSettings::GetInstance().GetBool("myprograms.gameautoregion"))
        {
          int iRegion = GetRegion(itemNumber);
          if (iRegion == VIDEO_NTSCM)
            strLaunch += " (NTSC-M)";
          if (iRegion == VIDEO_NTSCJ)
            strLaunch += " (NTSC-J)";
          if (iRegion == VIDEO_PAL50)
            strLaunch += " (PAL)";
          if (iRegion == VIDEO_PAL60)
            strLaunch += " (PAL-60)";
        }
        buttons.Add(CONTEXT_BUTTON_LAUNCH, strLaunch);

        DWORD dwTitleId = CUtil::GetXbeID(item->GetPath());
        CStdString strTitleID;
        strTitleID.Format("%08X",dwTitleId);
        CStdString strGameSavepath = URIUtils::AddFileToFolder("E:\\udata\\",strTitleID);

        if (CDirectory::Exists(strGameSavepath))
          buttons.Add(CONTEXT_BUTTON_GAMESAVES, 38778);         // Goto GameSaves

        if (CSettings::GetInstance().GetBool("myprograms.gameautoregion"))
          buttons.Add(CONTEXT_BUTTON_LAUNCH_IN, 519); // launch in video mode

        if (g_passwordManager.IsMasterLockUnlocked(false) || CProfilesManager::Get().GetCurrentProfile().canWriteDatabases())
        {
          if (item->IsShortCut())
            buttons.Add(CONTEXT_BUTTON_RENAME, 16105); // rename
          else
            buttons.Add(CONTEXT_BUTTON_RENAME, 38693); // edit xbe title
        }

        buttons.Add(CONTEXT_BUTTON_TRAINER_OPTIONS, 38712); // trainer options
      }
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowPrograms::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = (itemNumber >= 0 && itemNumber < m_vecItems->Size()) ? m_vecItems->Get(itemNumber) : CFileItemPtr();

  if (CGUIDialogContextMenu::OnContextButton("programs", item, button))
  {
    Update("");
    return true;
  }
  switch (button)
  {
  case CONTEXT_BUTTON_RENAME:
    {
      CStdString strDescription;
      CShortcut cut;
      if (item->IsShortCut())
      {
        cut.Create(item->GetPath());
        strDescription = cut.m_strLabel;
      }
      else
        strDescription = item->GetLabel();

      if (CGUIKeyboardFactory::ShowAndGetInput(strDescription, g_localizeStrings.Get(16008), false))
      {
        if (item->IsShortCut())
        {
          cut.m_strLabel = strDescription;
          cut.Save(item->GetPath());
        }
        else
        {
          // SetXBEDescription will truncate to 40 characters.
          CUtil::SetXBEDescription(item->GetPath(),strDescription);
          m_database.SetDescription(item->GetPath(),strDescription);
        }
        Update(m_vecItems->GetPath());
      }
      return true;
    }

  case CONTEXT_BUTTON_TRAINER_OPTIONS:
    {
      CGUIDialogTrainerSettings::ShowForTitle(item);
      return true;
    }

  case CONTEXT_BUTTON_LAUNCH:
    OnClick(itemNumber);
    return true;

  case CONTEXT_BUTTON_GAMESAVES:
    {
      CStdString strTitleID;
      strTitleID.Format("%08X",CUtil::GetXbeID(item->GetPath()));
      CStdString strGameSavepath = URIUtils::AddFileToFolder("E:\\udata\\",strTitleID);
      g_windowManager.ActivateWindow(WINDOW_GAMESAVES,strGameSavepath);
      return true;
    }
  case CONTEXT_BUTTON_LAUNCH_IN:
    OnChooseVideoModeAndLaunch(itemNumber);
    return true;
  default:
    break;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowPrograms::OnAddMediaSource()
{
  return CGUIDialogMediaSource::ShowAndAddMediaSource("programs");
}

bool CGUIWindowPrograms::OnChooseVideoModeAndLaunch(int item)
{
  if (item < 0 || item >= m_vecItems->Size()) return false;

  int btn_PAL = 1;
  int btn_NTSCM = 2;
  int btn_NTSCJ = 3;
  int btn_PAL60 = 4;
  CStdString strPAL, strNTSCJ, strNTSCM, strPAL60;
  strPAL = "PAL";
  strNTSCM = "NTSC-M";
  strNTSCJ = "NTSC-J";
  strPAL60 = "PAL-60";
  int iRegion = GetRegion(item,true);

  if (iRegion == VIDEO_NTSCM)
    strNTSCM += " (default)";
  if (iRegion == VIDEO_NTSCJ)
    strNTSCJ += " (default)";
  if (iRegion == VIDEO_PAL50)
    strPAL += " (default)";

  // add the needed buttons
  CContextButtons choices;
  choices.Add(btn_PAL, strPAL);
  choices.Add(btn_NTSCM, strNTSCM);
  choices.Add(btn_NTSCJ, strNTSCJ);
  choices.Add(btn_PAL60, strPAL60);

  int btnid = CGUIDialogContextMenu::ShowAndGetChoice(choices);

  if (btnid == btn_NTSCM)
  {
    m_iRegionSet = VIDEO_NTSCM;
    m_database.SetRegion(m_vecItems->Get(item)->GetPath(),1);
  }
  if (btnid == btn_NTSCJ)
  {
    m_iRegionSet = VIDEO_NTSCJ;
    m_database.SetRegion(m_vecItems->Get(item)->GetPath(),2);
  }
  if (btnid == btn_PAL)
  {
    m_iRegionSet = VIDEO_PAL50;
    m_database.SetRegion(m_vecItems->Get(item)->GetPath(),4);
  }
  if (btnid == btn_PAL60)
  {
    m_iRegionSet = VIDEO_PAL60;
    m_database.SetRegion(m_vecItems->Get(item)->GetPath(),8);
  }

  if (btnid > -1)
    return OnClick(item);

  return true;
}

bool CGUIWindowPrograms::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory, updateFilterPath))
    return false;

  m_thumbLoader.Load(*m_vecItems);
  return true;
}

bool CGUIWindowPrograms::OnPlayMedia(int iItem, const std::string &player)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return false;
  CFileItemPtr pItem = m_vecItems->Get(iItem);

  if (StringUtils::StartsWithNoCase(pItem->GetPath(), "insignia://"))
  {
    g_windowManager.ActivateWindow(WINDOW_INSIGNIA);
    return true;
  }

  if (pItem->IsDVD())
    return MEDIA_DETECT::CAutorun::PlayDisc();

  if (pItem->m_bIsFolder) return false;

  // launch xbe...
  char szPath[1024];
  char szParameters[1024];

  m_database.IncTimesPlayed(pItem->GetPath());

  int iRegion = m_iRegionSet?m_iRegionSet:GetRegion(iItem);

  DWORD dwTitleId = 0;
  if (!pItem->IsOnDVD())
    dwTitleId = m_database.GetTitleId(pItem->GetPath());
  if (!dwTitleId)
    dwTitleId = CUtil::GetXbeID(pItem->GetPath());

  // Load active trainer
  CFileItemList items;
  if (m_database.GetTrainers(items, dwTitleId))
  {
    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->GetProperty("isactive").asBoolean())
      {
        CTrainer* trainer = new CTrainer(items[i]->GetProperty("idtrainer").asInteger32());
        if (trainer->Load(items[i]->GetPath()))
        {
          m_database.GetTrainerOptions(trainer->GetTrainerId(), dwTitleId, trainer->GetOptions(), trainer->GetNumberOfOptions());
          CTrainer::InstallTrainer(*trainer);
        }
        else
        {
          delete trainer;
        }
      }
    }
  }

  m_database.Close();
  memset(szParameters, 0, sizeof(szParameters));

  strcpy(szPath, pItem->GetPath().c_str());

  if (pItem->IsShortCut())
  {
    CUtil::RunShortcut(pItem->GetPath().c_str());
    return false;
  }

  if (strlen(szParameters))
    CUtil::RunXBE(szPath, szParameters,F_VIDEO(iRegion));
  else
    CUtil::RunXBE(szPath,NULL,F_VIDEO(iRegion));
  return true;
}

int CGUIWindowPrograms::GetRegion(int iItem, bool bReload)
{
  if (!CSettings::GetInstance().GetBool("myprograms.gameautoregion"))
    return 0;

  int iRegion;
  if (bReload || m_vecItems->Get(iItem)->IsOnDVD())
  {
    CXBE xbe;
    iRegion = xbe.ExtractGameRegion(m_vecItems->Get(iItem)->GetPath());
  }
  else
  {
    m_database.Open();
    iRegion = m_database.GetRegion(m_vecItems->Get(iItem)->GetPath());
    m_database.Close();
  }
  if (iRegion == -1)
  {
    if (CSettings::GetInstance().GetBool("myprograms.gameautoregion"))
    {
      CXBE xbe;
      iRegion = xbe.ExtractGameRegion(m_vecItems->Get(iItem)->GetPath());
      if (iRegion < 1 || iRegion > 7)
        iRegion = 0;
      m_database.SetRegion(m_vecItems->Get(iItem)->GetPath(),iRegion);
    }
    else
      iRegion = 0;
  }

  if (bReload)
    return CXBE::FilterRegion(iRegion,true);
  else
    return CXBE::FilterRegion(iRegion);
}

bool CGUIWindowPrograms::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
#ifdef _XBOX
  bool bFlattened=false;
  if (URIUtils::IsDVD(strDirectory))
  {
    CStdString strPath = URIUtils::AddFileToFolder(strDirectory,"default.xbe");
    if (CFile::Exists(strPath)) // flatten dvd
    {
      CFileItemPtr item(new CFileItem("default.xbe"));
      item->SetPath(strPath);
      items.Add(item);
      items.SetPath(strDirectory);
      bFlattened = true;
    }
  }
  if (!bFlattened)
#endif
    if (!CGUIMediaWindow::GetDirectory(strDirectory, items))
      return false;

  // don't allow the view state to change these
  if (StringUtils::StartsWithNoCase(strDirectory, "addons://"))
  {
    for (int i=0;i<items.Size();++i)
    {
      items[i]->SetLabel2(items[i]->GetProperty("Addon.Version").asString());
      items[i]->SetLabelPreformated(true);
    }
  }

#ifdef _XBOX
  if (items.IsVirtualDirectoryRoot())
  {
    CFileItemPtr pItem(new CFileItem());
    pItem->SetPath("insignia://");
    pItem->SetIconImage("insignia/logo.png");
    pItem->SetLabel(g_localizeStrings.Get(38901));
    pItem->SetLabelPreformated(true);
    pItem->SetProperty("overview", g_localizeStrings.Get(38902));
    pItem->SetSpecialSort(SortSpecialOnTop);
    items.Add(pItem);

    items.SetLabel("");
    return true;
  }

  // flatten any folders
  m_database.BeginTransaction();
  unsigned int tick = XbmcThreads::SystemClockMillis();
  bool bProgressVisible = false;
  for (int i = 0; i < items.Size(); i++)
  {
    CStdString shortcutPath;
    CFileItemPtr item = items[i];
    if (!bProgressVisible && (XbmcThreads::SystemClockMillis() - tick) > 1500 && m_dlgProgress)
    { // tag loading takes more then 1.5 secs, show a progress dialog
      m_dlgProgress->SetHeading(189);
      m_dlgProgress->SetLine(0, 20120);
      m_dlgProgress->SetLine(1,"");
      m_dlgProgress->SetLine(2, item->GetLabel());
      m_dlgProgress->Open();
      bProgressVisible = true;
    }
    if (bProgressVisible)
    {
      m_dlgProgress->SetLine(2,item->GetLabel());
      m_dlgProgress->Progress();
    }

    if (item->m_bIsFolder && !item->IsParentFolder() && !item->IsPlugin())
    { // folder item - let's check for a default.xbe file, and flatten if we have one
      CStdString defaultXBE = URIUtils::AddFileToFolder(item->GetPath(), "default.xbe");
      if (CFile::Exists(defaultXBE))
      { // yes, format the item up
        item->SetPath(defaultXBE);
        item->m_bIsFolder = false;
      }
    }
    else if (item->IsShortCut())
    { // resolve the shortcut to set it's description etc.
      // and save the old shortcut path (so we can reassign it later)
      CShortcut cut;
      if (cut.Create(item->GetPath()))
      {
        shortcutPath = item->GetPath();
        item->SetPath(cut.m_strPath);
        item->SetArt("thumb", cut.m_strThumb);

        LABEL_MASKS labelMasks;
        m_guiState->GetSortMethodLabelMasks(labelMasks);
        CLabelFormatter formatter("", labelMasks.m_strLabel2File);
        if (!cut.m_strLabel.IsEmpty())
        {
          item->SetLabel(cut.m_strLabel);
          __stat64 stat;
          if (CFile::Stat(item->GetPath(),&stat) == 0)
            item->m_dwSize = stat.st_size;

          formatter.FormatLabel2(item.get());
          item->SetLabelPreformated(true);
        }
      }
    }
    if (item->IsXBE())
    {
      if (URIUtils::GetFileName(item->GetPath()).Equals("default_ffp.xbe"))
      {
        m_vecItems->Remove(i--);
        continue;
      }
      // add to database if not already there
      DWORD dwTitleID = item->IsOnDVD() ? 0 : m_database.GetProgramInfo(item.get());
      if (!dwTitleID)
      {
        CStdString description;
        if (CUtil::GetXBEDescription(item->GetPath(), description) && (!item->IsLabelPreformated() && !item->GetLabel().empty()))
          item->SetLabel(description);

        dwTitleID = CUtil::GetXbeID(item->GetPath());
        if (!item->IsOnDVD())
          m_database.AddProgramInfo(item.get(), dwTitleID);
      }
    }
    if (!shortcutPath.IsEmpty())
      item->SetPath(shortcutPath);

    if (item->IsXBE())
    {
      // Load XBMC4Gamers artwork and metadata
      CFileItemList items;
      std::string strRootPath = URIUtils::GetParentPath(item->GetPath());
      std::string strPath = URIUtils::AddFileToFolder(strRootPath, "_resources\\artwork\\");
      if(CDirectory::Exists(strPath) && CDirectory::GetDirectory(strPath, items, g_advancedSettings.m_pictureExtensions, DIR_FLAG_DEFAULTS))
      {
        for (int i = 0; i < items.Size(); i++)
        {
          std::string strProperty(items[i]->GetLabel());
          URIUtils::RemoveExtension(strProperty);
          item->SetArt(strProperty, items[i]->GetPath());
        }
      }

      items.Clear();
      strPath = URIUtils::AddFileToFolder(strRootPath, "_resources\\screenshots\\");
      if(CDirectory::Exists(strPath) && CDirectory::GetDirectory(strPath, items, g_advancedSettings.m_pictureExtensions, DIR_FLAG_DEFAULTS))
      {
        for (int i = 0; i < items.Size(); i++)
        {
          std::string strProperty(items[i]->GetLabel());
          URIUtils::RemoveExtension(strProperty);
          item->SetArt(strProperty, items[i]->GetPath());
        }
      }

      items.Clear();
      strPath = URIUtils::AddFileToFolder(strRootPath, "_resources\\media\\");
      if(CDirectory::Exists(strPath) && CDirectory::GetDirectory(strPath, items, g_advancedSettings.m_videoExtensions, DIR_FLAG_DEFAULTS))
      {
        for (int i = 0; i < items.Size(); i++)
        {
          std::string strProperty(items[i]->GetLabel());
          URIUtils::RemoveExtension(strProperty);
          item->SetProperty(StringUtils::Format("media_%s", strProperty.c_str()), items[i]->GetPath());
        }
      }

      CXBMCTinyXML doc;
      strPath = URIUtils::AddFileToFolder(strRootPath, "_resources\\default.xml");
      if (doc.LoadFile(strPath) && doc.RootElement())
      {
        const TiXmlElement* synopsis = doc.RootElement();
        std::string value;
        float fValue;
        int iValue;

        if (XMLUtils::GetString(synopsis, "sourcename", value))
          item->SetProperty("sourcename", value);

        if (XMLUtils::GetString(synopsis, "foldername", value))
          item->SetProperty("foldername", value);

        if (XMLUtils::GetString(synopsis, "title", value))
        {
          item->SetLabel(value);
          item->SetProperty("title", value);
        }

        if (XMLUtils::GetString(synopsis, "developer", value))
          item->SetProperty("developer", value);

        if (XMLUtils::GetString(synopsis, "publisher", value))
          item->SetProperty("publisher", value);

        if (XMLUtils::GetString(synopsis, "features_general", value))
          item->SetProperty("features_general", value);

        if (XMLUtils::GetString(synopsis, "features_online", value))
          item->SetProperty("features_online", value);

        if (XMLUtils::GetString(synopsis, "esrb", value))
          item->SetProperty("esrb", value);

        if (XMLUtils::GetString(synopsis, "esrb_descriptors", value))
          item->SetProperty("esrb_descriptors", value);

        if (XMLUtils::GetString(synopsis, "genre", value))
          item->SetProperty("genre", value);

        if (XMLUtils::GetString(synopsis, "release_date", value))
          item->SetProperty("release_date", value);

        if (XMLUtils::GetInt(synopsis, "year", iValue))
          item->SetProperty("year", iValue);

        if (XMLUtils::GetFloat(synopsis, "rating", fValue))
          item->SetProperty("rating", fValue);

        if (XMLUtils::GetString(synopsis, "platform", value))
          item->SetProperty("platform", value);

        if (XMLUtils::GetString(synopsis, "exclusive", value))
          item->SetProperty("exclusive", value);

        if (XMLUtils::GetString(synopsis, "titleid", value))
          item->SetProperty("titleid", value);

        if (XMLUtils::GetString(synopsis, "overview", value))
        {
          item->SetLabel2(value);
          item->SetProperty("overview", value);
        }
      }
    }
  }
  m_database.CommitTransaction();

  // set the folder thumb
  CProgramThumbLoader loader;
  loader.LoadItem(&items);

  if (bProgressVisible)
    m_dlgProgress->Close();
#endif

  return true;
}

std::string CGUIWindowPrograms::GetStartFolder(const std::string &dir)
{
  if (dir == "Plugins" || dir == "Addons")
    return "addons://sources/executable/";

  SetupShares();
  VECSOURCES shares;
  m_rootDir.GetSources(shares);
  bool bIsSourceName = false;
  int iIndex = CUtil::GetMatchingSource(dir, shares, bIsSourceName);
  if (iIndex > -1)
  {
    if (iIndex < (int)shares.size() && shares[iIndex].m_iHasLock == 2)
    {
      CFileItem item(shares[iIndex]);
      if (!g_passwordManager.IsItemUnlocked(&item,"programs"))
        return "";
    }
    if (bIsSourceName)
      return shares[iIndex].strPath;
    return dir;
  }
  return CGUIMediaWindow::GetStartFolder(dir);
}
