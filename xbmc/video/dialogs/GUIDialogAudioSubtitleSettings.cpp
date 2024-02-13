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

#include "system.h"
#include "video/dialogs/GUIDialogAudioSubtitleSettings.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "GUIPassword.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "Application.h"
#include "video/VideoDatabase.h"
#include "XBAudioConfig.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "URL.h"
#include "FileItem.h"
#include "addons/Skin.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "guilib/LocalizeStrings.h"

#include "defs_from_settings.h"

using namespace std;
using namespace XFILE;

#ifdef HAS_VIDEO_PLAYBACK
extern void xbox_audio_switch_channel(int iAudioStream, bool bAudioOnAllSpeakers); //lowlevel audio
#endif

CGUIDialogAudioSubtitleSettings::CGUIDialogAudioSubtitleSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_AUDIO_OSD_SETTINGS, "VideoOSDSettings.xml")
{
}

CGUIDialogAudioSubtitleSettings::~CGUIDialogAudioSubtitleSettings(void)
{
}

#define AUDIO_SETTINGS_VOLUME             1
#define AUDIO_SETTINGS_VOLUME_AMPLIFICATION 2
#define AUDIO_SETTINGS_DELAY              3
#define AUDIO_SETTINGS_STREAM             4
#define AUDIO_SETTINGS_OUTPUT_TO_ALL_SPEAKERS 5
#define AUDIO_SETTINGS_DIGITAL_ANALOG 6

// separator 7
#define SUBTITLE_SETTINGS_ENABLE          8
#define SUBTITLE_SETTINGS_DELAY           9
#define SUBTITLE_SETTINGS_STREAM          10
#define SUBTITLE_SETTINGS_BROWSER        11
#define AUDIO_SETTINGS_MAKE_DEFAULT      12

void CGUIDialogAudioSubtitleSettings::CreateSettings()
{
  m_usePopupSliders = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  // clear out any old settings
  m_settings.clear();
  // create our settings
  m_volume = g_application.GetVolume(false) * 0.01f;
  AddSlider(AUDIO_SETTINGS_VOLUME, 13376, &m_volume, VOLUME_MINIMUM * 0.01f, (VOLUME_MAXIMUM - VOLUME_MINIMUM) * 0.0001f, VOLUME_MAXIMUM * 0.01f, FormatDecibel, false);
  AddSlider(AUDIO_SETTINGS_VOLUME_AMPLIFICATION, 660, &CMediaSettings::Get().GetCurrentVideoSettings().m_VolumeAmplification, VOLUME_DRC_MINIMUM * 0.01f, (VOLUME_DRC_MAXIMUM - VOLUME_DRC_MINIMUM) * 0.0005f, VOLUME_DRC_MAXIMUM * 0.01f, FormatDecibel, false);
  AddSlider(AUDIO_SETTINGS_DELAY, 297, &CMediaSettings::Get().GetCurrentVideoSettings().m_AudioDelay, -g_advancedSettings.m_videoAudioDelayRange, .025f, g_advancedSettings.m_videoAudioDelayRange, FormatDelay);
  AddAudioStreams(AUDIO_SETTINGS_STREAM);

  // only show stuff available in digital mode if we have digital output
  if(g_audioConfig.HasDigitalOutput())
  {
    AddBool(AUDIO_SETTINGS_OUTPUT_TO_ALL_SPEAKERS, 38621, &CMediaSettings::Get().GetCurrentVideoSettings().m_OutputToAllSpeakers, CSettings::Get().GetInt("audiooutput.mode") == AUDIO_DIGITAL);

    int settings[2] = { 38630, 38631 }; //ANALOG, DIGITAL
    m_outputmode = CSettings::Get().GetInt("audiooutput.mode");
    AddSpin(AUDIO_SETTINGS_DIGITAL_ANALOG, 38629, &m_outputmode, 2, settings);
  }

  AddSeparator(7);
  m_subtitleVisible = g_application.m_pPlayer->GetSubtitleVisible();
  AddBool(SUBTITLE_SETTINGS_ENABLE, 13397, &m_subtitleVisible);
  AddSlider(SUBTITLE_SETTINGS_DELAY, 22006, &CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleDelay, -g_advancedSettings.m_videoSubsDelayRange, 0.1f, g_advancedSettings.m_videoSubsDelayRange, FormatDelay);
  AddSubtitleStreams(SUBTITLE_SETTINGS_STREAM);
  AddButton(SUBTITLE_SETTINGS_BROWSER,13250);
  AddButton(AUDIO_SETTINGS_MAKE_DEFAULT, 12376);
}

