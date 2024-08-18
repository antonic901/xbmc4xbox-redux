/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "GUIDialogAudioSubtitleSettings.h"
#include "Application.h"
#include "FileItem.h"
#include "GUIPassword.h"
#include "URL.h"
#include "addons/Skin.h"
#include "cores/IPlayer.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"
#include "Util.h"
#ifdef _XBOX
#include "filesystem/Directory.h"
#include "XBAudioConfig.h"
#include "defs_from_settings.h"
#endif

#define SETTING_AUDIO_VOLUME                   "audio.volume"
#define SETTING_AUDIO_VOLUME_AMPLIFICATION     "audio.volumeamplification"
#define SETTING_AUDIO_DELAY                    "audio.delay"
#define SETTING_AUDIO_STREAM                   "audio.stream"
#define SETTING_AUDIO_OUTPUT_TO_ALL_SPEAKERS   "audio.outputtoallspeakers"
#define SETTING_AUDIO_DIGITAL_ANALOG           "audio.digitalanalog"

#define SETTING_SUBTITLE_ENABLE                "subtitles.enable"
#define SETTING_SUBTITLE_DELAY                 "subtitles.delay"
#define SETTING_SUBTITLE_STREAM                "subtitles.stream"
#define SETTING_SUBTITLE_BROWSER               "subtitles.browser"

#define SETTING_AUDIO_MAKE_DEFAULT             "audio.makedefault"

using namespace std;

#ifdef HAS_VIDEO_PLAYBACK
extern void xbox_audio_switch_channel(int iAudioStream, bool bAudioOnAllSpeakers); // lowlevel audio
#endif

CGUIDialogAudioSubtitleSettings::CGUIDialogAudioSubtitleSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_AUDIO_OSD_SETTINGS, "DialogSettings.xml"),
    m_outputmode(0)
{ }

CGUIDialogAudioSubtitleSettings::~CGUIDialogAudioSubtitleSettings()
{ }

void CGUIDialogAudioSubtitleSettings::FrameMove()
{
  // update the volume setting if necessary
  float newVolume = g_application.GetVolume(false) * 0.01f;
  if (newVolume != m_volume)
    m_settingsManager->SetNumber(SETTING_AUDIO_VOLUME, newVolume);

  if (g_application.m_pPlayer)
  {
    const CVideoSettings &videoSettings = CMediaSettings::Get().GetCurrentVideoSettings();

    // these settings can change on the fly
    m_settingsManager->SetNumber(SETTING_AUDIO_DELAY, videoSettings.m_AudioDelay);
    m_settingsManager->SetInt(SETTING_AUDIO_STREAM, g_application.m_pPlayer->GetAudioStream());
    m_settingsManager->SetBool(SETTING_AUDIO_OUTPUT_TO_ALL_SPEAKERS, videoSettings.m_OutputToAllSpeakers);
    m_settingsManager->SetInt(SETTING_AUDIO_DIGITAL_ANALOG, CSettings::GetInstance().GetInt("audiooutput.mode"));

    m_settingsManager->SetBool(SETTING_SUBTITLE_ENABLE, videoSettings.m_SubtitleOn);
    m_settingsManager->SetNumber(SETTING_SUBTITLE_DELAY, videoSettings.m_SubtitleDelay);
    m_settingsManager->SetInt(SETTING_SUBTITLE_STREAM, g_application.m_pPlayer->GetSubtitle());
  }

  CGUIDialogSettingsManualBase::FrameMove();
}

std::string CGUIDialogAudioSubtitleSettings::FormatDelay(float value, float interval)
{
  if (fabs(value) < 0.5f * interval)
    return StringUtils::Format(g_localizeStrings.Get(22003).c_str(), 0.0);
  if (value < 0)
    return StringUtils::Format(g_localizeStrings.Get(22004).c_str(), fabs(value));

  return StringUtils::Format(g_localizeStrings.Get(22005).c_str(), value);
}

std::string CGUIDialogAudioSubtitleSettings::FormatDecibel(float value)
{
  return StringUtils::Format(g_localizeStrings.Get(14054).c_str(), value);
}

std::string CGUIDialogAudioSubtitleSettings::FormatPercentAsDecibel(float value)
{
  // TODO: calculate volume gain
  return StringUtils::Format(g_localizeStrings.Get(14054).c_str(), value/*CAEUtil::PercentToGain(value)*/);
}

void CGUIDialogAudioSubtitleSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  CVideoSettings &videoSettings = CMediaSettings::Get().GetCurrentVideoSettings();
  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_AUDIO_VOLUME)
  {
    m_volume = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    g_application.SetVolume(long(m_volume * 100.0f), false); // false - value is not in percent
  }
  else if (settingId == SETTING_AUDIO_VOLUME_AMPLIFICATION)
  {
    videoSettings.m_VolumeAmplification = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    g_application.m_pPlayer->SetDynamicRangeCompression((long)(videoSettings.m_VolumeAmplification * 100));
  }
  else if (settingId == SETTING_AUDIO_DELAY)
  {
    videoSettings.m_AudioDelay = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    g_application.m_pPlayer->SetAVDelay(videoSettings.m_AudioDelay);
  }
  else if (settingId == SETTING_AUDIO_STREAM)
  {
    m_audioStream = static_cast<const CSettingInt*>(setting)->GetValue();

    // first check if it's a stereo track that we can change between stereo, left and right
    if (g_application.m_pPlayer->GetAudioStreamCount() == 1)
    {
      if (m_audioStreamStereoMode)
      { // we're in the case we want - call the code to switch channels etc.
        // update the screen setting...
        videoSettings.m_AudioStream = -1 - m_audioStream;
        // call monkeyh1's code here...
        bool bAudioOnAllSpeakers = (CSettings::GetInstance().GetInt("audiooutput.mode") == AUDIO_DIGITAL) && CMediaSettings::Get().GetCurrentVideoSettings().m_OutputToAllSpeakers;
#if defined(HAS_VIDEO_PLAYBACK) && defined(HAS_XBOX_HARDWARE)
        xbox_audio_switch_channel(m_audioStream, bAudioOnAllSpeakers);
#endif
        return;
      }
    }
    // only change the audio stream if a different one has been asked for
    if (g_application.m_pPlayer->GetAudioStream() != m_audioStream)
    {
      videoSettings.m_AudioStream = m_audioStream;
      g_application.m_pPlayer->SetAudioStream(m_audioStream);    // Set the audio stream to the one selected
    }
  }
  else if (settingId == SETTING_AUDIO_OUTPUT_TO_ALL_SPEAKERS)
  {
    videoSettings.m_OutputToAllSpeakers = static_cast<const CSettingBool*>(setting)->GetValue();
    g_application.Restart();
  }
  else if (settingId == SETTING_AUDIO_DIGITAL_ANALOG)
  {
    m_outputmode = static_cast<const CSettingBool*>(setting)->GetValue();
    CSettings::GetInstance().SetInt("audiooutput.mode", m_outputmode);
    g_application.Restart();
  }
  else if (settingId == SETTING_SUBTITLE_ENABLE)
  {
    m_subtitleVisible = videoSettings.m_SubtitleOn = static_cast<const CSettingBool*>(setting)->GetValue();
    g_application.m_pPlayer->SetSubtitleVisible(videoSettings.m_SubtitleOn);
  }
  else if (settingId == SETTING_SUBTITLE_DELAY)
  {
    videoSettings.m_SubtitleDelay = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    g_application.m_pPlayer->SetSubTitleDelay(videoSettings.m_SubtitleDelay);
  }
  else if (settingId == SETTING_SUBTITLE_STREAM)
  {
    m_subtitleStream = videoSettings.m_SubtitleStream = static_cast<const CSettingInt*>(setting)->GetValue();
    g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
  }
}

void CGUIDialogAudioSubtitleSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_SUBTITLE_BROWSER)
  {
    CStdString strPath;
    if (URIUtils::IsInRAR(g_application.CurrentFileItem().GetPath()) || URIUtils::IsInZIP(g_application.CurrentFileItem().GetPath()))
      strPath = CURL(g_application.CurrentFileItem().GetPath()).GetHostName();
    else
      strPath = g_application.CurrentFileItem().GetPath();

    std::string strMask = ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.aqt|.jss|.ass|.idx|.rar|.zip";
    if (g_application.GetCurrentPlayer() == EPC_DVDPLAYER)
      strMask = ".srt|.rar|.zip|.ifo|.smi|.sub|.idx|.ass|.ssa|.txt";
    VECSOURCES shares(*CMediaSourceSettings::Get().GetSources("video"));
    if (CMediaSettings::Get().GetAdditionalSubtitleDirectoryChecked() != -1 && !CSettings::GetInstance().GetString("subtitles.custompath").empty())
    {
      CMediaSource share;
      std::vector<std::string> paths;
      paths.push_back(URIUtils::GetDirectory(strPath));
      paths.push_back(CSettings::GetInstance().GetString("subtitles.custompath"));
      share.FromNameAndPaths("video",g_localizeStrings.Get(21367),paths);
      shares.push_back(share);
      strPath = share.strPath;
      URIUtils::AddSlashAtEnd(strPath);
    }
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, strMask, g_localizeStrings.Get(293), strPath, false, true)) // "subtitles"
    {
      // TODO: check if subtitle caching can be removed on Xbox without perfomance decrease
      // https://github.com/xbmc/xbmc/commit/ea87f7e85744304bc4bab8a14088eb78fdc5677a
#ifndef _XBOX
      if (URIUtils::HasExtension(strPath, ".sub"))
      {
        if (XFILE::CFile::Exists(URIUtils::ReplaceExtension(strPath, ".idx")))
          strPath = URIUtils::ReplaceExtension(strPath, ".idx");
      }

      int id = g_application.m_pPlayer->AddSubtitle(strPath);
      if (id >= 0)
      {
        m_subtitleStream = id;
        g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
        g_application.m_pPlayer->SetSubtitleVisible(true);
      }
      CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleCached = true;
      Close();
#else
      CStdString strExt = URIUtils::GetExtension(strPath);
      if (URIUtils::HasExtension(strPath, ".idx") || URIUtils::HasExtension(strPath, ".sub"))
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

          if (XFILE::CFile::Copy(strPath,"special://temp/subtitle"+strExt+".keep"))
          {
            CStdString strPath2;
            CStdString strPath3;
            if (URIUtils::HasExtension(strPath, ".idx"))
            {
              strPath2 = URIUtils::ReplaceExtension(strPath,".sub");
              strPath3 = "special://temp/subtitle.sub.keep";
            }
            else
            {
              strPath2 = URIUtils::ReplaceExtension(strPath,".idx");
              if (!XFILE::CFile::Exists(strPath2) && (URIUtils::IsInRAR(strPath2) || URIUtils::IsInZIP(strPath2)))
              {
                CStdString strFileName = URIUtils::GetFileName(strPath);
                strPath3 = URIUtils::GetDirectory(strPath);
                URIUtils::GetParentPath(strPath3,strPath2);
                strPath2 = URIUtils::AddFileToFolder(strPath2,strFileName);
                strPath2 = URIUtils::ReplaceExtension(strPath2,".idx");
              }
              strPath3 = "special://temp/subtitle.idx.keep";
            }
            if (XFILE::CFile::Exists(strPath2))
              XFILE::CFile::Copy(strPath2,strPath3);
            else
            {
              CFileItemList items;
              CStdString strDir,strFileNameNoExtNoCase;
              URIUtils::Split(strPath,strDir,strPath3);
              strFileNameNoExtNoCase = URIUtils::ReplaceExtension(strPath3,".");
              strFileNameNoExtNoCase.ToLower();
              strDir = URIUtils::GetDirectory(strPath);
              XFILE::CDirectory::GetDirectory(strDir,items,".rar|.zip",XFILE::DIR_FLAG_NO_FILE_DIRS,true);
              for (int i=0;i<items.Size();++i)
                CUtil::CacheRarSubtitles(items[i]->GetPath(),strFileNameNoExtNoCase);
            }
            CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleOn = true;

            if (g_application.GetCurrentPlayer() == EPC_MPLAYER)
            {
              CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleCached = false;
              // reopen the file
              if ( g_application.PlayFile(g_application.CurrentFileItem(), "", true) && g_application.m_pPlayer )
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
              XFILE::CFile::Delete(strSub);
              XFILE::CFile::Delete(strIdx);
              XFILE::CFile::Rename(strSub + ".keep", strSub);
              XFILE::CFile::Rename(strIdx + ".keep", strIdx);

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
        std::string strExt = URIUtils::GetExtension(strPath);
        if (XFILE::CFile::Copy(strPath,"special://temp/subtitle.browsed"+strExt))
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
        CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleCached = true;
      }
#endif
    }
  }
  else if (settingId == SETTING_AUDIO_MAKE_DEFAULT)
    Save();
}

