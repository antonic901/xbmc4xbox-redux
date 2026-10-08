/*
 *  Copyright (c) 2006 elupus (Joakim Plate)
 *  Copyright (C) 2006-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "UPnPPlayer.h"

#include "ServiceBroker.h"
#include "ThumbLoader.h"
#include "UPnP.h"
#include "UPnPInternal.h"
#include "cores/DataCacheCore.h"
#include "dialogs/GUIDialogBusy.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/helpers/DialogHelper.h"
#include "music/MusicThumbLoader.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/Event.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "video/VideoThumbLoader.h"


#include <Platinum/Source/Devices/MediaRenderer/PltMediaController.h>
#include <Platinum/Source/Devices/MediaServer/PltDidl.h>
#include <Platinum/Source/Platinum/Platinum.h>

using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

NPT_SET_LOCAL_LOGGER("xbmc.upnp.player")

namespace UPNP
{

class CUPnPPlayerController : public PLT_MediaControllerDelegate
{
public:
  CUPnPPlayerController(PLT_MediaController* control,
                        PLT_DeviceDataReference& device,
                        IPlayerCallback& callback)
    : m_control(control),
      m_transport(NULL),
      m_device(device),
      m_instance(0),
      m_callback(callback),
      m_postime(0),
      m_posinfo()
  {
    m_device->FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", m_transport);
  }

  virtual void OnSetAVTransportURIResult(NPT_Result res,
                                 PLT_DeviceDataReference& device,
                                 void* userdata)
  {
    if (NPT_FAILED(res))
      CLog::Log(LOGERROR, "OnSetAVTransportURIResult failed");
    m_resstatus = res;
    m_resevent.Set();
  }

  virtual void OnPlayResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  {
    if (NPT_FAILED(res))
      CLog::Log(LOGERROR, "OnPlayResult failed");
    m_resstatus = res;
    m_resevent.Set();
  }

  virtual void OnStopResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata)
  {
    if (NPT_FAILED(res))
      CLog::Log(LOGERROR, "OnStopResult failed");
    m_resstatus = res;
    m_resevent.Set();
  }

  NPT_String GetTransportState() const
  {
    CSingleLock lock(m_section);
    return m_trainfo.cur_transport_state;
  }

  NPT_String GetTransportStatus() const
  {
    CSingleLock lock(m_section);
    return m_trainfo.cur_transport_status;
  }

  virtual void OnGetMediaInfoResult(NPT_Result res,
                            PLT_DeviceDataReference& device,
                            PLT_MediaInfo* info,
                            void* userdata)
  {
    if (NPT_FAILED(res) || info == NULL)
      CLog::Log(LOGERROR, "OnGetMediaInfoResult failed");
  }

  virtual void OnGetTransportInfoResult(NPT_Result res,
                                PLT_DeviceDataReference& device,
                                PLT_TransportInfo* info,
                                void* userdata)
  {
    CSingleLock lock(m_section);

    if (NPT_FAILED(res))
    {
      CLog::Log(LOGERROR, "OnGetTransportInfoResult failed");
      m_trainfo.cur_speed = "0";
      m_trainfo.cur_transport_state = "STOPPED";
      m_trainfo.cur_transport_status = "ERROR_OCCURED";
    }
    else
      m_trainfo = *info;
    m_traevnt.Set();
  }

  void UpdatePositionInfo()
  {
    if (m_postime == 0 || m_postime > CTimeUtils::GetFrameTime())
      return;

    m_control->GetTransportInfo(m_device, m_instance, this);
    m_control->GetPositionInfo(m_device, m_instance, this);
    m_postime = 0;
  }

  virtual void OnGetPositionInfoResult(NPT_Result res,
                               PLT_DeviceDataReference& device,
                               PLT_PositionInfo* info,
                               void* userdata)
  {
    CSingleLock lock(m_section);

    if (NPT_FAILED(res) || info == NULL)
    {
      CLog::Log(LOGERROR, "OnGetMediaInfoResult failed");
      m_posinfo = PLT_PositionInfo();
    }
    else
      m_posinfo = *info;
    m_postime = CTimeUtils::GetFrameTime() + 500;
    m_posevnt.Set();
  }

  virtual ~CUPnPPlayerController() {}

  PLT_MediaController* m_control;
  PLT_Service* m_transport;
  PLT_DeviceDataReference m_device;
  NPT_UInt32 m_instance;
  IPlayerCallback& m_callback;

  NPT_Result m_resstatus;
  CEvent m_resevent;

  unsigned int m_postime;

  CEvent m_posevnt;
  PLT_PositionInfo m_posinfo;

  CEvent m_traevnt;
  PLT_TransportInfo m_trainfo;

private:
  mutable CCriticalSection m_section;
};

CUPnPPlayer::CUPnPPlayer(IPlayerCallback& callback, const char* uuid)
  : IPlayer(callback),
    CThread("UPnPPlayer"),
    m_control(NULL),
    m_started(false),
    m_stopremote(false),
    m_hasVideo(false),
    m_hasAudio(false)
{
  m_control = CUPnP::GetInstance()->m_MediaController;

  PLT_DeviceDataReference device;
  if (NPT_SUCCEEDED(m_control->FindRenderer(uuid, device)))
  {
    m_delegate.reset(new CUPnPPlayerController(m_control, device, callback));
    CUPnP::RegisterUserdata(m_delegate.get());
  }
  else
    CLog::Log(LOGERROR, "couldn't find device as %s", uuid);
}

CUPnPPlayer::~CUPnPPlayer()
{
  CloseFile();
  CUPnP::UnregisterUserdata(m_delegate.get());
}

static NPT_Result WaitOnEvent(CEvent& event, XbmcThreads::EndTime& timeout)
{
  if (event.WaitMSec(0))
    return NPT_SUCCESS;

  if (!CGUIDialogBusy::WaitOnEvent(event))
    return NPT_FAILURE;

  return NPT_SUCCESS;
}

int CUPnPPlayer::PlayFile(const CFileItem& file,
                          const CPlayerOptions& options,
                          XbmcThreads::EndTime& timeout)
{
  CFileItem item(file);
  NPT_Reference<CThumbLoader> thumb_loader;
  NPT_Reference<PLT_MediaObject> obj;
  NPT_String path(file.GetPath().c_str());
  NPT_String tmp, resource;
  EMediaControllerQuirks quirks = EMEDIACONTROLLERQUIRKS_NONE;

  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);

  if (file.IsVideoDb())
    thumb_loader = NPT_Reference<CThumbLoader>(new CVideoThumbLoader());
  else if (item.IsMusicDb())
    thumb_loader = NPT_Reference<CThumbLoader>(new CMusicThumbLoader());

  obj = BuildObject(item, path, false, thumb_loader, NULL, CUPnP::GetServer(), UPnPPlayer);
  if (obj.IsNull())
    goto failed;

  NPT_CHECK_LABEL_SEVERE(PLT_Didl::ToDidl(*obj, "", tmp), failed_todidl);
  tmp.Insert(didl_header, 0);
  tmp.Append(didl_footer);

  quirks = GetMediaControllerQuirks(m_delegate->m_device.AsPointer());
  if (quirks & EMEDIACONTROLLERQUIRKS_X_MKV)
  {
    for (NPT_Cardinal i = 0; i < obj->m_Resources.GetItemCount(); i++)
    {
      if (obj->m_Resources[i].m_ProtocolInfo.GetContentType().Compare("video/x-matroska") == 0)
      {
        CLog::Log(LOGDEBUG, "PlayFile(%s): applying video/x-mkv quirk", file.GetPath().c_str());
        NPT_String protocolInfo = obj->m_Resources[i].m_ProtocolInfo.ToString();
        protocolInfo.Replace(":video/x-matroska:", ":video/x-mkv:");
        obj->m_Resources[i].m_ProtocolInfo = PLT_ProtocolInfo(protocolInfo);
      }
    }
  }

  /* The resource uri's are stored in the Didl. We must choose the best resource
   * for the playback device */
  NPT_Cardinal res_index;
  NPT_CHECK_LABEL_SEVERE(m_control->FindBestResource(m_delegate->m_device, *obj, res_index),
                         failed_findbestresource);

  // get the transport info to evaluate the TransportState to be able to
  // determine whether we first need to call Stop()
  timeout.Set(timeout.GetInitialTimeoutValue());
  NPT_CHECK_LABEL_SEVERE(
      m_control->GetTransportInfo(m_delegate->m_device, m_delegate->m_instance, m_delegate.get()),
      failed_gettransportinfo);
  NPT_CHECK_LABEL_SEVERE(WaitOnEvent(m_delegate->m_traevnt, timeout), failed_gettransportinfo);

  if (m_delegate->m_trainfo.cur_transport_state != "NO_MEDIA_PRESENT" &&
      m_delegate->m_trainfo.cur_transport_state != "STOPPED")
  {
    timeout.Set(timeout.GetInitialTimeoutValue());
    NPT_CHECK_LABEL_SEVERE(
        m_control->Stop(m_delegate->m_device, m_delegate->m_instance, m_delegate.get()),
        failed_stop);
    NPT_CHECK_LABEL_SEVERE(WaitOnEvent(m_delegate->m_resevent, timeout), failed_stop);
    NPT_CHECK_LABEL_SEVERE(m_delegate->m_resstatus, failed_stop);
  }

  timeout.Set(timeout.GetInitialTimeoutValue());
  NPT_CHECK_LABEL_SEVERE(m_control->SetAVTransportURI(m_delegate->m_device, m_delegate->m_instance,
                                                      obj->m_Resources[res_index].m_Uri,
                                                      (const char*)tmp, m_delegate.get()),
                         failed_setavtransporturi);
  NPT_CHECK_LABEL_SEVERE(WaitOnEvent(m_delegate->m_resevent, timeout), failed_setavtransporturi);
  NPT_CHECK_LABEL_SEVERE(m_delegate->m_resstatus, failed_setavtransporturi);

  timeout.Set(timeout.GetInitialTimeoutValue());
  NPT_CHECK_LABEL_SEVERE(
      m_control->Play(m_delegate->m_device, m_delegate->m_instance, "1", m_delegate.get()),
      failed_play);
  NPT_CHECK_LABEL_SEVERE(WaitOnEvent(m_delegate->m_resevent, timeout), failed_play);
  NPT_CHECK_LABEL_SEVERE(m_delegate->m_resstatus, failed_play);

  /* wait for PLAYING state */
  timeout.Set(timeout.GetInitialTimeoutValue());
  do
  {
    NPT_CHECK_LABEL_SEVERE(
        m_control->GetTransportInfo(m_delegate->m_device, m_delegate->m_instance, m_delegate.get()),
        failed_waitplaying);

    const NPT_String transportStatus = m_delegate->GetTransportStatus();
    const NPT_String transportState = m_delegate->GetTransportState();
    if (transportState == "PLAYING" || transportState == "PAUSED_PLAYBACK")
    {
      break;
    }
    if (transportState == "STOPPED" && transportStatus != "OK")
    {
      CLog::Log(LOGERROR, "OpenFile(%s): remote player signalled error", file.GetPath().c_str());
      return NPT_FAILURE;
    }

    NPT_CHECK_LABEL_SEVERE(WaitOnEvent(m_delegate->m_traevnt, timeout), failed_waitplaying);

  } while (!timeout.IsTimePast());

  if (options.starttime > 0)
  {
    /* many upnp units won't load file properly until after play (including xbmc) */
    NPT_CHECK_LABEL(m_control->Seek(m_delegate->m_device, m_delegate->m_instance, "REL_TIME",
                                    PLT_Didl::FormatTimeStamp((NPT_UInt32)options.starttime),
                                    m_delegate.get()),
                    failed_seek);
  }

  return NPT_SUCCESS;
