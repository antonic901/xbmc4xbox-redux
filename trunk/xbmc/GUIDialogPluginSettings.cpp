/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogPluginSettings.h"
#include "FileSystem/PluginDirectory.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogFileBrowser.h"
#include "GUIControlGroupList.h"
#include "Util.h"
#include "MediaManager.h"
#include "GUILabelControl.h"
#include "GUIRadioButtonControl.h"
#include "GUISpinControlEx.h"
#include "GUIImage.h"
#include "FileSystem/Directory.h"
#include "VideoInfoScanner.h"
#include "ScraperSettings.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "GUIDialogKeyboard.h"
#include "FileItem.h"
#include "ScriptSettings.h"
#include "GUIDialogOK.h"
#include "GUIControlGroupList.h"
#include "GUISettingsSliderControl.h"
#include "StringUtils.h"
#include "Settings.h"
#include "GUIInfoManager.h"
#include "GUIDialogSelect.h"

using namespace std;
using namespace DIRECTORY;

#define CONTROL_SETTINGS_AREA           2
#define CONTROL_DEFAULT_BUTTON          3
#define CONTROL_DEFAULT_RADIOBUTTON     4
#define CONTROL_DEFAULT_SPIN            5
#define CONTROL_DEFAULT_SEPARATOR       6
#define CONTROL_DEFAULT_LABEL_SEPARATOR 7
#define CONTROL_DEFAULT_SLIDER          8
#define CONTROL_SECTION_AREA            9
#define CONTROL_DEFAULT_SECTION_BUTTON  13

#define ID_BUTTON_OK                    10
#define ID_BUTTON_CANCEL                11
#define ID_BUTTON_DEFAULT               12
#define CONTROL_HEADING_LABEL           20

#define CONTROL_START_SETTING           200
#define CONTROL_START_SECTION           100

CGUIDialogPluginSettings::CGUIDialogPluginSettings()
   : CGUIDialogBoxBase(WINDOW_DIALOG_PLUGIN_SETTINGS, "DialogPluginSettings.xml")
{
  m_currentSection = 0;
  m_totalSections = 1;
}

CGUIDialogPluginSettings::~CGUIDialogPluginSettings(void)
{
}

bool CGUIDialogPluginSettings::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      FreeControls();
      FreeSections();
    }
    break;
    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      bool bCloseDialog = false;

      if (iControl == ID_BUTTON_DEFAULT)
        SetDefaults();
      else if (iControl != ID_BUTTON_OK)
        bCloseDialog = ShowVirtualKeyboard(iControl);

      if (iControl == ID_BUTTON_OK || iControl == ID_BUTTON_CANCEL || bCloseDialog)
      {
        if (iControl == ID_BUTTON_OK || bCloseDialog)
        {
          m_bConfirmed = true;
          SaveSettings();
        }
        if (iControl == ID_BUTTON_OK && !m_closeAction.IsEmpty())
        {
          g_applicationMessenger.ExecBuiltIn(m_closeAction);
        }
        Close();
        return true;
      }
    }
    break;
    case GUI_MSG_FOCUSED:
    {
      CGUIDialogBoxBase::OnMessage(message);
      int focusedControl = GetFocusedControlID();
      if (focusedControl >= CONTROL_START_SECTION && focusedControl < (int)(CONTROL_START_SECTION + m_totalSections) &&
          focusedControl - CONTROL_START_SECTION != (int)m_currentSection)
      { // changing section
        m_currentSection = focusedControl - CONTROL_START_SECTION;
        CreateControls();
      }
      return true;
    }
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