void CGUIDialogAudioSubtitleSettings::Save()
{
  if (!g_passwordManager.CheckSettingLevelLock(SettingLevelExpert) &&
      CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
    return;

  // prompt user if they are sure
  if (!CGUIDialogYesNo::ShowAndGetInput(12376, 750, 0, 12377))
    return;

  // reset the settings
  CVideoDatabase db;
  if (!db.Open())
    return;

  db.EraseVideoSettings();
  db.Close();

  CMediaSettings::Get().GetDefaultVideoSettings() = CMediaSettings::Get().GetCurrentVideoSettings();
  CMediaSettings::Get().GetDefaultVideoSettings().m_SubtitleStream = -1;
  CMediaSettings::Get().GetDefaultVideoSettings().m_AudioStream = -1;
  CSettings::GetInstance().Save();
}

void CGUIDialogAudioSubtitleSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(13396);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_OKAY_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 15067);
}

void CGUIDialogAudioSubtitleSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("audiosubtitlesettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioSubtitleSettings: unable to setup settings");
    return;
  }

  // get all necessary setting groups
  CSettingGroup *groupAudio = AddGroup(category);
  if (groupAudio == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioSubtitleSettings: unable to setup settings");
    return;
  }
  CSettingGroup *groupSubtitles = AddGroup(category);
  if (groupSubtitles == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioSubtitleSettings: unable to setup settings");
    return;
  }
  CSettingGroup *groupSaveAsDefault = AddGroup(category);
  if (groupSubtitles == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioSubtitleSettings: unable to setup settings");
    return;
  }

  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  CVideoSettings &videoSettings = CMediaSettings::Get().GetCurrentVideoSettings();

  CSettingDependency dependencyAudioOutputPassthroughDisabled(SettingDependencyTypeEnable, m_settingsManager);
  dependencyAudioOutputPassthroughDisabled.And()
    ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_AUDIO_DIGITAL_ANALOG, "1", SettingDependencyOperatorEquals, false, m_settingsManager)));
  SettingDependencies depsAudioOutputPassthroughDisabled;
  depsAudioOutputPassthroughDisabled.push_back(dependencyAudioOutputPassthroughDisabled);

  // audio settings
  // audio volume setting
  m_volume = g_application.GetVolume(false) * 0.01f;
  CSettingNumber *settingAudioVolume = AddSlider(groupAudio, SETTING_AUDIO_VOLUME, 13376, 0, m_volume, 14054, VOLUME_MINIMUM * 0.01f, (VOLUME_MAXIMUM - VOLUME_MINIMUM) * 0.0001f, VOLUME_MAXIMUM * 0.01f);
  static_cast<CSettingControlSlider*>(settingAudioVolume->GetControl())->SetFormatter(SettingFormatterPercentAsDecibel);

  // audio volume amplification setting
  /*if (SupportsAudioFeature(IPC_AUD_AMP))*/
  {
    CSettingNumber *settingAudioVolumeAmplification = AddSlider(groupAudio, SETTING_AUDIO_VOLUME_AMPLIFICATION, 660, 0, videoSettings.m_VolumeAmplification, 14054, VOLUME_DRC_MINIMUM * 0.01f, (VOLUME_DRC_MAXIMUM - VOLUME_DRC_MINIMUM) / 6000.0f, VOLUME_DRC_MAXIMUM * 0.01f);
  }

  // audio delay setting
  /*if (SupportsAudioFeature(IPC_AUD_OFFSET))*/
  {
    CSettingNumber *settingAudioDelay = AddSlider(groupAudio, SETTING_AUDIO_DELAY, 297, 0, videoSettings.m_AudioDelay, 0, -g_advancedSettings.m_videoAudioDelayRange, 0.025f, g_advancedSettings.m_videoAudioDelayRange, -1, usePopup);
    static_cast<CSettingControlSlider*>(settingAudioDelay->GetControl())->SetFormatter(SettingFormatterDelay);
  }

  // audio stream setting
  /*if (SupportsAudioFeature(IPC_AUD_SELECT_STREAM))*/
    AddAudioStreams(groupAudio, SETTING_AUDIO_STREAM);

  // audio output to all speakers setting
  // TODO: remove this setting
  /*if (SupportsAudioFeature(IPC_AUD_OUTPUT_STEREO))*/
  {
    CSettingBool *settingOutputToAllSpeakers = AddToggle(groupAudio, SETTING_AUDIO_OUTPUT_TO_ALL_SPEAKERS, 252, 0, videoSettings.m_OutputToAllSpeakers);
    settingOutputToAllSpeakers->SetDependencies(depsAudioOutputPassthroughDisabled);
  }

  // audio digital/analog setting
  if(g_audioConfig.HasDigitalOutput())
  {
    m_outputmode = CSettings::GetInstance().GetInt("audiooutput.mode");
    AddSpinner(groupAudio, SETTING_AUDIO_DIGITAL_ANALOG, 38629, 0, m_outputmode, XBAudioConfig::SettingAudioOutputFiller);
  }

  // subitlte settings
  m_subtitleVisible = g_application.m_pPlayer->GetSubtitleVisible();
  // subtitle enabled setting
  AddToggle(groupSubtitles, SETTING_SUBTITLE_ENABLE, 13397, 0, m_subtitleVisible);

  // subtitle delay setting
  /*if (SupportsSubtitleFeature(IPC_SUBS_OFFSET))*/
  {
    CSettingNumber *settingSubtitleDelay = AddSlider(groupSubtitles, SETTING_SUBTITLE_DELAY, 22006, 0, videoSettings.m_SubtitleDelay, 0, -g_advancedSettings.m_videoSubsDelayRange, 0.1f, g_advancedSettings.m_videoSubsDelayRange, -1, usePopup);
    static_cast<CSettingControlSlider*>(settingSubtitleDelay->GetControl())->SetFormatter(SettingFormatterDelay);
  }

  // subtitle stream setting
  /*if (SupportsSubtitleFeature(IPC_SUBS_SELECT))*/
    AddSubtitleStreams(groupSubtitles, SETTING_SUBTITLE_STREAM);

  // subtitle browser setting
  /*if (SupportsSubtitleFeature(IPC_SUBS_EXTERNAL))*/
    AddButton(groupSubtitles, SETTING_SUBTITLE_BROWSER, 13250, 0);

  // subtitle stream setting
  AddButton(groupSaveAsDefault, SETTING_AUDIO_MAKE_DEFAULT, 12376, 0);
}