failed_todidl:
  CLog::Log(LOGERROR, "PlayFile(%s) failed to serialize item into DIDL-Lite", file.GetPath().c_str());
  return NPT_FAILURE;
failed_findbestresource:
  CLog::Log(LOGERROR, "PlayFile(%s) failed to find a matching resource", file.GetPath().c_str());
  return NPT_FAILURE;
failed_gettransportinfo:
  CLog::Log(LOGERROR, "PlayFile(%s): call to GetTransportInfo failed", file.GetPath().c_str());
  return NPT_FAILURE;
failed_stop:
  CLog::Log(LOGERROR, "PlayFile(%s) failed to stop current playback", file.GetPath().c_str());
  return NPT_FAILURE;
failed_setavtransporturi:
  CLog::Log(LOGERROR, "PlayFile(%s) failed to set the playback URI", file.GetPath().c_str());
  return NPT_FAILURE;
failed_play:
  CLog::Log(LOGERROR, "PlayFile(%s) failed to start playback", file.GetPath().c_str());
  return NPT_FAILURE;
failed_waitplaying:
  CLog::Log(LOGERROR, "PlayFile(%s) failed to wait for PLAYING state", file.GetPath().c_str());
  return NPT_FAILURE;
failed_seek:
  CLog::Log(LOGERROR, "PlayFile(%s) failed to seek to start offset", file.GetPath().c_str());
  return NPT_FAILURE;