void CGUIDialogPluginSettings::OnInitWindow()
{
  m_closeAction = "";
  m_currentSection = 0;
  m_totalSections = 1;
  CreateSections();
  CreateControls();
  CGUIDialogBoxBase::OnInitWindow();
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
bool CGUIDialogPluginSettings::ShowAndGetInput(CURL& url, bool saveToDisk /* = true */)
{
  CPluginSettings settings;
  if (settings.Load(url))
  {
    // Create the dialog
    CGUIDialogPluginSettings* pDialog = NULL;
    pDialog = (CGUIDialogPluginSettings*) g_windowManager.GetWindow(WINDOW_DIALOG_PLUGIN_SETTINGS);
    if (!pDialog)
      return false;

    // Load language strings temporarily
    DIRECTORY::CPluginDirectory::LoadPluginStrings(url);

    pDialog->m_url = url;
    pDialog->m_strHeading = pDialog->m_url.GetFileNameWithoutPath();
    CUtil::RemoveSlashAtEnd(pDialog->m_strHeading);
    pDialog->m_profile.Format("special://profile/plugin_data/%s/%s", pDialog->m_url.GetHostName().c_str(), pDialog->m_strHeading.c_str());
    pDialog->m_strHeading.Format("$LOCALIZE[1045] - %s", pDialog->m_strHeading.c_str());
    pDialog->m_addon = settings;
    pDialog->m_changed = false;
    pDialog->m_saveToDisk = saveToDisk;
    pDialog->DoModal();
    if(pDialog->m_bConfirmed)
    {
      settings = pDialog->m_addon;
      settings.Save();
    }
    return pDialog->m_bConfirmed;
  }
  else
  { // addon does not support settings, inform user
    CGUIDialogOK::ShowAndGetInput(24000,0,24030,0);
    return false;
  }
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
bool CGUIDialogPluginSettings::ShowAndGetInput(SScraperInfo& info, bool saveToDisk /* = true */)
{
  // Create the dialog
  CGUIDialogPluginSettings* pDialog = NULL;
  pDialog = (CGUIDialogPluginSettings*) g_windowManager.GetWindow(WINDOW_DIALOG_PLUGIN_SETTINGS);
  if (!pDialog)
    return false;

  pDialog->m_addon = info.settings;
  pDialog->m_strHeading.Format("$LOCALIZE[20407] - %s", info.strTitle.c_str());
  pDialog->m_profile = "";
  pDialog->m_changed = false;
  pDialog->m_saveToDisk = saveToDisk;
  pDialog->DoModal();
  if(pDialog->m_bConfirmed)
    info.settings.LoadUserXML(static_cast<CScraperSettings&>(pDialog->m_addon).GetSettings());

  return pDialog->m_bConfirmed;
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
bool CGUIDialogPluginSettings::ShowAndGetInput(CStdString& path, bool saveToDisk /* = true */)
{
  CUtil::RemoveSlashAtEnd(path);
  CScriptSettings settings;
  if (settings.Load(path))
  {
    // Create the dialog
    CGUIDialogPluginSettings* pDialog = NULL;
    pDialog = (CGUIDialogPluginSettings*) g_windowManager.GetWindow(WINDOW_DIALOG_PLUGIN_SETTINGS);
    if (!pDialog)
      return false;

    pDialog->m_url = CURL(path);

    // Path where the language strings reside
    CStdString pathToLanguageFile = path;
    CStdString pathToFallbackLanguageFile = path;
    CUtil::AddFileToFolder(pathToLanguageFile, "resources", pathToLanguageFile);
    CUtil::AddFileToFolder(pathToFallbackLanguageFile, "resources", pathToFallbackLanguageFile);
    CUtil::AddFileToFolder(pathToLanguageFile, "language", pathToLanguageFile);
    CUtil::AddFileToFolder(pathToFallbackLanguageFile, "language", pathToFallbackLanguageFile);
    CUtil::AddFileToFolder(pathToLanguageFile, g_guiSettings.GetString("locale.language"), pathToLanguageFile);
    CUtil::AddFileToFolder(pathToFallbackLanguageFile, "english", pathToFallbackLanguageFile);
    CUtil::AddFileToFolder(pathToLanguageFile, "strings.xml", pathToLanguageFile);
    CUtil::AddFileToFolder(pathToFallbackLanguageFile, "strings.xml", pathToFallbackLanguageFile);
    // Load language strings temporarily
    g_localizeStringsTemp.Load(pathToLanguageFile, pathToFallbackLanguageFile);

    pDialog->m_strHeading = pDialog->m_url.GetFileNameWithoutPath();
    CUtil::RemoveSlashAtEnd(pDialog->m_strHeading);
    pDialog->m_profile.Format("special://profile/script_data/%s", pDialog->m_strHeading.c_str());
    pDialog->m_strHeading.Format("$LOCALIZE[1049] - %s", pDialog->m_strHeading.c_str());
    pDialog->m_addon = settings;
    pDialog->m_changed = false;
    pDialog->m_saveToDisk = saveToDisk;
    pDialog->DoModal();
    if(pDialog->m_bConfirmed)
    {
      settings = pDialog->m_addon;
      settings.Save();
    }
    return pDialog->m_bConfirmed;
  }
  else
  { // addon does not support settings, inform user
    CGUIDialogOK::ShowAndGetInput(24000,0,24030,0);
    return false;
  }
}

bool CGUIDialogPluginSettings::ShowVirtualKeyboard(int iControl)
{
  int controlId = CONTROL_START_SETTING;
  bool bCloseDialog = false;

  const TiXmlElement *setting = GetFirstSetting();
  while (setting)
  {
    if (controlId == iControl)
    {
      const char *id = setting->Attribute("id");
      const char *type = setting->Attribute("type");
      CStdString value = m_settings[id];
      const CGUIControl* control = GetControl(controlId);
      if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      {
        const char *option = setting->Attribute("option");
        const char *source = setting->Attribute("source");
        CStdString label = GetString(setting->Attribute("label"));

        if (strcmp(type, "text") == 0)
        {
          // get any options
          bool bHidden  = false;
          bool bEncoded = false;
          if (option)
          {
            bHidden = (strstr(option, "hidden") != NULL);
            bEncoded = (strstr(option, "urlencoded") != NULL);
          }
          if (bEncoded)
            CUtil::URLDecode(value);

          if (CGUIDialogKeyboard::ShowAndGetInput(value, label, true, bHidden))
          {
            // if hidden hide input
            if (bHidden)
            {
              CStdString hiddenText;
              hiddenText.append(value.size(), L'*');
              ((CGUIButtonControl *)control)->SetLabel2(hiddenText);
            }
            else
              ((CGUIButtonControl*) control)->SetLabel2(value);
            if (bEncoded)
              CUtil::URLEncode(value);
          }
        }
        else if (strcmp(type, "number") == 0 && CGUIDialogNumeric::ShowAndGetNumber(value, label))
        {
          ((CGUIButtonControl*) control)->SetLabel2(value);
        }
        else if (strcmp(type, "ipaddress") == 0 && CGUIDialogNumeric::ShowAndGetIPAddress(value, label))
        {
          ((CGUIButtonControl*) control)->SetLabel2(value);
        }
        else if (strcmpi(type, "select") == 0)
        {
          CGUIDialogSelect *pDlg = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
          if (pDlg)
          {
            pDlg->SetHeading(label.c_str());
            pDlg->Reset();

            vector<CStdString> valuesVec;
            if (setting->Attribute("values"))
              CUtil::Tokenize(setting->Attribute("values"), valuesVec, "|");
            else if (setting->Attribute("lvalues"))
            { // localize
              CUtil::Tokenize(setting->Attribute("lvalues"), valuesVec, "|");
              for (unsigned int i = 0; i < valuesVec.size(); i++)
              {
                CStdString value = g_localizeStringsTemp.Get(atoi(valuesVec[i]));
                if (value.IsEmpty())
                  value = g_localizeStrings.Get(atoi(valuesVec[i]));
                valuesVec[i] = value;
              }
            }
            else if (source)
            {
              valuesVec = GetFileEnumValues(source, setting->Attribute("mask"), setting->Attribute("option"));
            }

            for (unsigned int i = 0; i < valuesVec.size(); i++)
            {
              pDlg->Add(valuesVec[i]);
              if (valuesVec[i].Equals(value))
                pDlg->SetSelected(i); // FIXME: the SetSelected() does not select "i", it always defaults to the first position
            }
            pDlg->DoModal();
            int iSelected = pDlg->GetSelectedLabel();
            if (iSelected >= 0)
            {
              value = valuesVec[iSelected];
              ((CGUIButtonControl*) control)->SetLabel2(value);
            }
          }
        }
        else if (strcmpi(type, "audio") == 0 || strcmpi(type, "video") == 0 ||
          strcmpi(type, "image") == 0 || strcmpi(type, "executable") == 0 ||
          strcmpi(type, "file") == 0 || strcmpi(type, "folder") == 0)
        {
          // setup the shares
          VECSOURCES *shares = NULL;
          if (!source || strcmpi(source, "") == 0)
            shares = g_settings.GetSourcesFromType(type);
          else
            shares = g_settings.GetSourcesFromType(source);

          VECSOURCES localShares;
          if (!shares)
          {
            VECSOURCES networkShares;
            g_mediaManager.GetLocalDrives(localShares);
            if (!source || strcmpi(source, "local") != 0)
              g_mediaManager.GetNetworkLocations(networkShares);
            localShares.insert(localShares.end(), networkShares.begin(), networkShares.end());
          }
          else // always append local drives
          {
            localShares = *shares;
            g_mediaManager.GetLocalDrives(localShares);
          }

          if (strcmpi(type, "folder") == 0)
          {
            // get any options
            bool bWriteOnly = false;
            if (option)
              bWriteOnly = (strcmpi(option, "writeable") == 0);

            if (CGUIDialogFileBrowser::ShowAndGetDirectory(localShares, label, value, bWriteOnly))
              ((CGUIButtonControl*) control)->SetLabel2(value);
          }
          else if (strcmpi(type, "image") == 0)
          {
            if (CGUIDialogFileBrowser::ShowAndGetImage(localShares, label, value))
              ((CGUIButtonControl*) control)->SetLabel2(value);
          }
          else
          {
            // set the proper mask
            CStdString strMask;
            if (setting->Attribute("mask"))
            {
              strMask = setting->Attribute("mask");
              // convert mask qualifiers
              strMask.Replace("$AUDIO", g_stSettings.m_musicExtensions);
              strMask.Replace("$VIDEO", g_stSettings.m_videoExtensions);
              strMask.Replace("$IMAGE", g_stSettings.m_pictureExtensions);
#if defined(_WIN32_WINNT)
              strMask.Replace("$EXECUTABLE", ".exe|.bat|.cmd|.py");
#else
              strMask.Replace("$EXECUTABLE", ".xbe|.py");
#endif
            }
            else
            {
              if (strcmpi(type, "video") == 0)
                strMask = g_stSettings.m_videoExtensions;
              else if (strcmpi(type, "audio") == 0)
                strMask = g_stSettings.m_musicExtensions;
              else if (strcmpi(type, "executable") == 0)
#if defined(_WIN32_WINNT)
                strMask = ".exe|.bat|.cmd|.py";
#else
                strMask = ".xbe|.py";
#endif
            }

            // get any options
            bool bUseThumbs = false;
            bool bUseFileDirectories = false;
            if (option)
            {
              vector<CStdString> options;
              StringUtils::SplitString(option, "|", options);
              bUseThumbs = find(options.begin(), options.end(), "usethumbs") != options.end();
              bUseFileDirectories = find(options.begin(), options.end(), "treatasfolder") != options.end();
            }

            if (CGUIDialogFileBrowser::ShowAndGetFile(localShares, strMask, label, value))
              ((CGUIButtonControl*) control)->SetLabel2(value);
          }
        }
        else if (strcmpi(type, "action") == 0)
        {
          if (setting->Attribute("action"))
          {
            CStdString action = NormalizePath(setting->Attribute("action"));
            if (option)
              bCloseDialog = (strcmpi(option, "close") == 0);
            g_applicationMessenger.ExecBuiltIn(action);
          }
        }
        else if (strcmp(type, "date") == 0)
        {
          CDateTime date;
          if (!value.IsEmpty())
            date.SetFromDBDate(value);
          SYSTEMTIME timedate;
          date.GetAsSystemTime(timedate);
          if(CGUIDialogNumeric::ShowAndGetDate(timedate, label))
          {
            date = timedate;
            value = date.GetAsDBDate();
            ((CGUIButtonControl*) control)->SetLabel2(value);
          }
        }
        else if (strcmp(type, "time") == 0)
        {
          SYSTEMTIME timedate;
          if (!value.IsEmpty())
          {
            // assumes HH:MM
            timedate.wHour = atoi(value.Left(2));
            timedate.wMinute = atoi(value.Right(2));
          }
          if (CGUIDialogNumeric::ShowAndGetTime(timedate, label))
          {
            value.Format("%02d:%02d", timedate.wHour, timedate.wMinute);
            ((CGUIButtonControl*) control)->SetLabel2(value);
          }
        }
      }
      else if (control->GetControlType() == CGUIControl::GUICONTROL_RADIO)
      {
        value = ((CGUIRadioButtonControl*) control)->IsSelected() ? "true" : "false";
      }
      else if (control->GetControlType() == CGUIControl::GUICONTROL_SPINEX)
      {
        if (strcmpi(type, "fileenum") == 0 || strcmpi(type, "labelenum") == 0)
          value = ((CGUISpinControlEx*) control)->GetLabel();
        else
          value.Format("%i", ((CGUISpinControlEx*) control)->GetValue());
      }
      else if (control->GetControlType() == CGUIControl::GUICONTROL_SETTINGS_SLIDER)
      {
        SetSliderTextValue(control, setting->Attribute("format"));
        value.Format("%f", ((CGUISettingsSliderControl *)control)->GetFloatValue());
      }
      m_settings[id] = value;
      SetEnabledProperty(id);
      break;
    }
    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
  return bCloseDialog;
}

void CGUIDialogPluginSettings::SaveSettings(void)
{
  for (map<CStdString, CStdString>::iterator i = m_settings.begin(); i != m_settings.end(); ++i)
    m_addon.Set(i->first, i->second);

  if (m_saveToDisk)
    m_addon.Save();
}

void CGUIDialogPluginSettings::FreeSections()
{
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_SETTINGS_AREA);
  if (group)
  {
    group->FreeResources();
    group->ClearAll();
  }
  m_settings.clear();
}

void CGUIDialogPluginSettings::FreeControls()
{
  // clear the category group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(CONTROL_SETTINGS_AREA);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
}

void CGUIDialogPluginSettings::CreateSections()
{
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_SECTION_AREA);
  CGUIButtonControl *originalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_SECTION_BUTTON);

  if (originalButton)
    originalButton->SetVisible(false);

  // clear the category group
  FreeSections();

  const TiXmlElement *settings = m_addon.GetPluginRoot();
  // grab any onclose action
  if (settings->Attribute("onclose"))
    m_closeAction = NormalizePath(settings->Attribute("onclose"));

  // grab our categories
  const TiXmlElement *category = settings->FirstChildElement("category");
  if (!category) // add a default one...
    category = m_addon.GetPluginRoot();

  int buttonID = CONTROL_START_SECTION;
  while (category)
  { // add a category
    CGUIButtonControl *button = originalButton ? originalButton->Clone() : NULL;

    CStdString label = GetString(category->Attribute("label"));
    if (label.IsEmpty())
      label = g_localizeStrings.Get(128);

    // add the category button
    if (button && group)
    {
      button->SetID(buttonID++);
      button->SetLabel(label);
      button->SetVisible(true);
      if (category->Attribute("visible"))
        button->SetVisibleCondition(g_infoManager.TranslateString(category->Attribute("visible")), false);
      if (category->Attribute("enable"))
        button->SetEnableCondition(g_infoManager.TranslateString(category->Attribute("enable")));
      group->AddControl(button);
    }

    // grab a local copy of all the settings in this category
    const TiXmlElement *setting = category->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      if (id)
      {
        m_settings[id] = m_addon.Get(id);
        SetEnabledProperty(id);
      }
      setting = setting->NextSiblingElement("setting");
    }
    category = category->NextSiblingElement("category");
  }
  m_totalSections = buttonID - CONTROL_START_SECTION;
}