void CGUIDialogAudioSubtitleSettings::AddAudioStreams(unsigned int id)
{
  SettingInfo setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(460);
  setting.type = SettingInfo::SPIN;
  setting.min = 0;
  setting.data = &m_audioStream;

  // get the number of audio strams for the current movie
  setting.max = (float)g_application.m_pPlayer->GetAudioStreamCount() - 1;
  m_audioStream = g_application.m_pPlayer->GetAudioStream();

  if( m_audioStream < 0 ) m_audioStream = 0;

  // check if we have a single, stereo stream, and if so, allow us to split into
  // left, right or both
  if (!setting.max)
  {
    CStdString strAudioInfo;
    g_application.m_pPlayer->GetAudioInfo(strAudioInfo);
    int iNumChannels = atoi(strAudioInfo.Right(strAudioInfo.size() - strAudioInfo.Find("chns:") - 5).c_str());
    CStdString strAudioCodec = strAudioInfo.Mid(7, strAudioInfo.Find(") VBR") - 5);
    bool bDTS = strstr(strAudioCodec.c_str(), "DTS") != 0;
    bool bAC3 = strstr(strAudioCodec.c_str(), "AC3") != 0;
    if (iNumChannels == 2 && !(bDTS || bAC3))
    { // ok, enable these options
/*      if (CMediaSettings::Get().GetCurrentVideoSettings().m_AudioStream == -1)
      { // default to stereo stream
        CMediaSettings::Get().GetCurrentVideoSettings().m_AudioStream = 0;
      }*/
      setting.max = 2;
      for (int i = 0; i <= setting.max; i++)
        setting.entry.push_back(make_pair(setting.entry.size(), g_localizeStrings.Get(13320 + i)));
      m_audioStream = -CMediaSettings::Get().GetCurrentVideoSettings().m_AudioStream - 1;
      m_settings.push_back(setting);
      return;
    }
  }

  // cycle through each audio stream and add it to our list control
  for (int i = 0; i <= setting.max; ++i)
  {
    CStdString strItem;
    CStdString strName;
    g_application.m_pPlayer->GetAudioStreamName(i, strName);
    if (strName.length() == 0)
      strName = "Unnamed";

    strItem.Format("%s (%i/%i)", strName.c_str(), i + 1, (int)setting.max + 1);
    setting.entry.push_back(make_pair(setting.entry.size(), strItem));
  }

  if( setting.max < 0 )
  {
    setting.max = 0;
    setting.entry.push_back(make_pair(setting.entry.size(), g_localizeStrings.Get(231)));
  }

  m_settings.push_back(setting);
}

void CGUIDialogAudioSubtitleSettings::AddSubtitleStreams(unsigned int id)
{
  SettingInfo setting;

  setting.id = id;
  setting.name = g_localizeStrings.Get(462);
  setting.type = SettingInfo::SPIN;
  setting.min = 0;
  setting.data = &m_subtitleStream;
  m_subtitleStream = g_application.m_pPlayer->GetSubtitle();

  if(m_subtitleStream < 0) m_subtitleStream = 0;

  // get the number of audio strams for the current movie
  setting.max = (float)g_application.m_pPlayer->GetSubtitleCount() - 1;

  // cycle through each subtitle and add it to our entry list
  for (int i = 0; i <= setting.max; ++i)
  {
    CStdString strItem;
    CStdString strName;
    g_application.m_pPlayer->GetSubtitleName(i, strName);
    if (strName.length() == 0)
      strName = "Unnamed";

    CStdString strLanguage;
    g_application.m_pPlayer->GetSubtitleLanguage(i, strLanguage);

    if (strName != strLanguage)
      strName.Format("%s [%s]", strName.c_str(), strLanguage.c_str());

    strItem.Format("%s (%i/%i)", strName.c_str(), i + 1, (int)setting.max + 1);

    setting.entry.push_back(make_pair(setting.entry.size(), strItem));
  }

  if (setting.max < 0)
  { // no subtitle streams - just add a "None" entry
    m_subtitleStream = 0;
    setting.max = 0;
    setting.entry.push_back(make_pair(setting.entry.size(), g_localizeStrings.Get(231)));
  }
  m_settings.push_back(setting);
}