failed:
  CLog::Log(LOGERROR, "PlayFile(%s) failed", file.GetPath().c_str());
  return NPT_FAILURE;
}

bool CUPnPPlayer::OpenFile(const CFileItem& file, const CPlayerOptions& options)
{
  XbmcThreads::EndTime timeout(10000);

  /* if no path we want to attach to a already playing player */
  if (file.GetPath() == "")
  {
    NPT_CHECK_LABEL_SEVERE(
        m_control->GetTransportInfo(m_delegate->m_device, m_delegate->m_instance, m_delegate.get()),
        failed);

    NPT_CHECK_LABEL_SEVERE(WaitOnEvent(m_delegate->m_traevnt, timeout), failed);

    /* make sure the attached player is actually playing */
    const NPT_String transportState = m_delegate->GetTransportState();
    if (transportState != "PLAYING" && transportState != "PAUSED_PLAYBACK")
    {
      goto failed;
    }
  }
  else
    NPT_CHECK_LABEL_SEVERE(PlayFile(file, options, timeout), failed);

  if (!IsRunning())
    Create();

  m_stopremote = true;
  m_started = true;

  if (file.IsVideo())
  {
    m_hasVideo = true;
  }
  else if (file.IsAudio())
  {
    m_hasAudio = true;
  }

  m_callback.OnPlayBackStarted(file);
  m_callback.OnAVStarted(file);
  NPT_CHECK_LABEL_SEVERE(
      m_control->GetPositionInfo(m_delegate->m_device, m_delegate->m_instance, m_delegate.get()),
      failed);
  NPT_CHECK_LABEL_SEVERE(
      m_control->GetMediaInfo(m_delegate->m_device, m_delegate->m_instance, m_delegate.get()),
      failed);

  m_updateTimer.Set(0);

  return true;
failed:
  CLog::Log(LOGERROR, "OpenFile(%s) failed to open file", file.GetPath().c_str());
  return false;
}

