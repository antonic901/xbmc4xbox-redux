/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerController.h"

#include "ServiceBroker.h"
#include "application/Application.h"
#include "cores/IPlayer.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogSlider.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUISliderControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "video/dialogs/GUIDialogAudioSettings.h"

using namespace KODI;
using namespace UTILS;

CPlayerController::CPlayerController()
{
  m_sliderAction = 0;
}

CPlayerController::~CPlayerController() {}

CPlayerController& CPlayerController::GetInstance()
{
  static CPlayerController instance;
  return instance;
}

bool CPlayerController::OnAction(const CAction &action)
{
  const unsigned int MsgTime = 300;
  const unsigned int DisplTime = 2000;

  CApplicationPlayer *const appPlayer = g_application.m_pPlayer;

  if (appPlayer->IsPlayingVideo())
  {
    switch (action.GetID())
    {
      case ACTION_SHOW_SUBTITLES:
      {
        if (appPlayer->GetSubtitleCount() == 0)
        {
          CGUIDialogKaiToast::QueueNotification(
              CGUIDialogKaiToast::Info, g_localizeStrings.Get(287), g_localizeStrings.Get(10005),
              DisplTime, false, MsgTime);
          return true;
        }

        bool subsOn = !appPlayer->GetSubtitleVisible();
        appPlayer->SetSubtitleVisible(subsOn);
        std::string sub;
        if (subsOn)
        {
          std::string lang;
          SPlayerSubtitleStreamInfo info;
          appPlayer->GetSubtitleStreamInfo(g_application.m_pPlayer->GetSubtitle(), info);
          if (!g_LangCodeExpander.Lookup(info.language, lang))
            lang = g_localizeStrings.Get(13205); // Unknown

          if (info.name.length() == 0)
            sub = lang;
          else
            sub = StringUtils::Format("%s - %s", lang.c_str(), info.name.c_str());
        }
        else
          sub = g_localizeStrings.Get(1223);
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                              g_localizeStrings.Get(287), sub, DisplTime, false, MsgTime);
        return true;
      }

      case ACTION_NEXT_SUBTITLE:
      case ACTION_CYCLE_SUBTITLE:
      {
        if (appPlayer->GetSubtitleCount() == 0)
          return true;

        int currentSub = appPlayer->GetSubtitle();
        bool currentSubVisible = true;

        if (appPlayer->GetSubtitleVisible())
        {
          if (++currentSub >= appPlayer->GetSubtitleCount())
          {
            currentSub = 0;
            if (action.GetID() == ACTION_NEXT_SUBTITLE)
            {
              appPlayer->SetSubtitleVisible(false);
              currentSubVisible = false;
            }
          }
          appPlayer->SetSubtitle(currentSub);
        }
        else if (action.GetID() == ACTION_NEXT_SUBTITLE)
        {
          appPlayer->SetSubtitleVisible(true);
        }

        std::string sub, lang;
        if (currentSubVisible)
        {
          SPlayerSubtitleStreamInfo info;
          appPlayer->GetSubtitleStreamInfo(currentSub, info);
          if (!g_LangCodeExpander.Lookup(info.language, lang))
            lang = g_localizeStrings.Get(13205); // Unknown

          if (info.name.length() == 0)
            sub = lang;
          else
            sub = StringUtils::Format("%s - %s", lang.c_str(), info.name.c_str());
        }
        else
          sub = g_localizeStrings.Get(1223);
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(287), sub, DisplTime, false, MsgTime);
        return true;
      }

      case ACTION_SUBTITLE_DELAY_MIN:
      {
        float videoSubsDelayRange = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoSubsDelayRange;
        CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
        vs.m_SubtitleDelay -= 0.1f;
        if (vs.m_SubtitleDelay < -videoSubsDelayRange)
          vs.m_SubtitleDelay = -videoSubsDelayRange;
        appPlayer->SetSubTitleDelay(vs.m_SubtitleDelay);

        ShowSlider(action.GetID(), 22006, CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleDelay,
                   -videoSubsDelayRange, 0.1f, videoSubsDelayRange);
        return true;
      }

      case ACTION_SUBTITLE_DELAY_PLUS:
      {
        float videoSubsDelayRange = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoSubsDelayRange;
        CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
        vs.m_SubtitleDelay += 0.1f;
        if (vs.m_SubtitleDelay > videoSubsDelayRange)
          vs.m_SubtitleDelay = videoSubsDelayRange;
        appPlayer->SetSubTitleDelay(vs.m_SubtitleDelay);

        ShowSlider(action.GetID(), 22006, CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleDelay,
                   -videoSubsDelayRange, 0.1f, videoSubsDelayRange);
        return true;
      }

      case ACTION_SUBTITLE_DELAY:
      {
        float videoSubsDelayRange = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoSubsDelayRange;
        ShowSlider(action.GetID(), 22006, CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleDelay,
                   -videoSubsDelayRange, 0.1f, videoSubsDelayRange, true);
        return true;
      }

      case ACTION_AUDIO_DELAY:
      {
        float videoAudioDelayRange = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAudioDelayRange;
        ShowSlider(action.GetID(), 297, CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioDelay,
                   -videoAudioDelayRange, AUDIO_DELAY_STEP, videoAudioDelayRange, true);
        return true;
      }

      case ACTION_AUDIO_DELAY_MIN:
      {
        float videoAudioDelayRange = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAudioDelayRange;
        CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
        vs.m_AudioDelay -= AUDIO_DELAY_STEP;
        if (vs.m_AudioDelay < -videoAudioDelayRange)
          vs.m_AudioDelay = -videoAudioDelayRange;
        appPlayer->SetAVDelay(vs.m_AudioDelay);

        ShowSlider(action.GetID(), 297, CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioDelay,
                   -videoAudioDelayRange, AUDIO_DELAY_STEP, videoAudioDelayRange);
        return true;
      }

      case ACTION_AUDIO_DELAY_PLUS:
      {
        float videoAudioDelayRange = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAudioDelayRange;
        CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
        vs.m_AudioDelay += AUDIO_DELAY_STEP;
        if (vs.m_AudioDelay > videoAudioDelayRange)
          vs.m_AudioDelay = videoAudioDelayRange;
        appPlayer->SetAVDelay(vs.m_AudioDelay);

        ShowSlider(action.GetID(), 297, CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioDelay,
                   -videoAudioDelayRange, AUDIO_DELAY_STEP, videoAudioDelayRange);
        return true;
      }

      case ACTION_AUDIO_NEXT_LANGUAGE:
      {
        if (appPlayer->GetAudioStreamCount() == 1)
          return true;

        int currentAudio = appPlayer->GetAudioStream();
        int audioStreamCount = appPlayer->GetAudioStreamCount();

        if (++currentAudio >= audioStreamCount)
          currentAudio = 0;
        appPlayer->SetAudioStream(currentAudio); // Set the audio stream to the one selected
        std::string aud;
        std::string lan;
        SPlayerAudioStreamInfo info;
        appPlayer->GetAudioStreamInfo(currentAudio, info);
        if (!g_LangCodeExpander.Lookup(info.language, lan))
          lan = g_localizeStrings.Get(13205); // Unknown
        if (info.name.empty())
          aud = lan;
        else
          aud = StringUtils::Format("%s - %s", lan.c_str(), info.name.c_str());
        std::string caption = g_localizeStrings.Get(460);
        caption += StringUtils::Format(" (%i/%i)", currentAudio + 1, audioStreamCount);
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, caption, aud, DisplTime, false, MsgTime);
        return true;
      }

      case ACTION_ZOOM_IN:
      {
        CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
        vs.m_CustomZoomAmount += 0.01f;
        if (vs.m_CustomZoomAmount > 2.f)
          vs.m_CustomZoomAmount = 2.f;
        vs.m_ViewMode = ViewModeCustom;
        appPlayer->SetRenderViewMode(ViewModeCustom);
        ShowSlider(action.GetID(), 216, vs.m_CustomZoomAmount, 0.5f, 0.1f, 2.0f);
        return true;
      }

      case ACTION_ZOOM_OUT:
      {
        CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
        vs.m_CustomZoomAmount -= 0.01f;
        if (vs.m_CustomZoomAmount < 0.5f)
          vs.m_CustomZoomAmount = 0.5f;
        vs.m_ViewMode = ViewModeCustom;
        appPlayer->SetRenderViewMode(ViewModeCustom);
        ShowSlider(action.GetID(), 216, vs.m_CustomZoomAmount, 0.5f, 0.1f, 2.0f);
        return true;
      }

      case ACTION_INCREASE_PAR:
      {
        CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
        vs.m_CustomPixelRatio += 0.01f;
        if (vs.m_CustomPixelRatio > 2.f)
          vs.m_CustomPixelRatio = 2.f;
        vs.m_ViewMode = ViewModeCustom;
        appPlayer->SetRenderViewMode(ViewModeCustom);
        ShowSlider(action.GetID(), 217, vs.m_CustomPixelRatio, 0.5f, 0.1f, 2.0f);
        return true;
      }

      case ACTION_DECREASE_PAR:
      {
        CVideoSettings vs = CMediaSettings::GetInstance().GetCurrentVideoSettings();
        vs.m_CustomPixelRatio -= 0.01f;
        if (vs.m_CustomPixelRatio < 0.5f)
          vs.m_CustomPixelRatio = 0.5f;
        vs.m_ViewMode = ViewModeCustom;
        appPlayer->SetRenderViewMode(ViewModeCustom);
        ShowSlider(action.GetID(), 217, vs.m_CustomPixelRatio, 0.5f, 0.1f, 2.0f);
        return true;
      }

      default:
        break;
    }
  }
  return false;
}