void CGUIDialogAudioSubtitleSettings::AddAudioStreams(CSettingGroup *group, const std::string &settingId)
{
  m_audioStreamStereoMode = false;
  if (group == NULL || settingId.empty())
    return;

  m_audioStream = g_application.m_pPlayer->GetAudioStream();
  if (m_audioStream < 0)
    m_audioStream = 0;

  // check if we have a single, stereo stream, and if so, allow us to split into
  // left, right or both
  if (g_application.m_pPlayer->GetAudioStreamCount() == 1)
  {
    CStdString strAudioInfo;
    g_application.m_pPlayer->GetAudioInfo(strAudioInfo);

    /* TODO:STRING_CLEANUP */
    int iNumChannels = 0;
    size_t pos = strAudioInfo.find("chns:");
    if (pos != std::string::npos)
      iNumChannels = static_cast<int>(strtol(strAudioInfo.substr(pos + 5).c_str(), NULL, 0));

    std::string strAudioCodec;
    if (strAudioInfo.size() > 7)
      strAudioCodec = strAudioInfo.substr(7, strAudioInfo.find(") VBR") - 5);

    bool bDTS = strAudioCodec.find("DTS") != std::string::npos;
    bool bAC3 = strAudioCodec.find("AC3") != std::string::npos;

    if (iNumChannels == 2 && !(bDTS || bAC3))
    { // ok, enable these options
/*      if (CMediaSettings::Get().GetCurrentVideoSettings().m_AudioStream == -1)
      { // default to stereo stream
        CMediaSettings::Get().GetCurrentVideoSettings().m_AudioStream = 0;
      }*/
      StaticIntegerSettingOptions options;
      for (int i = 0; i < 3; ++i)
        options.push_back(make_pair(i, 13320 + i));

      m_audioStream = -CMediaSettings::Get().GetCurrentVideoSettings().m_AudioStream - 1;
      m_audioStreamStereoMode = true;
      AddSpinner(group, settingId, 460, 0, m_audioStream, options);
      return;
    }
  }

  AddSpinner(group, settingId, 460, 0, m_audioStream, AudioStreamsOptionFiller);
}