bool CUPnPPlayer::QueueNextFile(const CFileItem& file)
{
  CFileItem item(file);
  NPT_Reference<CThumbLoader> thumb_loader;
  NPT_Reference<PLT_MediaObject> obj;
  NPT_String path(file.GetPath().c_str());
  NPT_String tmp;

  if (file.IsVideoDb())
    thumb_loader = NPT_Reference<CThumbLoader>(new CVideoThumbLoader());
  else if (item.IsMusicDb())
    thumb_loader = NPT_Reference<CThumbLoader>(new CMusicThumbLoader());

  obj = BuildObject(item, path, false, thumb_loader, NULL, CUPnP::GetServer(), UPnPPlayer);
  if (!obj.IsNull())
  {
    NPT_CHECK_LABEL_SEVERE(PLT_Didl::ToDidl(*obj, "", tmp), failed);
    tmp.Insert(didl_header, 0);
    tmp.Append(didl_footer);
  }

  NPT_CHECK_LABEL_WARNING(
      m_control->SetNextAVTransportURI(m_delegate->m_device, m_delegate->m_instance,
                                       file.GetPath().c_str(), (const char*)tmp, m_delegate.get()),
      failed);
  if (!m_delegate->m_resevent.WaitMSec(10000))
    goto failed;
  NPT_CHECK_LABEL_WARNING(m_delegate->m_resstatus, failed);
  return true;

failed:
  CLog::Log(LOGERROR, "QueueNextFile(%s) failed to queue file", file.GetPath().c_str());
  return false;
}

bool CUPnPPlayer::CloseFile(bool reopen)
{
  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  if (m_stopremote)
  {
    NPT_CHECK_LABEL(m_control->Stop(m_delegate->m_device, m_delegate->m_instance, m_delegate.get()),
                    failed);
    if (!m_delegate->m_resevent.WaitMSec(10000))
      goto failed;
    NPT_CHECK_LABEL(m_delegate->m_resstatus, failed);
  }

  if (m_started)
  {
    m_started = false;
    m_callback.OnPlayBackStopped();
  }
  StopThread(true);
  CServiceBroker::GetDataCacheCore().Reset();
  return true;
failed:
  CLog::Log(LOGERROR, "CloseFile - unable to stop playback");
  CServiceBroker::GetDataCacheCore().Reset();
  return false;
}

void CUPnPPlayer::Pause()
{
  if (IsPaused())
  {
    NPT_CHECK_LABEL(
        m_control->Play(m_delegate->m_device, m_delegate->m_instance, "1", m_delegate.get()),
        failed);
    CDataCacheCore::GetInstance().SetSpeed(1.0, 1.0);
    m_callback.OnPlayBackResumed();
  }
  else
  {
    NPT_CHECK_LABEL(
        m_control->Pause(m_delegate->m_device, m_delegate->m_instance, m_delegate.get()), failed);
    CDataCacheCore::GetInstance().SetSpeed(1.0, 0.0);
    m_callback.OnPlayBackPaused();
  }

  return;
failed:
  CLog::Log(LOGERROR, "CloseFile - unable to pause/unpause playback");
}