#define UPDATE_SETTING_LABEL(x) \
    if (fabs(x) < setting.interval) \
      x = 0; \
    setting.format = "%2.3fs"; \
    if (x < 0)  \
      setting.format = g_localizeStrings.Get(22004); \
    if (x > 0)\
      setting.format = g_localizeStrings.Get(22005); 

void CGUIDialogAudioSubtitleSettings::OnSettingChanged(SettingInfo &setting)
{
  // check and update anything that needs it
  if (setting.id == AUDIO_SETTINGS_VOLUME)
  {
    int volume = (long)(m_volume * 100.0f);
    g_application.SetVolume(int(((float)(volume - VOLUME_MINIMUM)) / (VOLUME_MAXIMUM - VOLUME_MINIMUM)*100.0f + 0.5f));
  }
  else if (setting.id == AUDIO_SETTINGS_VOLUME_AMPLIFICATION)
  {
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->SetDynamicRangeCompression((long)(CMediaSettings::Get().GetCurrentVideoSettings().m_VolumeAmplification * 100));
  }
  else if (setting.id == AUDIO_SETTINGS_DELAY)
  {
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->SetAVDelay(CMediaSettings::Get().GetCurrentVideoSettings().m_AudioDelay);
  }
  else if (setting.id == AUDIO_SETTINGS_STREAM)
  {
    // first check if it's a stereo track that we can change between stereo, left and right
    if (g_application.m_pPlayer->GetAudioStreamCount() == 1)
    {
      if (setting.max == 2)
      { // we're in the case we want - call the code to switch channels etc.
        // update the screen setting...
        CMediaSettings::Get().GetCurrentVideoSettings().m_AudioStream = -1 - m_audioStream;
        // call monkeyh1's code here...
        bool bAudioOnAllSpeakers = (CSettings::Get().GetInt("audiooutput.mode") == AUDIO_DIGITAL) && CMediaSettings::Get().GetCurrentVideoSettings().m_OutputToAllSpeakers;
#if defined(HAS_VIDEO_PLAYBACK) && defined(HAS_XBOX_HARDWARE)
        xbox_audio_switch_channel(m_audioStream, bAudioOnAllSpeakers);
#endif
        return;
      }
    }
    // only change the audio stream if a different one has been asked for
    if (g_application.m_pPlayer->GetAudioStream() != m_audioStream)
    {
      CMediaSettings::Get().GetCurrentVideoSettings().m_AudioStream = m_audioStream;
      g_application.m_pPlayer->SetAudioStream(m_audioStream);    // Set the audio stream to the one selected
    }
  }
  else if (setting.id == AUDIO_SETTINGS_OUTPUT_TO_ALL_SPEAKERS)
  {
    g_application.Restart();
  }
  else if (setting.id == AUDIO_SETTINGS_DIGITAL_ANALOG)
  {
    if(m_outputmode == 0) // might be unneccesary (indexes match), but just for clearity
      CSettings::Get().SetInt("audiooutput.mode", AUDIO_ANALOG);
    else
      CSettings::Get().SetInt("audiooutput.mode", AUDIO_DIGITAL);

    EnableSettings(AUDIO_SETTINGS_OUTPUT_TO_ALL_SPEAKERS, CSettings::Get().GetInt("audiooutput.mode") == AUDIO_DIGITAL);
    g_application.Restart();
  }
  else if (setting.id == SUBTITLE_SETTINGS_ENABLE)
  {
    CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleOn = m_subtitleVisible;
    g_application.m_pPlayer->SetSubtitleVisible(CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleOn);
    if (g_application.GetCurrentPlayer() == EPC_MPLAYER && !CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleCached && CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleOn)
    {
      g_application.Restart(true); // cache subtitles
      Close();
    }
  }
  else if (setting.id == SUBTITLE_SETTINGS_DELAY)
  {
    g_application.m_pPlayer->SetSubTitleDelay(CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleDelay);
  }
  else if (setting.id == SUBTITLE_SETTINGS_STREAM && setting.max > 0)
  {
    CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleStream = m_subtitleStream;
    g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
  }
  else if (setting.id == SUBTITLE_SETTINGS_BROWSER)
  {
    CStdString strPath;
    if (URIUtils::IsInRAR(g_application.CurrentFileItem().GetPath()) || URIUtils::IsInZIP(g_application.CurrentFileItem().GetPath()))
    {
      CURL url(g_application.CurrentFileItem().GetPath());
      strPath = url.GetHostName();
    }
    else
      strPath = g_application.CurrentFileItem().GetPath();

    CStdString strMask = ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.aqt|.jss|.ass|.idx|.rar|.zip";
    if (g_application.GetCurrentPlayer() == EPC_DVDPLAYER)
      strMask = ".srt|.rar|.zip|.ifo|.smi|.sub|.idx|.ass|.ssa|.txt";
    VECSOURCES shares(*CMediaSourceSettings::Get().GetSources("video"));
    if (CMediaSettings::Get().GetAdditionalSubtitleDirectoryChecked() != -1 && !CSettings::Get().GetString("subtitles.custompath").empty())
    {
      CMediaSource share;
      std::vector<CStdString> paths;
      CStdString strPath1;
      URIUtils::GetDirectory(strPath,strPath1);
      paths.push_back(strPath1);
      strPath1 = CSettings::Get().GetString("subtitles.custompath");
      paths.push_back(CSettings::Get().GetString("subtitles.custompath"));
      share.FromNameAndPaths("video",g_localizeStrings.Get(21367),paths);
      // hack
      CMediaSourceSettings::Get().AddShare("video", share);
      strPath = share.strPath;
      URIUtils::AddSlashAtEnd(strPath);
    }
    if (CGUIDialogFileBrowser::ShowAndGetFile(*CMediaSourceSettings::Get().GetSources("video"),strMask,g_localizeStrings.Get(293),strPath,false,true)) // "subtitles"
    {
      CStdString strExt;
      URIUtils::GetExtension(strPath,strExt);
      if (strExt.CompareNoCase(".idx") == 0 || strExt.CompareNoCase(".sub") == 0)
      {
        // else get current position
        double time = g_application.GetTime();

        // Playback could end and delete m_pPlayer while dialog is up so make sure it's valid
       	if (g_application.m_pPlayer)
        {
          // get player state, needed for dvd's
          CStdString state = g_application.m_pPlayer->GetPlayerState();
          
          if (g_application.GetCurrentPlayer() == EPC_MPLAYER)
              g_application.m_pPlayer->CloseFile(); // to conserve memory if unraring
              
          if (CFile::Cache(strPath,"special://temp/subtitle"+strExt+".keep"))
          {
            CStdString strPath2;
            CStdString strPath3;
            if (strExt.CompareNoCase(".idx") == 0)
            {
              strPath2 = URIUtils::ReplaceExtension(strPath,".sub");
              strPath3 = "special://temp/subtitle.sub.keep";
            }
            else
            {
              strPath2 = URIUtils::ReplaceExtension(strPath,".idx");
              if (!CFile::Exists(strPath2) && (URIUtils::IsInRAR(strPath2) || URIUtils::IsInZIP(strPath2)))
              {
                CStdString strFileName = URIUtils::GetFileName(strPath);
                URIUtils::GetDirectory(strPath,strPath3);
                URIUtils::GetParentPath(strPath3,strPath2);
                URIUtils::AddFileToFolder(strPath2,strFileName,strPath2);
                strPath2 = URIUtils::ReplaceExtension(strPath2,".idx");
              }
              strPath3 = "special://temp/subtitle.idx.keep";
            }
            if (CFile::Exists(strPath2))
              CFile::Cache(strPath2,strPath3);
            else
            {
              CFileItemList items;
              CStdString strDir,strFileNameNoExtNoCase;
              URIUtils::Split(strPath,strDir,strPath3);
              strFileNameNoExtNoCase = URIUtils::ReplaceExtension(strPath3,".");
              strFileNameNoExtNoCase.ToLower();
              URIUtils::GetDirectory(strPath,strDir);
              CDirectory::GetDirectory(strDir,items,".rar|.zip",XFILE::DIR_FLAG_NO_FILE_DIRS,true);
              for (int i=0;i<items.Size();++i)
                CUtil::CacheRarSubtitles(items[i]->GetPath(),strFileNameNoExtNoCase);
            }
            CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleOn = true;

            if (g_application.GetCurrentPlayer() == EPC_MPLAYER)
            {
              CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleCached = false;
              // reopen the file
              if ( g_application.PlayFile(g_application.CurrentFileItem(), true) && g_application.m_pPlayer )
              {
                // and seek to the position
                g_application.m_pPlayer->SetPlayerState(state);
                g_application.SeekTime(time);
              }
            }
            else
            {
              CStdString strSub("special://temp/subtitle.sub");
              CStdString strIdx("special://temp/subtitle.idx");
              CFile::Delete(strSub);
              CFile::Delete(strIdx);
              CFile::Rename(strSub + ".keep", strSub);
              CFile::Rename(strIdx + ".keep", strIdx);

              int id = g_application.m_pPlayer->AddSubtitle("special://temp/subtitle.idx");
              if(id >= 0)
              {

                m_subtitleStream = id;
                g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
                g_application.m_pPlayer->SetSubtitleVisible(true);
              }

              Close();
            }
          }
        }
      }
      else
      {
        m_subtitleStream = g_application.m_pPlayer->GetSubtitleCount();
        CStdString strExt;
        URIUtils::GetExtension(strPath,strExt);
        if (CFile::Cache(strPath,"special://temp/subtitle.browsed"+strExt))
        {
          int id = g_application.m_pPlayer->AddSubtitle("special://temp/subtitle.browsed"+strExt);
          if(id >= 0)
          {
            m_subtitleStream = id;
            g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
            g_application.m_pPlayer->SetSubtitleVisible(true);
          }
        }

        Close();
      }
      CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleCached = true;
    }
  }
  else if (setting.id == AUDIO_SETTINGS_MAKE_DEFAULT)
  {
    if (CProfilesManager::Get().GetCurrentProfile().settingsLocked() &&
        CProfilesManager::Get().GetMasterProfile().getLockMode() != ::LOCK_MODE_EVERYONE)
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return;

    // prompt user if they are sure
    if (CGUIDialogYesNo::ShowAndGetInput(12376, 750, 0, 12377))
    { // reset the settings
      CVideoDatabase db;
      db.Open();
      db.EraseVideoSettings();
      db.Close();
      CMediaSettings::Get().GetDefaultVideoSettings() = CMediaSettings::Get().GetCurrentVideoSettings();
      CMediaSettings::Get().GetDefaultVideoSettings().m_SubtitleStream = -1;
      CMediaSettings::Get().GetDefaultVideoSettings().m_AudioStream = -1;
      CSettings::Get().Save();
    }
  }
}

void CGUIDialogAudioSubtitleSettings::FrameMove()
{
  m_volume = g_application.GetVolume(false) * 0.01f;
  UpdateSetting(AUDIO_SETTINGS_VOLUME);
  if (g_application.m_pPlayer)
  {
    // these settings can change on the fly
    UpdateSetting(AUDIO_SETTINGS_DELAY);
    UpdateSetting(SUBTITLE_SETTINGS_ENABLE);
    UpdateSetting(SUBTITLE_SETTINGS_DELAY);
  }
  CGUIDialogSettings::FrameMove();
}

CStdString CGUIDialogAudioSubtitleSettings::FormatDecibel(float value, float interval)
{
  CStdString text;
  text.Format("%2.1f dB", value);
  return text;
}

CStdString CGUIDialogAudioSubtitleSettings::FormatDelay(float value, float interval)
{
  CStdString text;
  if (fabs(value) < 0.5f*interval)
    text = "0.000s";
  else if (value < 0)
    text.Format(g_localizeStrings.Get(22004).c_str(), fabs(value));
  else
    text.Format(g_localizeStrings.Get(22005).c_str(), value);
  return text;
}