void CPlayerController::ShowSlider(int action, int label, float value, float min, float delta, float max, bool modal)
{
  m_sliderAction = action;
  if (modal)
    CGUIDialogSlider::ShowAndGetInput(g_localizeStrings.Get(label), value, min, delta, max, this);
  else
    CGUIDialogSlider::Display(label, value, min, delta, max, this);
}

void CPlayerController::OnSliderChange(void *data, CGUISliderControl *slider)
{
  if (!slider)
    return;

  if (m_sliderAction == ACTION_ZOOM_OUT || m_sliderAction == ACTION_ZOOM_IN ||
      m_sliderAction == ACTION_INCREASE_PAR || m_sliderAction == ACTION_DECREASE_PAR)
  {
    std::string strValue = StringUtils::Format("%1.2f", slider->GetFloatValue());
    slider->SetTextValue(strValue);
  }
  else
    slider->SetTextValue(
        CGUIDialogAudioSettings::FormatDelay(slider->GetFloatValue(), AUDIO_DELAY_STEP));

  CApplicationPlayer *const appPlayer = g_application.m_pPlayer;

  if (appPlayer->HasPlayer())
  {
    if (m_sliderAction == ACTION_AUDIO_DELAY)
    {
      appPlayer->SetAVDelay(slider->GetFloatValue());
    }
    else if (m_sliderAction == ACTION_SUBTITLE_DELAY)
    {
      appPlayer->SetSubTitleDelay(slider->GetFloatValue());
    }
  }
}