void CGUIDialogAudioSubtitleSettings::AddSubtitleStreams(CSettingGroup *group, const std::string &settingId)
{
  if (group == NULL || settingId.empty())
    return;

  m_subtitleStream = CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleStream;
  if (m_subtitleStream < 0)
    m_subtitleStream = 0;

  AddSpinner(group, settingId, 462, 0, m_subtitleStream, SubtitleStreamsOptionFiller);
}

void CGUIDialogAudioSubtitleSettings::AudioStreamsOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  int audioStreamCount = g_application.m_pPlayer->GetAudioStreamCount();

  // cycle through each audio stream and add it to our list control
  for (int i = 0; i < audioStreamCount; ++i)
  {
    std::string strItem;
    CStdString strLanguage;

    CStdString name, language;
    g_application.m_pPlayer->GetAudioStreamName(i, name);
    g_application.m_pPlayer->GetAudioStreamLanguage(i, language);

    if (!g_LangCodeExpander.Lookup(strLanguage, language))
      strLanguage = g_localizeStrings.Get(13205); // Unknown

    if (name.length() == 0)
      strItem = strLanguage;
    else
      strItem = StringUtils::Format("%s - %s", strLanguage.c_str(), name.c_str());

    strItem += StringUtils::Format(" (%i/%i)", i + 1, audioStreamCount);
    list.push_back(make_pair(strItem, i));
  }

  if (list.empty())
  {
    list.push_back(make_pair(g_localizeStrings.Get(231), -1));
    current = -1;
  }
}

void CGUIDialogAudioSubtitleSettings::SubtitleStreamsOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  int subtitleStreamCount = g_application.m_pPlayer->GetSubtitleCount();

  // cycle through each subtitle and add it to our entry list
  for (int i = 0; i < subtitleStreamCount; ++i)
  {
    CStdString name, language;
    g_application.m_pPlayer->GetSubtitleName(i, name);
    g_application.m_pPlayer->GetSubtitleLanguage(i, language);

    CStdString strItem;
    CStdString strLanguage;

    if (!g_LangCodeExpander.Lookup(strLanguage, language))
      strLanguage = g_localizeStrings.Get(13205); // Unknown

    if (name.length() == 0)
      strItem = strLanguage;
    else
      strItem = StringUtils::Format("%s - %s", strLanguage.c_str(), name.c_str());

    strItem += StringUtils::Format(" (%i/%i)", i + 1, subtitleStreamCount);

    list.push_back(make_pair(strItem, i));
  }

  // no subtitle streams - just add a "None" entry
  if (list.empty())
  {
    list.push_back(make_pair(g_localizeStrings.Get(231), -1));
    current = -1;
  }
}

std::string CGUIDialogAudioSubtitleSettings::SettingFormatterDelay(const CSettingControlSlider *control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum)
{
  if (!value.isDouble())
    return "";

  float fValue = value.asFloat();
  float fStep = step.asFloat();

  if (fabs(fValue) < 0.5f * fStep)
    return StringUtils::Format(g_localizeStrings.Get(22003).c_str(), 0.0);
  if (fValue < 0)
    return StringUtils::Format(g_localizeStrings.Get(22004).c_str(), fabs(fValue));

  return StringUtils::Format(g_localizeStrings.Get(22005).c_str(), fValue);
}

std::string CGUIDialogAudioSubtitleSettings::SettingFormatterPercentAsDecibel(const CSettingControlSlider *control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum)
{
  if (control == NULL || !value.isDouble())
    return "";

  std::string formatString = control->GetFormatString();
  if (control->GetFormatLabel() > -1)
    formatString = g_localizeStrings.Get(control->GetFormatLabel());

  // TODO: calculate volume gain
  return StringUtils::Format(formatString.c_str(), value.asFloat()/*CAEUtil::PercentToGain(value.asFloat())*/);
}