void CUPnPPlayer::SeekTime(int64_t ms)
{
  NPT_CHECK_LABEL(m_control->Seek(m_delegate->m_device, m_delegate->m_instance, "REL_TIME",
                                  PLT_Didl::FormatTimeStamp((NPT_UInt32)(ms / 1000)),
                                  m_delegate.get()),
                  failed);

  CDataCacheCore::GetInstance().SeekFinished(0);
  return;
failed:
  CLog::Log(LOGERROR, "SeekTime - unable to seek playback");
}

float CUPnPPlayer::GetPercentage()
{
  int64_t tot = GetTotalTime();
  if (tot)
    return 100.0f * GetTime() / tot;
  else
    return 0.0f;
}

void CUPnPPlayer::SeekPercentage(float percent)
{
  int64_t tot = GetTotalTime();
  if (tot)
    SeekTime((int64_t)(tot * percent / 100));
}

void CUPnPPlayer::Seek(bool bPlus, bool bLargeStep, bool bChapterOverride)
{
}

void CUPnPPlayer::Process()
{
  while (!m_bStop)
  {
    NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
    m_delegate->UpdatePositionInfo();

    if (m_started)
    {
      // Update player times
      CDataCacheCore& dataCacheCore = CDataCacheCore::GetInstance();
      if (m_updateTimer.IsTimePast())
      {
        dataCacheCore.SetPlayTimes(0, GetTime(), 0, GetTotalTime());
        m_updateTimer.Set(500);
      }

      // Player may be paused or resumed from the target player, state needs to be synchronized to data cache core.
      if (IsPaused() && dataCacheCore.GetSpeed() > 0.0f)
      {
        m_callback.OnPlayBackPaused();
        dataCacheCore.SetSpeed(1.0, 0.0);
      }
      else if (!IsPaused() && dataCacheCore.GetSpeed() == 0.0f)
      {
        m_callback.OnPlayBackResumed();
        dataCacheCore.SetSpeed(1.0, 1.0);
      }

      if (m_delegate->GetTransportState() == "STOPPED")
      {
        CLog::Log(LOGINFO, "Transport state flagged as STOPPED. Triggering OnPlayBackEnded.");
        m_started = false;
        m_callback.OnPlayBackEnded();
      }
    }
  }
failed:
  CloseFile();
}

bool CUPnPPlayer::IsPlaying() const
{
  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  return m_delegate->GetTransportState() != "STOPPED";
failed:
  return false;
}

bool CUPnPPlayer::IsPaused() const
{
  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  return m_delegate->GetTransportState() == "PAUSED_PLAYBACK";
failed:
  return false;
}

void CUPnPPlayer::SetVolume(float volume)
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_SERVICES_UPNPPLAYERVOLUMESYNC))
  {
    return;
  }

  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  NPT_CHECK_LABEL(m_control->SetVolume(m_delegate->m_device, m_delegate->m_instance, "Master",
                                       (int)(volume * 100), m_delegate.get()),
                  failed);
  return;
failed:
  CLog::Log(LOGERROR, "- unable to set volume");
}

int64_t CUPnPPlayer::GetTime()
{
  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  return m_delegate->m_posinfo.rel_time.ToMillis();
failed:
  return 0;
}

int64_t CUPnPPlayer::GetTotalTime()
{
  NPT_CHECK_POINTER_LABEL_SEVERE(m_delegate, failed);
  return m_delegate->m_posinfo.track_duration.ToMillis();
failed:
  return 0;
};

bool CUPnPPlayer::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_STOP:
      if (IsPlaying())
      {
        //stop on remote system
        m_stopremote = HELPERS::ShowYesNoDialogText(37022, 37023) == DialogResponse::CHOICE_YES;

        return false; /* let normal code handle the action */
      }
      [[fallthrough]];
    default:
      return false;
  }
}

void CUPnPPlayer::SetSpeed(float speed)
{
  if (IsPaused() && speed == 1.0f)
  {
    NPT_CHECK_LABEL(
        m_control->Play(m_delegate->m_device, m_delegate->m_instance, "1", m_delegate.get()),
        failed);
    m_callback.OnPlayBackResumed();
    CDataCacheCore::GetInstance().SetSpeed(1.0, 1.0);
  }
  return;
failed:
  CLog::Log(LOGERROR, "- unable to set speed");
}

void CUPnPPlayer::OnExit()
{
  if (m_started)
  {
    m_callback.OnPlayBackEnded();
  }
}

} /* namespace UPNP */