void CGUIDialogPluginSettings::CreateControls()
{
  FreeControls();

  CGUISpinControlEx *pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  CGUIRadioButtonControl *pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  CGUIButtonControl *pOriginalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  CGUIImage *pOriginalImage = (CGUIImage *)GetControl(CONTROL_DEFAULT_SEPARATOR);
  CGUILabelControl *pOriginalLabel = (CGUILabelControl *)GetControl(CONTROL_DEFAULT_LABEL_SEPARATOR);
  CGUISettingsSliderControl *pOriginalSlider = (CGUISettingsSliderControl *)GetControl(CONTROL_DEFAULT_SLIDER);

  if (!pOriginalSpin || !pOriginalRadioButton || !pOriginalButton || !pOriginalImage
               || !pOriginalLabel || !pOriginalSlider)
    return;

  pOriginalSpin->SetVisible(false);
  pOriginalRadioButton->SetVisible(false);
  pOriginalButton->SetVisible(false);
  pOriginalImage->SetVisible(false);
  pOriginalLabel->SetVisible(false);
  pOriginalSlider->SetVisible(false);

  // clear the category group
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_SETTINGS_AREA);
  if (!group)
    return;

  // set our dialog heading
  SET_CONTROL_LABEL(CONTROL_HEADING_LABEL, m_strHeading);

  CGUIControl* pControl = NULL;
  int controlId = CONTROL_START_SETTING;
  const TiXmlElement *setting = GetFirstSetting();
  while (setting)
  {
    const char *type = setting->Attribute("type");
    const char *id = setting->Attribute("id");
    CStdString values;
    if (setting->Attribute("values"))
      values = setting->Attribute("values");
    CStdString lvalues;
    if (setting->Attribute("lvalues"))
      lvalues = setting->Attribute("lvalues");
    CStdString entries;
    if (setting->Attribute("entries"))
      entries = setting->Attribute("entries");
    CStdString defaultValue;
    if (setting->Attribute("default"))
      defaultValue = setting->Attribute("default");
    const char *subsetting = setting->Attribute("subsetting");
    CStdString label = GetString(setting->Attribute("label"), subsetting && 0 == strcmpi(subsetting, "true"));

    bool bSort=false;
    const char *sort = setting->Attribute("sort");
    if (sort && (strcmp(sort, "yes") == 0))
      bSort=true;

    if (type)
    {
      CStdString value;
      if (id)
        value = m_settings[id];

      if (strcmpi(type, "text") == 0 || strcmpi(type, "ipaddress") == 0 ||
        strcmpi(type, "number") == 0 ||strcmpi(type, "video") == 0 ||
        strcmpi(type, "audio") == 0 || strcmpi(type, "image") == 0 ||
        strcmpi(type, "folder") == 0 || strcmpi(type, "executable") == 0 ||
        strcmpi(type, "file") == 0 || strcmpi(type, "action") == 0 ||
        strcmpi(type, "date") == 0 || strcmpi(type, "time") == 0 ||
        strcmpi(type, "select") == 0)
      {
        pControl = new CGUIButtonControl(*pOriginalButton);
        if (!pControl) return;
        ((CGUIButtonControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
        ((CGUIButtonControl *)pControl)->SetLabel(label);
        if (id)
        {
          // get any option to test for hidden
          const char *option = setting->Attribute("option");
          if (option && (strstr(option, "urlencoded")))
            CUtil::URLDecode(value);
          if (option && (strstr(option, "hidden")))
          {
            CStdString hiddenText;
            hiddenText.append(value.size(), L'*');
            ((CGUIButtonControl *)pControl)->SetLabel2(hiddenText);
          }
          else
            ((CGUIButtonControl *)pControl)->SetLabel2(value);
        }
        else if (!defaultValue.IsEmpty())
          ((CGUIButtonControl *)pControl)->SetLabel2(defaultValue);
      }
      else if (strcmpi(type, "bool") == 0)
      {
        pControl = new CGUIRadioButtonControl(*pOriginalRadioButton);
        if (!pControl) return;
        ((CGUIRadioButtonControl *)pControl)->SetLabel(label);
        ((CGUIRadioButtonControl *)pControl)->SetSelected(m_settings[id] == "true");
      }
      else if (strcmpi(type, "enum") == 0 || strcmpi(type, "labelenum") == 0)
      {
        vector<CStdString> valuesVec;
        vector<CStdString> entryVec;

        pControl = new CGUISpinControlEx(*pOriginalSpin);
        if (!pControl) return;
        ((CGUISpinControlEx *)pControl)->SetText(label);

        if (!lvalues.IsEmpty())
          CUtil::Tokenize(lvalues, valuesVec, "|");
        else if (values.Equals("$HOURS"))
        {
          for (unsigned int i = 0; i < 24; i++)
          {
            CDateTime time(2000, 1, 1, i, 0, 0);
            valuesVec.push_back(g_infoManager.LocalizeTime(time, TIME_FORMAT_HH_MM_XX));
          }
        }
        else
          CUtil::Tokenize(values, valuesVec, "|");
        if (!entries.IsEmpty())
          CUtil::Tokenize(entries, entryVec, "|");

        if(bSort && strcmpi(type, "labelenum") == 0)
          std::sort(valuesVec.begin(), valuesVec.end(), sortstringbyname());

        for (unsigned int i = 0; i < valuesVec.size(); i++)
        {
          int iAdd = i;
          if (entryVec.size() > i)
            iAdd = atoi(entryVec[i]);
          if (!lvalues.IsEmpty())
          {
            CStdString replace = g_localizeStringsTemp.Get(atoi(valuesVec[i]));
            if (replace.IsEmpty())
              replace = g_localizeStrings.Get(atoi(valuesVec[i]));
            ((CGUISpinControlEx *)pControl)->AddLabel(replace, iAdd);
          }
          else
            ((CGUISpinControlEx *)pControl)->AddLabel(valuesVec[i], iAdd);
        }
        if (strcmpi(type, "labelenum") == 0)
        { // need to run through all our settings and find the one that matches
          ((CGUISpinControlEx*) pControl)->SetValueFromLabel(m_settings[id]);
        }
        else
          ((CGUISpinControlEx*) pControl)->SetValue(atoi(m_settings[id]));

      }
      else if (strcmpi(type, "fileenum") == 0)
      {
        pControl = new CGUISpinControlEx(*pOriginalSpin);
        if (!pControl) return;
        ((CGUISpinControlEx *)pControl)->SetText(label);
        ((CGUISpinControlEx *)pControl)->SetFloatValue(1.0f);

        vector<CStdString> items = GetFileEnumValues(values, setting->Attribute("mask"), setting->Attribute("option"));
        for (unsigned int i = 0; i < items.size(); ++i)
        {
          ((CGUISpinControlEx *)pControl)->AddLabel(items[i], i);
          if (items[i].Equals(m_settings[id]))
            ((CGUISpinControlEx *)pControl)->SetValue(i);
        }
      }
      else if (strcmpi(type, "slider") == 0)
      {
        pControl = new CGUISettingsSliderControl(*pOriginalSlider);
        if (!pControl) return;
        ((CGUISettingsSliderControl *)pControl)->SetText(label);

        float fMin = 0.0f;
        float fMax = 100.0f;
        float fInc = 1.0f;
        vector<CStdString> range;
        StringUtils::SplitString(setting->Attribute("range"), ",", range);
        if (range.size() > 1)
        {
          fMin = (float)atof(range[0]);
          if (range.size() > 2)
          {
            fMax = (float)atof(range[2]);
            fInc = (float)atof(range[1]);
          }
          else
            fMax = (float)atof(range[1]);
        }

        ((CGUISettingsSliderControl *)pControl)->SetType(SPIN_CONTROL_TYPE_FLOAT);
        ((CGUISettingsSliderControl *)pControl)->SetFloatRange(fMin, fMax);
        ((CGUISettingsSliderControl *)pControl)->SetFloatInterval(fInc);
        ((CGUISettingsSliderControl *)pControl)->SetFloatValue((float)atof(m_settings[id]));
        SetSliderTextValue(pControl, setting->Attribute("format"));
      }
      else if (strcmpi(type, "lsep") == 0)
      {
        pControl = new CGUILabelControl(*pOriginalLabel);
        if (pControl)
          ((CGUILabelControl *)pControl)->SetLabel(label);
      }
      else if (strcmpi(type, "sep") == 0)
        pControl = new CGUIImage(*pOriginalImage);
    }

    if (pControl)
    {
      pControl->SetWidth(group->GetWidth());
      pControl->SetVisible(true);
      if (setting->Attribute("visible"))
      {
        CGUIInfoBool allowHiddenFocus(false);
        if (setting->Attribute("allowhiddenfocus"))
          allowHiddenFocus.Parse(setting->Attribute("allowhiddenfocus"));
        pControl->SetVisibleCondition(g_infoManager.TranslateString(setting->Attribute("visible")), allowHiddenFocus);
      }
      if (setting->Attribute("enable"))
        pControl->SetEnableCondition(g_infoManager.TranslateString(setting->Attribute("enable")));
      pControl->SetID(controlId);
      pControl->AllocResources();
      group->AddControl(pControl);
      pControl = NULL;
    }

    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
}

vector<CStdString> CGUIDialogPluginSettings::GetFileEnumValues(const CStdString &path, const CStdString &mask, const CStdString &options) const
{
  // Create our base path, used for type "fileenum" settings
  CStdString fullPath;
  // replace $CWD with the path and $PROFILE with the profile path of the addon
  if (path.Find("$PROFILE") >= 0 || path.Find("$CWD") >= 0)
    fullPath = NormalizePath(path);
  else
  {
    CStdString url = m_url.Get();
    // replace plugin:// with the plugins root path
    url.Replace("plugin://", "special://home/plugins/");
    fullPath = CUtil::ValidatePath(CUtil::AddFileToFolder(url, path));
  }

  bool hideExtensions = (options.CompareNoCase("hideext") == 0);
  // fetch directory
  CFileItemList items;
  if (!mask.IsEmpty())
    CDirectory::GetDirectory(fullPath, items, mask);
  else
    CDirectory::GetDirectory(fullPath, items);

  vector<CStdString> values;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if ((mask.Equals("/") && pItem->m_bIsFolder) || !pItem->m_bIsFolder)
    {
      if (hideExtensions)
        pItem->RemoveExtension();
      values.push_back(pItem->GetLabel());
    }
  }
  return values;
}

void CGUIDialogPluginSettings::SetEnabledProperty(const CStdString &id)
{
  SetProperty(id, m_settings[id]);
}

CStdString CGUIDialogPluginSettings::GetString(const char *value, bool subSetting) const
{
  if (!value)
    return "";
  int id = atoi(value);
  CStdString prefix(subSetting ? "- " : "");
  if (id > 0)
    return prefix + g_localizeStringsTemp.Get(id);

  // localize values
  CStdString strValue = CGUIInfoLabel::ReplaceLocalize(value);
  strValue = CGUIInfoLabel::ReplaceAddonStrings(strValue);
  return prefix + strValue;
}

void CGUIDialogPluginSettings::SetSliderTextValue(const CGUIControl *control, const char *format)
{
  if (format)
  {
    CStdString strValue;
    vector<CStdString> formats;
    StringUtils::SplitString(format, ",", formats);

    if (formats.size() == 3 && !formats[2].IsEmpty() && ((CGUISettingsSliderControl *)control)->GetProportion() == 1.0f)
      strValue.Format(GetString(formats[2]), ((CGUISettingsSliderControl *)control)->GetFloatValue());
    else if (formats.size() >= 2 && !formats[1].IsEmpty() && ((CGUISettingsSliderControl *)control)->GetProportion() == 0.0f)
      strValue.Format(GetString(formats[1]), ((CGUISettingsSliderControl *)control)->GetFloatValue());
    else
      strValue.Format(GetString(formats[0]), ((CGUISettingsSliderControl *)control)->GetFloatValue());

    ((CGUISettingsSliderControl *)control)->SetTextValue(strValue);
  }
}

CStdString CGUIDialogPluginSettings::NormalizePath(const char *value) const
{
  CStdString normalPath = value;
  CStdString path = m_url.Get();
  CUtil::RemoveSlashAtEnd(path);
  // we need to change plugin:// protocol if we are using runscript
  if (m_url.GetProtocol().Equals("plugin") && !normalPath.Left(10).Equals("runplugin("))
    path.Replace("plugin://", "Q:\\plugins\\");
  // replace $CWD with the addon's path
  normalPath.Replace("$CWD", path);
  // replace $PROFILE with the profile path of the addon
  normalPath.Replace("$PROFILE", m_profile);
  // replace $ID with the addon's path or path to default.py
  if (!m_url.GetProtocol().Equals("plugin") || (m_url.GetProtocol().Equals("plugin") && normalPath.Left(10).Equals("runscript(")))
    path = CUtil::AddFileToFolder(path, "default.py");
  normalPath.Replace("$ID", path);
  // validatePath will not work on action commands, so replace / here
  normalPath.Replace("/", "\\");

  return CUtil::ValidatePath(normalPath, true);
}

// Go over all the settings and set their default values
void CGUIDialogPluginSettings::SetDefaults()
{
  const TiXmlElement *category = m_addon.GetPluginRoot()->FirstChildElement("category");
  if (!category) // add a default one...
    category = m_addon.GetPluginRoot();

  while (category)
  {
    const TiXmlElement *setting = category->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      const char *type = setting->Attribute("type");
      const char *value = setting->Attribute("default");
      if (id)
      {
        if (value)
          m_settings[id] = value;
        else if (0 == strcmpi(type, "bool"))
          m_settings[id] = "false";
        else if (0 == strcmpi(type, "slider") || 0 == strcmpi(type, "enum"))
          m_settings[id] = "0";
        else if (0 != strcmpi(type, "action"))
          m_settings[id] = "";
        SetEnabledProperty(id);
      }
      setting = setting->NextSiblingElement("setting");
    }
    category = category->NextSiblingElement("category");
  }
  CreateControls();
}

const TiXmlElement *CGUIDialogPluginSettings::GetFirstSetting()
{
  const TiXmlElement *category = m_addon.GetPluginRoot()->FirstChildElement("category");
  if (!category)
    category = m_addon.GetPluginRoot();
  for (unsigned int i = 0; i < m_currentSection && category; i++)
    category = category->NextSiblingElement("category");
  if (category)
    return category->FirstChildElement("setting");
  return NULL;
}

void CGUIDialogPluginSettings::Render()
{
  // update status of current section button
  bool alphaFaded = false;
  CGUIControl *control = GetFirstFocusableControl(CONTROL_START_SECTION + m_currentSection);
  if (control && !control->HasFocus())
  {
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetAlpha(0x80);
      alphaFaded = true;
    }
    else if (control->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetSelected(true);
      alphaFaded = true;
    }
  }
  CGUIDialogBoxBase::Render();
  if (alphaFaded && m_bRunning) // dialog may close during Render()
  {
    control->SetFocus(false);
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      ((CGUIButtonControl *)control)->SetAlpha(0xFF);
    else
      ((CGUIButtonControl *)control)->SetSelected(false);
  }
}

CURL CGUIDialogPluginSettings::m_url;

