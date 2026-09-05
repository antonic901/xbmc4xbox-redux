/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Application.h"

#include "Autorun.h"
#include "GUIInfoManager.h"
#include "LangInfo.h"
#include "PlayListPlayer.h"
#include "SectionLoader.h"
#include "ServiceManager.h"
#include "URL.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/RepositoryUpdater.h"
#include "addons/Service.h"
#include "addons/Skin.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "application/ApplicationActionListeners.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationSkinHandling.h"
#include "application/ApplicationStackHelper.h"
#include "application/ApplicationVolumeHandling.h"
#include "application/ApplicationXbox.h"
#include "cores/IPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogCache.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIFontManager.h"
#include "guilib/TextureManager.h"
#include "input/InputManager.h"
#include "interfaces/builtins/Builtins.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "music/MusicLibraryQueue.h"
#include "music/tags/MusicInfoTag.h"
#include "network/Network.h"
#include "network/NetworkServices.h"
#include "playlists/PlayListFactory.h"
#include "threads/SystemClock.h"
#include "threads/platform/win/Win32Exception.h"
#include "utils/ContentUtils.h"
#include "utils/JobManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/Variant.h"
#include "video/Bookmark.h"
#include "video/VideoLibraryQueue.h"

#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif
#include "GUILargeTextureManager.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/DllLibCurl.h"
#include "filesystem/File.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/ButtonTranslator.h"
#include "input/KeyboardLayoutManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "playlists/PlayList.h"
#include "playlists/SmartPlayList.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/RegExp.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/xbox/WinSystemXbox.h"

#include <cmath>

#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#include "filesystem/UPnPDirectory.h"
#endif
#include "PartyModeManager.h"
#include "interfaces/AnnouncementManager.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "music/MusicUtils.h"
#include "music/MusicThumbLoader.h"

// Windows includes
#include "guilib/GUIWindowManager.h"
#include "video/PlayerController.h"

// Dialog includes
#include "addons/gui/GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "video/dialogs/GUIDialogVideoBookmarks.h"

#include "DatabaseManager.h"
#include "storage/DetectDVDType.h"
#include "storage/MediaManager.h"
#include "utils/AlarmClock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include "FileItem.h"
#include "addons/AddonSystemSettings.h"
#include "pictures/GUIWindowSlideShow.h"
#include "utils/CharsetConverter.h"

#include "platform/xbox/XKHDD.h"
#include "platform/xbox/filesystem/MemoryUnitManager.h"

#include <boost/bind.hpp>
#include <boost/move/make_unique.hpp>

using namespace ADDON;
using namespace XFILE;
#ifdef HAS_OPTICAL_DRIVE
using namespace MEDIA_DETECT;
#endif
using namespace VIDEO;
using namespace MUSIC_INFO;
using namespace KODI;
using namespace KODI::MESSAGING;

using namespace XbmcThreads;

#define MAX_FFWD_SPEED 5

// uncomment this if you want to use release libs in the debug build.
// Atm this saves you 7 mb of memory
#define USE_RELEASE_LIBS

#ifdef HAS_LCD
#pragma comment (lib,"lib/libXenium/XeniumSPIg.lib")
#endif

#if defined(_DEBUG) && !defined(USE_RELEASE_LIBS)
  #pragma comment (lib,"lib/libsmb/libsmbd.lib")      // SECTIONNAME=LIBSMB
  #pragma comment (lib,"lib/libGoAhead/goaheadd.lib") // SECTIONNAME=LIBHTTP
  #pragma comment (lib,"lib/sqLite/libSQLite3d.lib")
  #pragma comment (lib,"lib/libshout/libshoutd.lib" )
  #pragma comment (lib,"lib/libcdio/libcdiod.lib" )
  #pragma comment (lib,"lib/libiconv/libiconvd.lib")
  #pragma comment (lib,"lib/libfribidi/libfribidid.lib")
  #pragma comment (lib,"lib/libpcre/libpcred.lib")
#else
  #pragma comment (lib,"lib/libsmb/libsmb.lib")
  #pragma comment (lib,"lib/libGoAhead/goahead.lib")
  #pragma comment (lib,"lib/sqLite/libSQLite3.lib")
  #pragma comment (lib,"lib/libcdio/libcdio.lib")
  #pragma comment (lib,"lib/libiconv/libiconv.lib")
  #pragma comment (lib,"lib/libfribidi/libfribidi.lib")
  #pragma comment (lib,"lib/libpcre/libpcre.lib")
#endif

CApplication::CApplication(void)
  :
#ifdef HAS_OPTICAL_DRIVE
    m_Autorun(new CAutorun()),
#endif
    m_itemCurrentFile(boost::make_shared<CFileItem>()),
    m_bInitializing(true),
    m_nextPlaylistItem(-1),
    m_bStop(false)
{
  TiXmlBase::SetCondenseWhiteSpace(false);

  // register application components
  RegisterComponent(boost::shared_ptr<CApplicationActionListeners>(new CApplicationActionListeners(m_critSection)));
  RegisterComponent(boost::make_shared<CApplicationPlayer>());
  RegisterComponent(boost::make_shared<CApplicationPowerHandling>());
  RegisterComponent(boost::shared_ptr<CApplicationSkinHandling>(new CApplicationSkinHandling(this, this, m_bInitializing)));
  RegisterComponent(boost::make_shared<CApplicationVolumeHandling>());
  RegisterComponent(boost::make_shared<CApplicationStackHelper>());
  RegisterComponent(boost::make_shared<CApplicationXbox>());
}

CApplication::~CApplication(void)
{
  DeregisterComponent(typeid(CApplicationXbox));
  DeregisterComponent(typeid(CApplicationStackHelper));
  DeregisterComponent(typeid(CApplicationVolumeHandling));
  DeregisterComponent(typeid(CApplicationSkinHandling));
  DeregisterComponent(typeid(CApplicationPowerHandling));
  DeregisterComponent(typeid(CApplicationPlayer));
  DeregisterComponent(typeid(CApplicationActionListeners));
}

extern "C" void __stdcall init_emu_environ();
extern "C" void __stdcall update_emu_environ();
extern "C" void __stdcall cleanup_emu_environ();

LONG WINAPI CApplication::UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
  PCSTR pExceptionString = "Unknown exception code";

  #define STRINGIFY_EXCEPTION(code) case code: pExceptionString = #code; break
  switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
  {
    STRINGIFY_EXCEPTION(EXCEPTION_ACCESS_VIOLATION);
    STRINGIFY_EXCEPTION(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
    STRINGIFY_EXCEPTION(EXCEPTION_BREAKPOINT);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_DENORMAL_OPERAND);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_DIVIDE_BY_ZERO);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_INEXACT_RESULT);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_INVALID_OPERATION);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_OVERFLOW);
    STRINGIFY_EXCEPTION(EXCEPTION_FLT_STACK_CHECK);
    STRINGIFY_EXCEPTION(EXCEPTION_ILLEGAL_INSTRUCTION);
    STRINGIFY_EXCEPTION(EXCEPTION_INT_DIVIDE_BY_ZERO);
    STRINGIFY_EXCEPTION(EXCEPTION_INT_OVERFLOW);
    STRINGIFY_EXCEPTION(EXCEPTION_INVALID_DISPOSITION);
    STRINGIFY_EXCEPTION(EXCEPTION_NONCONTINUABLE_EXCEPTION);
    STRINGIFY_EXCEPTION(EXCEPTION_SINGLE_STEP);
  }
  #undef STRINGIFY_EXCEPTION

  CLog::Log(LOGFATAL, "%s (0x%08x)\n at 0x%08x", pExceptionString, ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo->ExceptionRecord->ExceptionAddress);
  return ExceptionInfo->ExceptionRecord->ExceptionCode;
}

bool CApplication::Create()
{
  m_bStop = false;

  RegisterSettings();

  // Register JobManager service
  CServiceBroker::RegisterJobManager(boost::make_shared<CJobManager>());

  // Announcement service
  m_pAnnouncementManager = boost::make_shared<ANNOUNCEMENT::CAnnouncementManager>();
  m_pAnnouncementManager->Start();
  CServiceBroker::RegisterAnnouncementManager(m_pAnnouncementManager);

  const boost::shared_ptr<KODI::MESSAGING::CApplicationMessenger> appMessenger = boost::make_shared<CApplicationMessenger>();
  CServiceBroker::RegisterAppMessenger(appMessenger);

  const boost::shared_ptr<KODI::KEYBOARD::CKeyboardLayoutManager> keyboardLayoutManager = boost::make_shared<KEYBOARD::CKeyboardLayoutManager>();
  CServiceBroker::RegisterKeyboardLayoutManager(keyboardLayoutManager);

  m_ServiceManager = boost::movelib::make_unique<CServiceManager>();

  if (!m_ServiceManager->InitStageOne())
  {
    return false;
  }

  // here we register all global classes for the CApplicationMessenger,
  // after that we can send messages to the corresponding modules
  appMessenger->RegisterReceiver(this);
  appMessenger->RegisterReceiver(&CServiceBroker::GetPlaylistPlayer());
  appMessenger->SetGUIThread(CThread::GetCurrentThreadId());
  appMessenger->SetProcessThread(CThread::GetCurrentThreadId());

  // copy required files
  CUtil::CopyUserDataIfNeeded("special://masterprofile/", "RssFeeds.xml");
  CUtil::CopyUserDataIfNeeded("special://masterprofile/", "favourites.xml");
  CUtil::CopyUserDataIfNeeded("special://masterprofile/", "Lircmap.xml");

  CLog::Init(CSpecialProtocol::TranslatePath("special://logpath").c_str());

  CDirectory::Create("special://xbmc/addons");

  // Init our DllLoaders emu env
  init_emu_environ();

  /* install win32 exception translator, win32 exceptions
   * can now be caught using c++ try catch */
  win32_exception::install_handler();

  PrintStartupLog();

  CLog::Log(LOGINFO, "loading settings");
  const boost::shared_ptr<CSettingsComponent> settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent->Load())
    return false;

  CLog::Log(LOGINFO, "creating subdirectories");
  const boost::shared_ptr<CProfileManager> profileManager = settingsComponent->GetProfileManager();
  const boost::shared_ptr<CSettings> settings = settingsComponent->GetSettings();
  CLog::Log(LOGINFO, "userdata folder: %s",
            CURL::GetRedacted(profileManager->GetProfileUserDataFolder()).c_str());
  CLog::Log(LOGINFO, "recording folder: %s",
            CURL::GetRedacted(settings->GetString(CSettings::SETTING_AUDIOCDS_RECORDINGPATH)).c_str());
  CLog::Log(LOGINFO, "screenshots folder: %s",
            CURL::GetRedacted(settings->GetString(CSettings::SETTING_DEBUG_SCREENSHOTPATH)).c_str());
  CDirectory::Create(profileManager->GetUserDataFolder());
  CDirectory::Create(profileManager->GetProfileUserDataFolder());
  profileManager->CreateProfileFolders();

  update_emu_environ();//apply the GUI settings

  if (!m_ServiceManager->InitStageTwo(
          settingsComponent->GetProfileManager()->GetProfileUserDataFolder()))
  {
    return false;
  }

  // initialize m_replayGainSettings
  GetComponent<CApplicationVolumeHandling>()->CacheReplayGainSettings(*settings);

  // load the keyboard layouts
  if (!keyboardLayoutManager->Load())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to load keyboard layouts");
    return false;
  }

  GetComponent<CApplicationXbox>()->OnCreate();

  // show recovery console on fatal error instead of freezing
  CLog::Log(LOGINFO, "install unhandled exception filter");
  SetUnhandledExceptionFilter(UnhandledExceptionFilter);

  CUtil::InitRandomSeed();

  return true;
}

bool CApplication::CreateGUI()
{
  m_frameMoveGuard.lock();

  m_pWinSystem = CWinSystemXbox::CreateWinSystem();
  CServiceBroker::RegisterWinSystem(m_pWinSystem.get());

  if (!m_pWinSystem->InitWindowSystem())
  {
    CLog::Log(LOGDEBUG, "CApplication::%s - unable to init windowing system", __FUNCTION__);
    m_pWinSystem->DestroyWindowSystem();
    m_pWinSystem.reset();
    CServiceBroker::UnregisterWinSystem();
    return false;
  }

  // Retrieve the matching resolution based on GUI settings
  bool sav_res = false;
  CDisplaySettings::GetInstance().SetCurrentResolution(CDisplaySettings::GetInstance().GetDisplayResolution());
  CLog::Log(LOGINFO, "Checking resolution %i",
            CDisplaySettings::GetInstance().GetCurrentResolution());

  const boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  if (!CServiceBroker::GetWinSystem()->GetGfxContext().IsValidResolution(CDisplaySettings::GetInstance().GetCurrentResolution()))
  {
    // Oh uh - doesn't look good for starting in their wanted screenmode
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    CDisplaySettings::GetInstance().SetCurrentResolution(RES_AUTORES);
    sav_res = true;
  }
  if (!InitWindow())
  {
    return false;
  }

  // Set default screen saver mode
  boost::shared_ptr<CSettingString> screensaverModeSetting = boost::static_pointer_cast<CSettingString>(settings->GetSetting(CSettings::SETTING_SCREENSAVER_MODE));
  screensaverModeSetting->SetDefault("screensaver.xbmc.builtin.dim");

  if (sav_res)
    CDisplaySettings::GetInstance().SetCurrentResolution(RES_AUTORES, true);

  m_pGUI = boost::movelib::make_unique<CGUIComponent>();
  m_pGUI->Init();

  // Splash requires gui component!!
  CServiceBroker::GetRenderSystem()->ShowSplash("");

  // The key mappings may already have been loaded by a peripheral
  CLog::Log(LOGINFO, "load keymapping");
  if (!CButtonTranslator::GetInstance().Load())
    return false;

  RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
  CLog::Log(LOGINFO, "GUI format %ix%i, Display %s", info.iWidth, info.iHeight, info.strMode.c_str());

  return true;
}

bool CApplication::InitWindow(RESOLUTION res)
{
  if (res == RES_INVALID)
    res = CDisplaySettings::GetInstance().GetCurrentResolution();

  if (!CServiceBroker::GetWinSystem()->CreateNewWindow("xbox",
                                                      true, CDisplaySettings::GetInstance().GetResolutionInfo(res)))
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to create window");
    return false;
  }

  if (!CServiceBroker::GetRenderSystem()->InitRenderSystem())
  {
    CLog::Log(LOGFATAL, "CApplication::Create: Unable to init rendering system");
    return false;
  }
  // set GUI res and force the clear of the screen
  CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(res, TRUE, true);
  return true;
}

void initializeDatabaseManager(CDatabaseManager& databaseManager, CEvent& event)
{
  databaseManager.Initialize();
  event.Set();
}

void initializeFontManager(GUIFontManager& guiFontManager, CEvent& event)
{
  guiFontManager.Initialize();
  event.Set();
}

void checkForAddonUpdates(CEvent& event, ADDON::AddonInfos& incompatibleAddons)
{
  if (CServiceBroker::GetRepositoryUpdater().CheckForUpdates())
    CServiceBroker::GetRepositoryUpdater().Await();

  incompatibleAddons = CServiceBroker::GetAddonMgr().MigrateAddons();
  event.Set();
}

bool CApplication::Initialize()
{
  GetComponent<CApplicationXbox>()->StartServices();

  // load the language and its translated strings
  if (!LoadLanguage(false))
    return false;

  // load media manager sources (e.g. root addon type sources depend on language strings to be available)
  CServiceBroker::GetMediaManager().LoadSources();

  const boost::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  m_ServiceManager->GetNetwork().WaitForNet();

  // initialize (and update as needed) our databases
  CDatabaseManager &databaseManager = m_ServiceManager->GetDatabaseManager();

  CEvent event(true);
  CServiceBroker::GetJobManager()->Submit(boost::bind(&initializeDatabaseManager, boost::ref(databaseManager), boost::ref(event)));

  std::string localizedStr = g_localizeStrings.Get(24150);
  int iDots = 1;
  while (!event.WaitMSec(1000))
  {
    if (databaseManager.IsUpgrading())
      CServiceBroker::GetRenderSystem()->ShowSplash(std::string(iDots, ' ') + localizedStr + std::string(iDots, '.'));

    if (iDots == 3)
      iDots = 1;
    else
      ++iDots;
  }
  CServiceBroker::GetRenderSystem()->ShowSplash("");

  // Initialize GUI font manager to build/update fonts cache
  //! @todo Move GUIFontManager into service broker and drop the global reference
  event.Reset();
  GUIFontManager& guiFontManager = g_fontManager;
  CServiceBroker::GetJobManager()->Submit(boost::bind(&initializeFontManager, boost::ref(guiFontManager), boost::ref(event)));
  localizedStr = g_localizeStrings.Get(39175);
  iDots = 1;
  while (!event.WaitMSec(1000))
  {
    if (g_fontManager.IsUpdating())
      CServiceBroker::GetRenderSystem()->ShowSplash(std::string(iDots, ' ') + localizedStr +
                                                    std::string(iDots, '.'));

    if (iDots == 3)
      iDots = 1;
    else
      ++iDots;
  }
  CServiceBroker::GetRenderSystem()->ShowSplash("");

  // GUI depends on seek handler
  GetComponent<CApplicationPlayer>()->GetSeekHandler().Configure();

  const boost::shared_ptr<CApplicationSkinHandling> skinHandling = GetComponent<CApplicationSkinHandling>();

  bool uiInitializationFinished = false;

  if (CServiceBroker::GetGUI()->GetWindowManager().Initialized())
  {
    const boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

    CServiceBroker::GetGUI()->GetWindowManager().CreateWindows();

    skinHandling->m_confirmSkinChange = false;

    std::vector<AddonInfoPtr> incompatibleAddons;
    event.Reset();

    // Addon migration
    if (CServiceBroker::GetAddonMgr().GetIncompatibleEnabledAddonInfos(incompatibleAddons))
    {
      if (CAddonSystemSettings::GetInstance().GetAddonAutoUpdateMode() == AUTO_UPDATES_ON)
      {
        CServiceBroker::GetJobManager()->Submit(
            boost::bind(&checkForAddonUpdates, boost::ref(event), incompatibleAddons),
            CJob::PRIORITY_DEDICATED);
        localizedStr = g_localizeStrings.Get(24151);
        iDots = 1;
        while (!event.WaitMSec(1000))
        {
          CServiceBroker::GetRenderSystem()->ShowSplash(std::string(iDots, ' ') + localizedStr +
                                                        std::string(iDots, '.'));
          if (iDots == 3)
            iDots = 1;
          else
            ++iDots;
        }
        m_incompatibleAddons = incompatibleAddons;
      }
      else
      {
        // If no update is active disable all incompatible addons during start
        m_incompatibleAddons =
            CServiceBroker::GetAddonMgr().DisableIncompatibleAddons(incompatibleAddons);
      }
    }

    // Start splashscreen and load skin
    CServiceBroker::GetRenderSystem()->ShowSplash("");
    skinHandling->m_confirmSkinChange = true;

    SettingPtr setting= settings->GetSetting(CSettings::SETTING_LOOKANDFEEL_SKIN);
    if (!setting)
    {
      CLog::Log(LOGFATAL, "Failed to load setting for: %s", CSettings::SETTING_LOOKANDFEEL_SKIN);
      return false;
    }

    CServiceBroker::RegisterTextureCache(boost::make_shared<CTextureCache>());

    std::string skinId = settings->GetString(CSettings::SETTING_LOOKANDFEEL_SKIN);
    if (!skinHandling->LoadSkin(skinId))
    {
      CLog::Log(LOGERROR, "Failed to load skin '%s'", skinId.c_str());
      std::string defaultSkin =
          boost::static_pointer_cast<const CSettingString>(setting)->GetDefault();
      if (!skinHandling->LoadSkin(defaultSkin))
      {
        CLog::Log(LOGFATAL, "Default skin '%s' could not be loaded! Terminating..", defaultSkin.c_str());
        return false;
      }
    }

    // initialize splash window after splash screen disappears
    // because we need a real window in the background which gets
    // rendered while we load the main window or enter the master lock key
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SPLASH);

    if (settings->GetBool(CSettings::SETTING_MASTERLOCK_STARTUPLOCK) &&
        profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
        !profileManager->GetMasterProfile().getLockCode().empty())
    {
      g_passwordManager.CheckStartUpLock();
    }

    // check if we should use the login screen
    if (profileManager->UsingLoginScreen())
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_LOGIN_SCREEN);
    }
    else
    {
      // activate the configured start window
      int firstWindow = g_SkinInfo->GetFirstWindow();
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(firstWindow);

      if (CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_STARTUP_ANIM))
      {
        CLog::Log(LOGWARNING, "CApplication::Initialize - startup.xml taints init process");
      }

      // the startup window is considered part of the initialization as it most likely switches to the final window
      uiInitializationFinished = firstWindow != WINDOW_STARTUP_ANIM;
    }
  }
  else //No GUI Created
  {
    uiInitializationFinished = true;
  }

  if (!m_ServiceManager->InitStageThree(profileManager))
  {
    CLog::Log(LOGERROR, "Application - Init3 failed");
  }

  g_sysinfo.Refresh();

  CLog::Log(LOGINFO, "removing tempfiles");
  CUtil::RemoveTempFiles();

  if (!profileManager->UsingLoginScreen())
  {
    UpdateLibraries();
    SetLoggingIn(false);
  }

  m_slowTimer.StartZero();

  // register action listeners
  const boost::shared_ptr<CApplicationActionListeners> appListener = GetComponent<CApplicationActionListeners>();
  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  appListener->RegisterActionListener(&appPlayer->GetSeekHandler());
  appListener->RegisterActionListener(&CPlayerController::GetInstance());

  CServiceBroker::GetRepositoryUpdater().Start();
  if (!profileManager->UsingLoginScreen())
    CServiceBroker::GetServiceAddons().Start();

  CLog::Log(LOGINFO, "initialize done");

  const boost::shared_ptr<CApplicationPowerHandling> appPower = GetComponent<CApplicationPowerHandling>();
  // reset our screensaver (starts timers etc.)
  appPower->ResetScreenSaver();

  // if the user interfaces has been fully initialized let everyone know
  if (uiInitializationFinished)
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UI_READY);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }

  return true;
}

bool CApplication::OnSettingsSaving() const
{
  // don't save settings when we're busy stopping the application
  // a lot of screens try to save settings on deinit and deinit is
  // called for every screen when the application is stopping
  return !m_bStop;
}

void CApplication::Render()
{
  // do not render if we are stopped or in background
  if (m_bStop)
    return;

  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  const boost::shared_ptr<CApplicationPowerHandling> appPower = GetComponent<CApplicationPowerHandling>();

  bool hasRendered = false;

  if (CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo() &&
      !appPlayer->IsPausedPlayback())
  {
    // On Xbox when CGUIWindowFullScreen is active and video playback is not paused,
    // rendering is happening inside CXBoxRenderManager thread and not main render loop.
    // Because of that we need to skip main render loop.
    // TODO: understand why it's done like this and if it's possible to move videoplayback inside main render loop
    Sleep(50);
    appPower->ResetScreenSaver();
    CServiceBroker::GetGUI()->GetInfoManager().ResetCache();
    return;
  }

  if (!CServiceBroker::GetRenderSystem()->BeginRender())
    return;

  // render gui layer
  if (appPower->GetRenderGUI())
  {
    {
      hasRendered |= CServiceBroker::GetGUI()->GetWindowManager().Render();
    }
    // execute post rendering actions (finalize window closing)
    CServiceBroker::GetGUI()->GetWindowManager().AfterRender();
  }

  CServiceBroker::GetRenderSystem()->EndRender();

  // reset our info cache - we do this at the end of Render so that it is
  // fresh for the next process(), or after a windowclose animation (where process()
  // isn't called)
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
  infoMgr.ResetCache();
  infoMgr.GetInfoProviders().GetGUIControlsInfoProvider().ResetContainerMovingCache();

  if (hasRendered)
  {
    infoMgr.GetInfoProviders().GetSystemInfoProvider().UpdateFPS();
  }

  CServiceBroker::GetWinSystem()->GetGfxContext().Flip(hasRendered, false);

  CTimeUtils::UpdateFrameTime();
}

bool CApplication::OnAction(const CAction &action)
{
  // special case for switching between GUI & fullscreen mode.
  if (action.GetID() == ACTION_SHOW_GUI)
  { // Switch to fullscreen mode if we can
    CGUIComponent* gui = CServiceBroker::GetGUI();
    if (gui)
    {
      if (gui->GetWindowManager().SwitchToFullScreen())
      {
        GetComponent<CApplicationPowerHandling>()->m_navigationTimer.StartZero();
        return true;
      }
    }
  }

  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();

  if (action.GetID() == ACTION_CREATE_EPISODE_BOOKMARK)
  {
    CGUIDialogVideoBookmarks::OnAddEpisodeBookmark();
  }
  if (action.GetID() == ACTION_CREATE_BOOKMARK)
  {
    CGUIDialogVideoBookmarks::OnAddBookmark();
  }

  // The action PLAYPAUSE behaves as ACTION_PAUSE if we are currently
  // playing or ACTION_PLAYER_PLAY if we are seeking (FF/RW) or not playing.
  if (action.GetID() == ACTION_PLAYER_PLAYPAUSE)
  {
    CGUIWindowSlideShow *slideShow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
    if ((appPlayer->IsPlaying() && appPlayer->GetPlaySpeed() == 1) ||
        (slideShow->InSlideShow() && !slideShow->IsPaused()))
      return OnAction(CAction(ACTION_PAUSE));
    else
      return OnAction(CAction(ACTION_PLAYER_PLAY));
  }

  // in normal case
  // just pass the action to the current window and let it handle it
  if (CServiceBroker::GetGUI()->GetWindowManager().OnAction(action))
  {
    GetComponent<CApplicationPowerHandling>()->ResetNavigationTimer();
    return true;
  }

  // handle extra global presses

  // notify action listeners
  if (GetComponent<CApplicationActionListeners>()->NotifyActionListeners(action))
    return true;

  // screenshot : take a screenshot :)
  if (action.GetID() == ACTION_TAKE_SCREENSHOT)
  {
    CUtil::TakeScreenshot();
    return true;
  }
  // built in functions : execute the built-in
  if (action.GetID() == ACTION_BUILT_IN_FUNCTION)
  {
    CBuiltins::GetInstance().Execute(action.GetName());
    GetComponent<CApplicationPowerHandling>()->ResetNavigationTimer();
    return true;
  }

  // reload keymaps
  if (action.GetID() == ACTION_RELOAD_KEYMAPS)
    CButtonTranslator::GetInstance().Load();

  // show info : Shows the current video or song information
  if (action.GetID() == ACTION_SHOW_INFO)
  {
    CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().ToggleShowInfo();
    return true;
  }

  if (action.GetID() == ACTION_SET_RATING && appPlayer->IsPlayingAudio())
  {
    int userrating = MUSIC_UTILS::ShowSelectRatingDialog(m_itemCurrentFile->GetMusicInfoTag()->GetUserrating());
    if (userrating < 0) // Nothing selected, so user rating unchanged
      return true;
    userrating = std::min(userrating, 10);
    if (userrating != m_itemCurrentFile->GetMusicInfoTag()->GetUserrating())
    {
      m_itemCurrentFile->GetMusicInfoTag()->SetUserrating(userrating);
      // Mirror changes to GUI item
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_itemCurrentFile);

      // Asynchronously update song userrating in music library
      MUSIC_UTILS::UpdateSongRatingJob(m_itemCurrentFile, userrating);

      // Tell all windows (e.g. playlistplayer, media windows) to update the fileitem
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }
    return true;
  }

  else if ((action.GetID() == ACTION_INCREASE_RATING || action.GetID() == ACTION_DECREASE_RATING) &&
           appPlayer->IsPlayingAudio())
  {
    int userrating = m_itemCurrentFile->GetMusicInfoTag()->GetUserrating();
    bool needsUpdate(false);
    if (userrating > 0 && action.GetID() == ACTION_DECREASE_RATING)
    {
      m_itemCurrentFile->GetMusicInfoTag()->SetUserrating(userrating - 1);
      needsUpdate = true;
    }
    else if (userrating < 10 && action.GetID() == ACTION_INCREASE_RATING)
    {
      m_itemCurrentFile->GetMusicInfoTag()->SetUserrating(userrating + 1);
      needsUpdate = true;
    }
    if (needsUpdate)
    {
      // Mirror changes to current GUI item
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_itemCurrentFile);

      // Asynchronously update song userrating in music library
      MUSIC_UTILS::UpdateSongRatingJob(m_itemCurrentFile, m_itemCurrentFile->GetMusicInfoTag()->GetUserrating());

      // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }

    return true;
  }
  else if ((action.GetID() == ACTION_INCREASE_RATING || action.GetID() == ACTION_DECREASE_RATING) &&
           appPlayer->IsPlayingVideo())
  {
    int rating = m_itemCurrentFile->GetVideoInfoTag()->m_iUserRating;
    bool needsUpdate(false);
    if (rating > 1 && action.GetID() == ACTION_DECREASE_RATING)
    {
      m_itemCurrentFile->GetVideoInfoTag()->m_iUserRating = rating - 1;
      needsUpdate = true;
    }
    else if (rating < 10 && action.GetID() == ACTION_INCREASE_RATING)
    {
      m_itemCurrentFile->GetVideoInfoTag()->m_iUserRating = rating + 1;
      needsUpdate = true;
    }
    if (needsUpdate)
    {
      // Mirror changes to GUI item
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_itemCurrentFile);

      CVideoDatabase db;
      if (db.Open())
      {
        db.SetVideoUserRating(m_itemCurrentFile->GetVideoInfoTag()->m_iDbId,
                              m_itemCurrentFile->GetVideoInfoTag()->m_iUserRating,
                              m_itemCurrentFile->GetVideoInfoTag()->m_type);
        db.Close();
      }
      // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile);
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }
    return true;
  }

  // Now check with the playlist player if action can be handled.
  // In case of ACTION_PREV_ITEM, we only allow the playlist player to take it if we're less than ACTION_PREV_ITEM_THRESHOLD seconds into playback.
  if (!(action.GetID() == ACTION_PREV_ITEM && appPlayer->CanSeek() &&
        GetTime() > ACTION_PREV_ITEM_THRESHOLD))
  {
    if (CServiceBroker::GetPlaylistPlayer().OnAction(action))
      return true;
  }

  // Now check with the player if action can be handled.
  bool bNotifyPlayer = false;
  if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
    bNotifyPlayer = true;
  else if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME)
    bNotifyPlayer = true;
  else if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_DIALOG_VIDEO_OSD)
  {
    switch (action.GetID())
    {
      case ACTION_NEXT_ITEM:
      case ACTION_PREV_ITEM:
        bNotifyPlayer = true;
        break;
      default:
        break;
    }
  }
  else if (action.GetID() == ACTION_STOP)
    bNotifyPlayer = true;

  if (bNotifyPlayer)
  {
    if (appPlayer->OnAction(action))
      return true;
  }

  // stop : stops playing current audio song
  if (action.GetID() == ACTION_STOP)
  {
    StopPlaying();
    return true;
  }

  // In case the playlist player nor the player didn't handle PREV_ITEM, because we are past the ACTION_PREV_ITEM_THRESHOLD secs limit.
  // If so, we just jump to the start of the track.
  if (action.GetID() == ACTION_PREV_ITEM && appPlayer->CanSeek())
  {
    SeekTime(0);
    appPlayer->SetPlaySpeed(1);
    return true;
  }

  if (appPlayer->IsPlaying())
  {
    // pause : toggle pause action
    if (action.GetID() == ACTION_PAUSE)
    {
      appPlayer->Pause();
      // go back to normal play speed on unpause
      if (!appPlayer->IsPaused() && appPlayer->GetPlaySpeed() != 1)
        appPlayer->SetPlaySpeed(1);

      CGUIComponent *gui = CServiceBroker::GetGUI();
      if (gui)
        gui->GetAudioManager().Enable(appPlayer->IsPaused());
      return true;
    }
    // play: unpause or set playspeed back to normal
    if (action.GetID() == ACTION_PLAYER_PLAY)
    {
      // if currently paused - unpause
      if (appPlayer->IsPaused())
        return OnAction(CAction(ACTION_PAUSE));
      // if we do a FF/RW then go back to normal speed
      if (appPlayer->GetPlaySpeed() != 1)
        appPlayer->SetPlaySpeed(1);
      return true;
    }
    if (!appPlayer->IsPaused())
    {
      if (action.GetID() == ACTION_PLAYER_FORWARD || action.GetID() == ACTION_PLAYER_REWIND)
      {
        int playSpeed = appPlayer->GetPlaySpeed();

        if (action.GetID() == ACTION_PLAYER_REWIND && (playSpeed == 1)) // Enables Rewinding
          playSpeed *= -2;
        else if (action.GetID() == ACTION_PLAYER_REWIND && playSpeed > 1) //goes down a notch if you're FFing
          playSpeed /= 2;
        else if (action.GetID() == ACTION_PLAYER_FORWARD && playSpeed < 1) //goes up a notch if you're RWing
          playSpeed /= 2;
        else
          playSpeed *= 2;

        if (action.GetID() == ACTION_PLAYER_FORWARD && playSpeed == -1) //sets iSpeed back to 1 if -1 (didn't plan for a -1)
          playSpeed = 1;
        if (playSpeed > 32 || playSpeed < -32)
          playSpeed = 1;

        appPlayer->SetPlaySpeed(playSpeed);
        return true;
      }
      else if ((action.GetAmount() || appPlayer->GetPlaySpeed() != 1) &&
               (action.GetID() == ACTION_ANALOG_REWIND || action.GetID() == ACTION_ANALOG_FORWARD))
      {
        // calculate the speed based on the amount the button is held down
        int iPower = (int)(action.GetAmount() * MAX_FFWD_SPEED + 0.5f);
        // amount can be negative, for example rewind and forward share the same axis
        iPower = std::abs(iPower);
        // returns 0 -> MAX_FFWD_SPEED
        int iSpeed = 1 << iPower;
        if (iSpeed != 1 && action.GetID() == ACTION_ANALOG_REWIND)
          iSpeed = -iSpeed;
        appPlayer->SetPlaySpeed(iSpeed);
        if (iSpeed == 1)
          CLog::Log(LOGDEBUG,"Resetting playspeed");
        return true;
      }
    }
    // allow play to unpause
    else
    {
      if (action.GetID() == ACTION_PLAYER_PLAY)
      {
        // unpause, and set the playspeed back to normal
        appPlayer->Pause();

        CGUIComponent *gui = CServiceBroker::GetGUI();
        if (gui)
          gui->GetAudioManager().Enable(appPlayer->IsPaused());

        appPlayer->SetPlaySpeed(1);
        return true;
      }
    }
  }


  if (action.GetID() == ACTION_SWITCH_PLAYER)
  {
    const CPlayerCoreFactory &playerCoreFactory = m_ServiceManager->GetPlayerCoreFactory();

    if (appPlayer->IsPlaying())
    {
      std::vector<std::string> players;
      CFileItem item(*m_itemCurrentFile.get());
      playerCoreFactory.GetPlayers(item, players);
      std::string player = playerCoreFactory.SelectPlayerDialog(players);
      if (!player.empty())
      {
        item.SetStartOffset(CUtil::ConvertSecsToMilliSecs(GetTime()));
        PlayFile(item, player, true);
      }
    }
  }

  if (action.GetID() == ACTION_MUTE)
  {
    const boost::shared_ptr<CApplicationVolumeHandling> appVolume = GetComponent<CApplicationVolumeHandling>();
    appVolume->ToggleMute();
    appVolume->ShowVolumeBar(&action);
    return true;
  }

  if (action.GetID() == ACTION_TOGGLE_DIGITAL_ANALOG)
  {
    const boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    bool passthrough = settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH);
    settings->SetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH, !passthrough);

#ifdef _XBOX
    // Why we do this on Xbox?
    g_application.Restart();
#endif

    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SETTINGS_SYSTEM)
    {
      CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0,0,WINDOW_INVALID,CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }
    return true;
  }

  // Check for global volume control
  if ((action.GetAmount() && (action.GetID() == ACTION_VOLUME_UP || action.GetID() == ACTION_VOLUME_DOWN)) || action.GetID() == ACTION_VOLUME_SET)
  {
    const boost::shared_ptr<CApplicationVolumeHandling> appVolume = GetComponent<CApplicationVolumeHandling>();
    if (appVolume->IsMuted())
      appVolume->UnMute();
    int volume = appVolume->GetVolumeRatio();

    // calculate speed so that a full press will equal 1 second from min to max
    float speed = float(CApplicationVolumeHandling::VOLUME_MAXIMUM - CApplicationVolumeHandling::VOLUME_MINIMUM);

    if (action.GetRepeat())
      speed *= action.GetRepeat();
    else
      speed /= 50; //50 fps

    if (action.GetID() == ACTION_VOLUME_UP)
      volume += (int)((float)fabs(action.GetAmount()) * action.GetAmount() * speed);
    else if (action.GetID() == ACTION_VOLUME_DOWN)
      volume -= (int)((float)fabs(action.GetAmount()) * action.GetAmount() * speed);
    else
      volume = static_cast<int>(action.GetAmount() * speed);
    if (volume != appVolume->GetVolumeRatio())
      appVolume->SetVolume(volume, false);
    // show visual feedback of volume or passthrough indicator
    appVolume->ShowVolumeBar(&action);
    return true;
  }

  if (action.GetID() == ACTION_SHOW_PLAYLIST)
  {
    const PLAYLIST::Id playlistId = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    if (playlistId == PLAYLIST::TYPE_VIDEO &&
        CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_VIDEO_PLAYLIST)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    }
    else if (playlistId == PLAYLIST::TYPE_MUSIC &&
             CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() !=
                 WINDOW_MUSIC_PLAYLIST)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    }
    return true;
  }
  return false;
}

int CApplication::GetMessageMask()
{
  return TMSG_MASK_APPLICATION;
}

void CApplication::OnApplicationMessage(ThreadMessage* pMsg)
{
  uint32_t msg = pMsg->dwMessage;

  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();

  switch (msg)
  {
  case TMSG_POWERDOWN:
    if (Stop(0))
    {
      Sleep(200);
      XKHDD::SpindownHarddisk();
      XKUtils::XBOXPowerOff();
      while (1) { Sleep(0); }
    }
    break;

  case TMSG_QUIT:
    Stop(0);
    CBuiltins::GetInstance().Execute("XBMC.Dashboard()");
    break;

  case TMSG_SHUTDOWN:
    GetComponent<CApplicationPowerHandling>()->HandleShutdownMessage();
    break;

  case TMSG_RESTART:
  case TMSG_RESET:
    if (Stop(0))
    {
      Sleep(200);
      XKUtils::XBOXPowerCycle();
      while (1) { Sleep(0); }
    }
    break;

  case TMSG_INHIBITIDLESHUTDOWN:
    GetComponent<CApplicationPowerHandling>()->InhibitIdleShutdown(pMsg->param1 != 0);
    break;

  case TMSG_INHIBITSCREENSAVER:
    GetComponent<CApplicationPowerHandling>()->InhibitScreenSaver(pMsg->param1 != 0);
    break;

  case TMSG_ACTIVATESCREENSAVER:
    GetComponent<CApplicationPowerHandling>()->ActivateScreenSaver();
    break;

  case TMSG_RESETSCREENSAVER:
    GetComponent<CApplicationPowerHandling>()->m_bResetScreenSaver = true;
    break;

  case TMSG_VOLUME_SHOW:
  {
    CAction action(pMsg->param1);
    GetComponent<CApplicationVolumeHandling>()->ShowVolumeBar(&action);
  }
  break;

  case TMSG_NETWORKMESSAGE:
    m_ServiceManager->GetNetwork().NetworkMessage(static_cast<CNetwork::EMESSAGE>(pMsg->param1),
                                                  pMsg->param2);
    break;

  case TMSG_SETLANGUAGE:
    SetLanguage(pMsg->strParam);
    break;


  case TMSG_SWITCHTOFULLSCREEN:
  {
    CGUIComponent* gui = CServiceBroker::GetGUI();
    if (gui)
      gui->GetWindowManager().SwitchToFullScreen(true);
    break;
  }

  case TMSG_SETVIDEORESOLUTION:
    CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(static_cast<RESOLUTION>(pMsg->param1), pMsg->strParam == "true" ? TRUE : FALSE, pMsg->param2 == 1);
    break;

  case TMSG_EXECUTE_SCRIPT:
    CScriptInvocationManager::GetInstance().ExecuteAsync(pMsg->strParam);
    break;

  case TMSG_EXECUTE_BUILT_IN:
    CBuiltins::GetInstance().Execute(pMsg->strParam);
    break;

  case TMSG_PICTURE_SHOW:
  {
    CGUIWindowSlideShow *slideShow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);

    // stop playing file
    if (appPlayer->IsPlayingVideo())
      StopPlaying();

    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();

    const boost::shared_ptr<CApplicationPowerHandling> appPower = GetComponent<CApplicationPowerHandling>();
    appPower->ResetScreenSaver();
    appPower->WakeUpScreenSaverAndDPMS();

    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_SLIDESHOW)
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SLIDESHOW);
    if (URIUtils::IsZIP(pMsg->strParam) || URIUtils::IsRAR(pMsg->strParam)) // actually a cbz/cbr
    {
      CFileItemList items;
      CURL pathToUrl;
      if (URIUtils::IsZIP(pMsg->strParam))
        pathToUrl = URIUtils::CreateArchivePath("zip", CURL(pMsg->strParam), "");
      else
        pathToUrl = URIUtils::CreateArchivePath("rar", CURL(pMsg->strParam), "");

      CUtil::GetRecursiveListing(pathToUrl.Get(), items, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pictureExtensions, XFILE::DIR_FLAG_NO_FILE_DIRS);
      if (items.Size() > 0)
      {
        slideShow->Reset();
        for (int i = 0; i<items.Size(); ++i)
        {
          slideShow->Add(items[i].get());
        }
        slideShow->Select(items[0]->GetPath());
      }
    }
    else
    {
      CFileItem item(pMsg->strParam, false);
      slideShow->Reset();
      slideShow->Add(&item);
      slideShow->Select(pMsg->strParam);
    }
  }
  break;

  case TMSG_PICTURE_SLIDESHOW:
  {
    CGUIWindowSlideShow *slideShow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);

    if (appPlayer->IsPlayingVideo())
      StopPlaying();

    slideShow->Reset();

    CFileItemList items;
    std::string strPath = pMsg->strParam;
    std::string extensions = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pictureExtensions;
    if (pMsg->param1)
      extensions += "|.tbn";
    CUtil::GetRecursiveListing(strPath, items, extensions);

    if (items.Size() > 0)
    {
      for (int i = 0; i<items.Size(); ++i)
        slideShow->Add(items[i].get());
      slideShow->StartSlideShow(true); //Start the slideshow!
    }

    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_SLIDESHOW)
    {
      if (items.Size() == 0)
      {
        CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_SCREENSAVER_MODE, "screensaver.xbmc.builtin.dim");
        GetComponent<CApplicationPowerHandling>()->ActivateScreenSaver();
      }
      else
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SLIDESHOW);
    }

  }
  break;

  case TMSG_LOADPROFILE:
    {
      const int profile = pMsg->param1;
      if (profile >= 0)
        CServiceBroker::GetSettingsComponent()->GetProfileManager()->LoadProfile(static_cast<unsigned int>(profile));
    }
    break;

  default:
    CLog::Log(LOGERROR, "%s: Unhandled threadmessage sent, %" PRIu32 , __FUNCTION__, msg);
    break;
  }
}

void CApplication::LockFrameMoveGuard()
{
  m_frameMoveGuard.lock();
  CServiceBroker::GetWinSystem()->GetGfxContext().lock();
};

void CApplication::UnlockFrameMoveGuard()
{
  CServiceBroker::GetWinSystem()->GetGfxContext().unlock();
  m_frameMoveGuard.unlock();
};

void CApplication::FrameMove(bool processEvents, bool processGUI)
{
  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  if (processEvents)
  {
    // currently we calculate the repeat time (ie time from last similar keypress) just global as fps
    float frameTime = m_frameTime.GetElapsedSeconds();
    m_frameTime.StartZero();
    // never set a frametime less than 2 fps to avoid problems when debugging and on breaks
    if (frameTime > 0.5f)
      frameTime = 0.5f;

    if (processGUI)
    {
      CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
      // check if there are notifications to display
      CGUIDialogKaiToast *toast = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogKaiToast>(WINDOW_DIALOG_KAI_TOAST);
      if (toast && toast->DoWork())
      {
        if (!toast->IsDialogRunning())
        {
          toast->Open();
        }
      }
    }

    GetComponent<CApplicationXbox>()->UpdateLCD();

    CServiceBroker::GetInputManager().Process(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog(), frameTime);

    if (processGUI)
    {
      appPlayer->GetSeekHandler().FrameMove();
    }
  }

  if (processGUI)
  {
    if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiSmartRedraw && m_guiRefreshTimer.IsTimePast())
    {
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_REFRESH_TIMER, 0, 0);
      m_guiRefreshTimer.Set(500);
    }

    if (!m_bStop)
    {
      CServiceBroker::GetGUI()->GetWindowManager().Process(CTimeUtils::GetFrameTime());
    }
    CServiceBroker::GetGUI()->GetWindowManager().FrameMove();
  }

  // this will go away when render systems gets its own thread
  CServiceBroker::GetWinSystem()->DriveRenderLoop();
}

void CApplication::ResetCurrentItem()
{
  m_itemCurrentFile->Reset();
  if (m_pGUI)
    m_pGUI->GetInfoManager().ResetCurrentItem();
}

int CApplication::Run()
{
  CLog::Log(LOGINFO, "Running the application...");

  // Run the app
  while (!m_bStop)
  {
    // Animate and render a frame

    Process();

    if (!m_bStop)
    {
      FrameMove(true);
    }

    if (!m_bStop)
    {
      Render();
    }
  }

  Cleanup();

  CLog::Log(LOGINFO, "Exiting the application...");
  return m_ExitCode;
}

bool CApplication::Cleanup()
{
  try
  {
    ResetCurrentItem();
    StopPlaying();

    if (m_ServiceManager)
      m_ServiceManager->DeinitStageThree();

    CLog::Log(LOGINFO, "unload skin");
    GetComponent<CApplicationSkinHandling>()->UnloadSkin();

    CServiceBroker::UnregisterTextureCache();

    // stop all remaining scripts; must be done after skin has been unloaded,
    // not before some windows still need it when deinitializing during skin
    // unloading
    CScriptInvocationManager::GetInstance().Uninitialize();

    CRenderSystemBase *renderSystem = CServiceBroker::GetRenderSystem();
    if (renderSystem)
      renderSystem->DestroyRenderSystem();

    CWinSystemBase *winSystem = CServiceBroker::GetWinSystem();
    if (winSystem)
      winSystem->DestroyWindow();

    if (m_pGUI)
      m_pGUI->GetWindowManager().DestroyWindows();

    CLog::Log(LOGINFO, "unload sections");

    //  Shutdown as much as possible of the
    //  application, to reduce the leaks dumped
    //  to the vc output window before calling
    //  _CrtDumpMemoryLeaks(). Most of the leaks
    //  shown are no real leaks, as parts of the app
    //  are still allocated.

    g_localizeStrings.Clear();
    g_LangCodeExpander.Clear();
    g_charsetConverter.clear();
    g_directoryCache.Clear();
    //CServiceBroker::GetInputManager().ClearKeymaps(); //! @todo
    CServiceBroker::GetPlaylistPlayer().Clear();

    if (m_ServiceManager)
      m_ServiceManager->DeinitStageTwo();

#ifdef HAS_OPTICAL_DRIVE
    CLibcdio::ReleaseInstance();
#endif
#ifdef _CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
    while(1); // execution ends
#endif

    if (m_pGUI)
    {
      m_pGUI->Deinit();
      m_pGUI.reset();
    }

    if (winSystem)
    {
      winSystem->DestroyWindowSystem();
      CServiceBroker::UnregisterWinSystem();
      winSystem = NULL;
      m_pWinSystem.reset();
    }

    // Cleanup was called more than once on exit during my tests
    if (m_ServiceManager)
    {
      m_ServiceManager->DeinitStageOne();
      m_ServiceManager.reset();
    }

    CServiceBroker::UnregisterKeyboardLayoutManager();

    CServiceBroker::UnregisterAppMessenger();

    CServiceBroker::UnregisterAnnouncementManager();
    m_pAnnouncementManager->Deinitialize();
    m_pAnnouncementManager.reset();

    CServiceBroker::UnregisterJobManager();

    UnregisterSettings();

    m_bInitializing = true;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Cleanup()");
    return false;
  }
}

bool CApplication::Stop(int exitCode)
{
  CLog::Log(LOGINFO, "Stopping the application...");

  bool success = true;

  CLog::Log(LOGINFO, "Stopping player");
  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  appPlayer->ClosePlayer();

  try
  {
    m_frameMoveGuard.unlock();

    CVariant vExitCode(CVariant::VariantTypeObject);
    vExitCode["exitcode"] = exitCode;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::System, "xbmc", "OnQuit", vExitCode);

    // Abort any active screensaver
    GetComponent<CApplicationPowerHandling>()->WakeUpScreenSaverAndDPMS();

    g_alarmClock.StopThread();

    CLog::Log(LOGINFO, "Storing total System Uptime");
    g_sysinfo.SetTotalUptime(g_sysinfo.GetTotalUptime() + (int)(CTimeUtils::GetFrameTime() / 60000));

    // Update the settings information (volume, uptime etc. need saving)
    if (CFile::Exists(CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetSettingsFile()))
    {
      CLog::Log(LOGINFO, "Saving settings");
      CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
    }
    else
      CLog::Log(LOGINFO, "Not saving settings (settings.xml is not present)");

    // kodi may crash or deadlock during exit (shutdown / reboot) due to
    // either a bug in core or misbehaving addons. so try saving
    // skin settings early
    CLog::Log(LOGINFO, "Saving skin settings");
    if (g_SkinInfo != NULL)
      g_SkinInfo->SaveSettings();

    m_bStop = true;
    // Add this here to keep the same ordering behaviour for now
    // Needs cleaning up
    CServiceBroker::GetAppMessenger()->Stop();
    m_ExitCode = exitCode;
    CLog::Log(LOGINFO, "Stopping all");

    // cancel any jobs from the jobmanager
    CServiceBroker::GetJobManager()->CancelJobs();

    // stop scanning before we kill the network and so on
    if (CMusicLibraryQueue::GetInstance().IsRunning())
      CMusicLibraryQueue::GetInstance().CancelAllJobs();

    if (CVideoLibraryQueue::GetInstance().IsRunning())
      CVideoLibraryQueue::GetInstance().CancelAllJobs();

    CServiceBroker::GetAppMessenger()->Cleanup();

    m_ServiceManager->GetNetwork().NetworkMessage(CNetwork::SERVICES_DOWN, 0);

    // Stop services before unloading Python
    CServiceBroker::GetServiceAddons().Stop();

    // Stop any other python scripts that may be looping waiting for monitor.abortRequested()
    CScriptInvocationManager::GetInstance().StopRunningScripts();

    // unregister action listeners
    const boost::shared_ptr<CApplicationActionListeners> appListener = GetComponent<CApplicationActionListeners>();
    appListener->UnregisterActionListener(&GetComponent<CApplicationPlayer>()->GetSeekHandler());
    appListener->UnregisterActionListener(&CPlayerController::GetInstance());

    CGUIComponent *gui = CServiceBroker::GetGUI();
    if (gui)
      gui->GetAudioManager().DeInitialize(1);

    GetComponent<CApplicationXbox>()->StopServices();

    CLog::Log(LOGINFO, "Application stopped");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Stop()");
    success = false;
  }

  cleanup_emu_environ();

  Sleep(200);

  return success;
}

namespace
{
class CCreateAndLoadPlayList : public IRunnable
{
public:
  CCreateAndLoadPlayList(CFileItem& item, boost::movelib::unique_ptr<PLAYLIST::CPlayList>& playlist)
    : m_item(item), m_playlist(playlist)
  {
  }

  virtual void Run()
  {
    const boost::movelib::unique_ptr<PLAYLIST::CPlayList> playlist(PLAYLIST::CPlayListFactory::Create(m_item));
    if (playlist)
    {
      if (playlist->Load(m_item.GetPath()))
        *m_playlist = *playlist;
    }
  }

private:
  CFileItem& m_item;
  boost::movelib::unique_ptr<PLAYLIST::CPlayList>& m_playlist;
};
} // namespace

bool CApplication::PlayMedia(CFileItem& item, const std::string& player, PLAYLIST::Id playlistId)
{
  // if the item is a plugin we need to resolve the plugin paths
  if (URIUtils::HasPluginPath(item) && !XFILE::CPluginDirectory::GetResolvedPluginResult(item))
    return false;

  if (item.IsSmartPlayList())
  {
    CFileItemList items;
    CUtil::GetRecursiveListing(item.GetPath(), items, "", DIR_FLAG_NO_FILE_DIRS);
    if (items.Size())
    {
      CSmartPlaylist smartpl;
      //get name and type of smartplaylist, this will always succeed as GetDirectory also did this.
      smartpl.OpenAndReadName(item.GetURL());
      PLAYLIST::CPlayList playlist;
      playlist.Add(items);
      PLAYLIST::Id smartplPlaylistId = PLAYLIST::TYPE_VIDEO;

      if (smartpl.GetType() == "songs" || smartpl.GetType() == "albums" ||
          smartpl.GetType() == "artists")
        smartplPlaylistId = PLAYLIST::TYPE_MUSIC;

      return ProcessAndStartPlaylist(smartpl.GetName(), playlist, smartplPlaylistId);
    }
  }
  else if (item.IsPlayList() || item.IsInternetStream())
  {
    // Not owner. Dialog auto-deletes itself.
    CGUIDialogCache* dlgCache =
        new CGUIDialogCache(5000, g_localizeStrings.Get(10214), item.GetLabel());

    //is or could be a playlist
    boost::movelib::unique_ptr<PLAYLIST::CPlayList> playlist;
    CCreateAndLoadPlayList getPlaylist(item, playlist);
    bool cancelled = !CGUIDialogBusy::Wait(&getPlaylist, 100, true);

    if (dlgCache)
    {
      dlgCache->Close();
      if (dlgCache->IsCanceled())
        cancelled = true;
    }

    if (cancelled)
      return true;

    if (playlist)
    {

      if (playlistId != PLAYLIST::TYPE_NONE)
      {
        int track=0;
        if (item.HasProperty("playlist_starting_track"))
          track = (int)item.GetProperty("playlist_starting_track").asInteger();
        return ProcessAndStartPlaylist(item.GetPath(), *playlist, playlistId, track);
      }
      else
      {
        CLog::Log(LOGWARNING,
                  "CApplication::PlayMedia called to play a playlist %s but no idea which playlist "
                  "to use, playing first item",
                  item.GetPath().c_str());
        if (playlist->size())
          return PlayFile(*(*playlist)[0], "", false);
      }
    }
  }

  //nothing special just play
  return PlayFile(item, player, false);
}

// PlayStack()
// For playing a multi-file video.  Particularly inefficient
// on startup, as we are required to calculate the length
// of each video, so we open + close each one in turn.
// A faster calculation of video time would improve this
// substantially.
// return value: same with PlayFile()
bool CApplication::PlayStack(CFileItem& item, bool bRestart)
{
  const boost::shared_ptr<CApplicationStackHelper> stackHelper = GetComponent<CApplicationStackHelper>();
  if (!stackHelper->InitializeStack(item))
    return false;

  boost::optional<int> startoffset = stackHelper->InitializeStackStartPartAndOffset(item);
  if (!startoffset)
  {
    CLog::Log(LOGERROR, "Failed to obtain start offset for stack %s. Aborting playback.",
               item.GetDynPath().c_str());
    return false;
  }

  CFileItem selectedStackPart = stackHelper->GetCurrentStackPartFileItem();
  selectedStackPart.SetStartOffset(startoffset.value());

  if (item.HasProperty("savedplayerstate"))
  {
    selectedStackPart.SetProperty("savedplayerstate", item.GetProperty("savedplayerstate")); // pass on to part
    item.ClearProperty("savedplayerstate");
  }

  return PlayFile(selectedStackPart, "", true);
}

bool CApplication::PlayFile(CFileItem item, const std::string& player, bool bRestart)
{
  // Ensure the MIME type has been retrieved for http:// and shout:// streams
  if (item.GetMimeType().empty())
    item.FillInMimeType();

  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  const boost::shared_ptr<CApplicationStackHelper> stackHelper = GetComponent<CApplicationStackHelper>();

  if (!bRestart)
  {
    // bRestart will be true when called from PlayStack(), skipping this block
    appPlayer->SetPlaySpeed(1);

    m_nextPlaylistItem = -1;
    stackHelper->Clear();

    if (item.IsVideo())
      CUtil::ClearSubtitles();
  }

  if (item.IsDiscStub())
  {
    // TODO: add GUIDialogPlayEject and CMediaManager::playStubFile
    return false;
  }

  if (item.IsPlayList())
    return false;

  // Translate/Resolve the url if needed
  const boost::movelib::unique_ptr<IDirectory> dir(CFactoryDirectory::Create(item));
  if (dir && !dir->Resolve(item))
  {
    return false;
  }

  // if we have a stacked set of files, we need to setup our stack routines for
  // "seamless" seeking and total time of the movie etc.
  // will recall with restart set to true
  if (item.IsStack())
    return PlayStack(item, bRestart);

  CPlayerOptions options;

  if (item.HasProperty("StartPercent"))
  {
    options.startpercent = item.GetProperty("StartPercent").asDouble();
    item.SetStartOffset(0);
  }

  options.starttime = CUtil::ConvertMilliSecsToSecs(item.GetStartOffset());

  if (bRestart)
  {
    // have to be set here due to playstack using this for starting the file
    if (item.HasVideoInfoTag())
      options.state = item.GetVideoInfoTag()->GetResumePoint().playerState;
  }
  if (!bRestart || stackHelper->IsPlayingISOStack())
  {
    // the following code block is only applicable when bRestart is false OR to ISO stacks

    if (item.IsVideo())
    {
      // open the d/b and retrieve the bookmarks for the current movie
      CVideoDatabase dbs;
      dbs.Open();

      std::string path = item.GetPath();
      std::string videoInfoTagPath(item.GetVideoInfoTag()->m_strFileNameAndPath);
      if (videoInfoTagPath.find("removable://") == 0 || item.IsVideoDb())
        path = videoInfoTagPath;

      // Note that we need to load the tag from database also if the item already has a tag,
      // because for example the (full) video info for strm files will be loaded here.
      dbs.LoadVideoInfo(path, *item.GetVideoInfoTag());

      if (item.HasProperty("savedplayerstate"))
      {
        options.starttime = CUtil::ConvertMilliSecsToSecs(item.GetStartOffset());
        options.state = item.GetProperty("savedplayerstate").asString();
        item.ClearProperty("savedplayerstate");
      }
      else if (item.GetStartOffset() == STARTOFFSET_RESUME)
      {
        options.starttime = 0.0;
        if (item.IsResumePointSet())
        {
          options.starttime = item.GetCurrentResumeTime();
          if (item.HasVideoInfoTag())
            options.state = item.GetVideoInfoTag()->GetResumePoint().playerState;
        }
        else
        {
          CBookmark bookmark;
          std::string path = item.GetPath();
          if (item.HasVideoInfoTag() && StringUtils::StartsWith(item.GetVideoInfoTag()->m_strFileNameAndPath, "removable://"))
            path = item.GetVideoInfoTag()->m_strFileNameAndPath;
          else if (item.HasProperty("original_listitem_url") && URIUtils::IsPlugin(item.GetProperty("original_listitem_url").asString()))
            path = item.GetProperty("original_listitem_url").asString();
          if (dbs.GetResumeBookMark(path, bookmark))
          {
            options.starttime = bookmark.timeInSeconds;
            options.state = bookmark.playerState;
          }
        }

        if (options.starttime == 0.0 && item.HasVideoInfoTag())
        {
          // No resume point is set, but check if this item is part of a multi-episode file
          const CVideoInfoTag *tag = item.GetVideoInfoTag();

          if (tag->m_iBookmarkId > 0)
          {
            CBookmark bookmark;
            dbs.GetBookMarkForEpisode(*tag, bookmark);
            options.starttime = bookmark.timeInSeconds;
            options.state = bookmark.playerState;
          }
        }
      }
      else if (item.HasVideoInfoTag())
      {
        const CVideoInfoTag *tag = item.GetVideoInfoTag();

        if (tag->m_iBookmarkId > 0)
        {
          CBookmark bookmark;
          dbs.GetBookMarkForEpisode(*tag, bookmark);
          options.starttime = bookmark.timeInSeconds;
          options.state = bookmark.playerState;
        }
      }

      dbs.Close();
    }
  }

  // this really aught to be inside !bRestart, but since PlayStack
  // uses that to init playback, we have to keep it outside
  const PLAYLIST::Id playlistId = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  if (item.IsAudio() && playlistId == PLAYLIST::TYPE_MUSIC)
  { // playing from a playlist by the looks
    // don't switch to fullscreen if we are not playing the first item...
    options.fullscreen = !CServiceBroker::GetPlaylistPlayer().HasPlayedFirstFile() &&
        CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICFILES_SELECTACTION) &&
        !CMediaSettings::GetInstance().DoesMediaStartWindowed();
  }
  else if (item.IsVideo() && playlistId == PLAYLIST::TYPE_VIDEO &&
           CServiceBroker::GetPlaylistPlayer().GetPlaylist(playlistId).size() > 1)
  { // playing from a playlist by the looks
    // don't switch to fullscreen if we are not playing the first item...
    options.fullscreen = !CServiceBroker::GetPlaylistPlayer().HasPlayedFirstFile() &&
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreenOnMovieStart &&
        !CMediaSettings::GetInstance().DoesMediaStartWindowed();
  }
  else if (stackHelper->IsPlayingRegularStack())
  {
    //! @todo - this will fail if user seeks back to first file in stack
    if (stackHelper->GetCurrentPartNumber() == 0 ||
        stackHelper->GetRegisteredStack(item)->GetStartOffset() != 0)
      options.fullscreen = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->
          m_fullScreenOnMovieStart && !CMediaSettings::GetInstance().DoesMediaStartWindowed();
    else
      options.fullscreen = false;
  }
  else
    options.fullscreen = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->
        m_fullScreenOnMovieStart && !CMediaSettings::GetInstance().DoesMediaStartWindowed();

  // reset VideoStartWindowed as it's a temp setting
  CMediaSettings::GetInstance().SetMediaStartWindowed(false);

  // We have to stop parsing a cdg before mplayer is deallocated. WHY???
  GetComponent<CApplicationXbox>()->StopKaraoke();

  {
    // for playing a new item, previous playing item's callback may already
    // pushed some delay message into the threadmessage list, they are not
    // expected be processed after or during the new item playback starting.
    // so we clean up previous playing item's playback callback delay messages here.
    int previousMsgsIgnoredByNewPlaying[] = {
      GUI_MSG_PLAYBACK_STARTED,
      GUI_MSG_PLAYBACK_ENDED,
      GUI_MSG_PLAYBACK_STOPPED,
      GUI_MSG_PLAYLIST_CHANGED,
      GUI_MSG_PLAYLISTPLAYER_STOPPED,
      GUI_MSG_PLAYLISTPLAYER_STARTED,
      GUI_MSG_PLAYLISTPLAYER_CHANGED,
      GUI_MSG_QUEUE_NEXT_ITEM,
      0
    };
    int dMsgCount = CServiceBroker::GetGUI()->GetWindowManager().RemoveThreadMessageByMessageIds(&previousMsgsIgnoredByNewPlaying[0]);
    if (dMsgCount > 0)
      CLog::Log(LOGDEBUG, "Ignored %i playback thread messages", dMsgCount);
  }

  const boost::shared_ptr<CApplicationVolumeHandling> appVolume = GetComponent<CApplicationVolumeHandling>();
  appPlayer->OpenFile(item, options, m_ServiceManager->GetPlayerCoreFactory(), player, *this);
  appPlayer->SetVolume(appVolume->GetVolumeRatio());
  appPlayer->SetMute(appVolume->IsMuted());

  CGUIComponent *gui = CServiceBroker::GetGUI();
  if (gui)
    gui->GetAudioManager().Enable(false);

  return true;
}

void CApplication::PlaybackCleanup()
{
  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  const boost::shared_ptr<CApplicationStackHelper> stackHelper = GetComponent<CApplicationStackHelper>();

  if (!appPlayer->IsPlaying())
  {
    CGUIComponent *gui = CServiceBroker::GetGUI();
    if (gui)
      CServiceBroker::GetGUI()->GetAudioManager().Enable(true);

    GetComponent<CApplicationXbox>()->StartLEDControl(false);
    GetComponent<CApplicationXbox>()->DimLCDOnPlayback(false);
    GetComponent<CApplicationXbox>()->StopKaraoke();

    appPlayer->OpenNext(m_ServiceManager->GetPlayerCoreFactory());
  }

  if (!appPlayer->IsPlayingVideo())
  {
    if(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
       CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME)
    {
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
    }
    else
    {
      //  resets to res_desktop or look&feel resolution (including refreshrate)
      CServiceBroker::GetWinSystem()->GetGfxContext().SetFullScreenVideo(false);
    }
  }

  const boost::shared_ptr<CApplicationPowerHandling> appPower = GetComponent<CApplicationPowerHandling>();

  if (!appPlayer->IsPlayingAudio() &&
      CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::TYPE_NONE &&
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION)
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();  // save vis settings
    appPower->WakeUpScreenSaverAndDPMS();
    CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
  }

  // DVD ejected while playing in vis ?
  if (!appPlayer->IsPlayingAudio() &&
      (m_itemCurrentFile->IsCDDA() || m_itemCurrentFile->IsOnDVD()) &&
      !CDetectDVDMedia::IsDiscInDrive() &&
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION)
  {
    // yes, disable vis
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();    // save vis settings
    appPower->WakeUpScreenSaverAndDPMS();
    CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
  }

  if (!appPlayer->IsPlaying())
  {
    stackHelper->Clear();
    appPlayer->ResetPlayer();
  }
}

bool CApplication::IsPlayingFullScreenVideo() const
{
  const boost::shared_ptr<const CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  return appPlayer->IsPlayingVideo() &&
         CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenVideo();
}

bool CApplication::IsFullScreen()
{
  return IsPlayingFullScreenVideo() ||
        (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION) ||
         CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW;
}

void CApplication::StopPlaying()
{
  CGUIComponent *gui = CServiceBroker::GetGUI();

  if (gui)
  {
    int iWin = gui->GetWindowManager().GetActiveWindow();
    const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
    if (appPlayer->IsPlaying())
    {
      GetComponent<CApplicationXbox>()->StopKaraoke();

      appPlayer->ClosePlayer();

      // turn off visualisation window when stopping
      if ((iWin == WINDOW_VISUALISATION ||
           iWin == WINDOW_FULLSCREEN_VIDEO ||
           iWin == WINDOW_FULLSCREEN_GAME) &&
           !m_bStop)
        gui->GetWindowManager().PreviousWindow();

      g_partyModeManager.Disable();
    }
  }
}

bool CApplication::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1()==GUI_MSG_REMOVED_MEDIA)
      {
        // Update general playlist: Remove DVD playlist items
        int nRemoved = CServiceBroker::GetPlaylistPlayer().RemoveDVDItems();
        if ( nRemoved > 0 )
        {
          CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0 );
          CServiceBroker::GetGUI()->GetWindowManager().SendMessage( msg );
        }
        // stop the file if it's on dvd (will set the resume point etc)
        if (m_itemCurrentFile->IsOnDVD())
          StopPlaying();
      }
      else if (message.GetParam1() == GUI_MSG_UI_READY)
      {
        // remove splash window
        CServiceBroker::GetGUI()->GetWindowManager().Delete(WINDOW_SPLASH);

        // show the volumebar if the volume is muted
        const boost::shared_ptr<CApplicationVolumeHandling> appVolume = GetComponent<CApplicationVolumeHandling>();
        if (appVolume->IsMuted() ||
            appVolume->GetVolumeRatio() <= CApplicationVolumeHandling::VOLUME_MINIMUM)
          appVolume->ShowVolumeBar();

        if (!m_incompatibleAddons.empty())
        {
          // filter addons that are not dependencies
          std::vector<std::string> disabledAddonNames;
          for (std::vector<boost::shared_ptr<ADDON::CAddonInfo> >::const_iterator addoninfo = m_incompatibleAddons.begin(); addoninfo != m_incompatibleAddons.end(); ++addoninfo)
          {
            if (!CAddonType::IsDependencyType((*addoninfo)->MainType()))
              disabledAddonNames.push_back((*addoninfo)->Name());
          }

          // migration (incompatible addons) dialog
          std::string addonList = StringUtils::Join(disabledAddonNames, ", ");
          std::string msg = StringUtils::Format(g_localizeStrings.Get(24149).c_str(), addonList.c_str());
          HELPERS::ShowOKDialogText(24148, boost::move(msg));
          m_incompatibleAddons.clear();
        }

        // offer enabling addons at kodi startup that are disabled due to
        // e.g. os package manager installation on linux
        ConfigureAndEnableAddons();

        m_bInitializing = false;

        if (message.GetSenderId() == WINDOW_SETTINGS_PROFILES)
          GetComponent<CApplicationSkinHandling>()->ReloadSkin(false);
      }
      else if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
      {
        CFileItemPtr item = boost::static_pointer_cast<CFileItem>(message.GetItem());
        if (m_itemCurrentFile->IsSamePath(item.get()))
        {
          m_itemCurrentFile->UpdateInfo(*item);
          CServiceBroker::GetGUI()->GetInfoManager().UpdateCurrentItem(*item);
        }
      }
    }
    break;

  case GUI_MSG_PLAYBACK_STARTED:
    {
      m_itemCurrentFile =
          boost::make_shared<CFileItem>(*boost::static_pointer_cast<CFileItem>(message.GetItem()));

      PLAYLIST::CPlayList playList = CServiceBroker::GetPlaylistPlayer().GetPlaylist(
          CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());

      // Update our infoManager with the new details etc.
      if (m_nextPlaylistItem >= 0)
      {
        // playing an item which is not in the list - player might be stopped already
        // so do nothing
        if (playList.size() <= m_nextPlaylistItem)
          return true;

        // we've started a previously queued item
        CFileItemPtr item = playList[m_nextPlaylistItem];
        // update the playlist manager
        int currentSong = CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx();
        int param = ((currentSong & 0xffff) << 16) | (m_nextPlaylistItem & 0xffff);
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0, CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist(), param, item);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
        CServiceBroker::GetPlaylistPlayer().SetCurrentItemIdx(m_nextPlaylistItem);
        m_itemCurrentFile = boost::make_shared<CFileItem>(*item);
      }
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_itemCurrentFile);
      g_partyModeManager.OnSongChange(true);

      GetComponent<CApplicationXbox>()->CheckNetworkHDSpinDown(true);
      GetComponent<CApplicationXbox>()->StartLEDControl(true);
      GetComponent<CApplicationXbox>()->DimLCDOnPlayback(true);

      // Start our Karaoke parser
      GetComponent<CApplicationXbox>()->StartKaraoke(m_itemCurrentFile);

#ifdef HAS_PYTHON
      // informs python script currently running playback has started
      // (does nothing if python is not loaded)
      CServiceBroker::GetXBPython().OnPlayBackStarted(*m_itemCurrentFile);
#endif

      CVariant param;
      param["player"]["speed"] = 1;
      param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();

      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnPlay",
                                                         m_itemCurrentFile, param);

      return true;
    }
    break;

  case GUI_MSG_QUEUE_NEXT_ITEM:
    {
      // Check to see if our playlist player has a new item for us,
      // and if so, we check whether our current player wants the file
      int iNext = CServiceBroker::GetPlaylistPlayer().GetNextItemIdx();
      PLAYLIST::CPlayList& playlist = CServiceBroker::GetPlaylistPlayer().GetPlaylist(
          CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());
      if (iNext < 0 || iNext >= playlist.size())
      {
        GetComponent<CApplicationPlayer>()->OnNothingToQueueNotify();
        return true; // nothing to do
      }

      // ok, grab the next song
      CFileItem file(*playlist[iNext]);
      // handle plugin://
      CURL url(file.GetDynPath());
      if (url.IsProtocol("plugin"))
        XFILE::CPluginDirectory::GetPluginResult(url.Get(), file, false);

      // Don't queue if next media type is different from current one
      bool bNothingToQueue = false;

      const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
      if (!file.IsVideo() && appPlayer->IsPlayingVideo())
        bNothingToQueue = true;
      else if ((!file.IsAudio() || file.IsVideo()) && appPlayer->IsPlayingAudio())
        bNothingToQueue = true;

      if (bNothingToQueue)
      {
        appPlayer->OnNothingToQueueNotify();
        return true;
      }

#ifdef HAS_UPNP
      if (URIUtils::IsUPnP(file.GetDynPath()))
      {
        if (!XFILE::CUPnPDirectory::GetResource(file.GetDynURL(), file))
          return true;
      }
#endif

      // ok - send the file to the player, if it accepts it
      if (appPlayer->QueueNextFile(file))
      {
        // player accepted the next file
        m_nextPlaylistItem = iNext;
      }
      else
      {
        /* Player didn't accept next file: *ALWAYS* advance playlist in this case so the player can
            queue the next (if it wants to) and it doesn't keep looping on this song */
        CServiceBroker::GetPlaylistPlayer().SetCurrentItemIdx(iNext);
      }

      return true;
    }
    break;

    case GUI_MSG_PLAY_TRAILER:
    {
      const CFileItem* item = dynamic_cast<CFileItem*>(message.GetItem().get());
      if (item == NULL)
      {
        CLog::Log(LOGERROR, "Supplied item is not a CFileItem! Trailer cannot be played.");
        return false;
      }

      boost::movelib::unique_ptr<CFileItem> trailerItem =
          ContentUtils::GeneratePlayableTrailerItem(*item, g_localizeStrings.Get(20410));

      if (item->IsPlayList())
      {
        boost::movelib::unique_ptr<CFileItemList> fileitemList = boost::movelib::make_unique<CFileItemList>();
        fileitemList->Add(boost::shared_ptr<CFileItem>(trailerItem.release()));
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, -1, -1,
                                                   static_cast<void*>(fileitemList.release()));
      }
      else
      {
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 1, 0,
                                                   static_cast<void*>(trailerItem.release()));
      }
      break;
    }

  case GUI_MSG_PLAYBACK_STOPPED:
  {
    CVariant data(CVariant::VariantTypeObject);
    data["end"] = false;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnStop",
                                                       m_itemCurrentFile, data);

    ResetCurrentItem();
    PlaybackCleanup();
#ifdef HAS_PYTHON
    CServiceBroker::GetXBPython().OnPlayBackStopped();
#endif
     return true;
  }

  case GUI_MSG_PLAYBACK_ENDED:
  {
    CVariant data(CVariant::VariantTypeObject);
    data["end"] = true;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnStop",
                                                       m_itemCurrentFile, data);

    const boost::shared_ptr<CApplicationStackHelper> stackHelper = GetComponent<CApplicationStackHelper>();
    if (stackHelper->IsPlayingRegularStack() && stackHelper->HasNextStackPartFileItem())
    { // just play the next item in the stack
      PlayFile(stackHelper->SetNextStackPartCurrentFileItem(), "", true);
      return true;
    }

    ResetCurrentItem();

    if (!CServiceBroker::GetPlaylistPlayer().PlayNext(1, true))
      GetComponent<CApplicationPlayer>()->ClosePlayer();

    PlaybackCleanup();

#ifdef HAS_PYTHON
    CServiceBroker::GetXBPython().OnPlayBackEnded();
#endif
    return true;
  }

  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    ResetCurrentItem();
    if (GetComponent<CApplicationPlayer>()->IsPlaying())
      StopPlaying();
    PlaybackCleanup();
    return true;

  case GUI_MSG_PLAYBACK_PAUSED:
  {
    CVariant param;
    param["player"]["speed"] = 0;
    param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnPause",
                                                       m_itemCurrentFile, param);
    return true;
  }

  case GUI_MSG_PLAYBACK_RESUMED:
  {
    CVariant param;
    param["player"]["speed"] = 1;
    param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnResume",
                                                       m_itemCurrentFile, param);
    return true;
  }

  case GUI_MSG_PLAYBACK_SEEKED:
  {
    CVariant param;
    const int64_t iTime = message.GetParam1AsI64();
    const int64_t seekOffset = message.GetParam2AsI64();
    param["player"]["time"]["hours"] = iTime;
    param["player"]["seekoffset"]["hours"] = seekOffset;
    param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    const CApplicationComponents &components = CServiceBroker::GetAppComponents();
    const boost::shared_ptr<const CApplicationPlayer> appPlayer = components.GetComponent<CApplicationPlayer>();
    param["player"]["speed"] = static_cast<int>(appPlayer->GetPlaySpeed());
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnSeek",
                                                       m_itemCurrentFile, param);

    return true;
  }

  case GUI_MSG_PLAYBACK_SPEED_CHANGED:
  {
    CVariant param;
    param["player"]["speed"] = message.GetParam1();
    param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "xbmc", "OnSpeedChanged",
                                                       m_itemCurrentFile, param);

    return true;
  }

  case GUI_MSG_PLAYBACK_ERROR:
    HELPERS::ShowOKDialogText(16026, 16027);
    return true;

  case GUI_MSG_PLAYLISTPLAYER_STARTED:
  case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    {
      return true;
    }
    break;
  case GUI_MSG_EXECUTE:
    if (message.GetNumStringParams())
      return ExecuteXBMCAction(message.GetStringParam(), message.GetItem());
    break;
  }
  return false;
}

bool CApplication::ExecuteXBMCAction(std::string actionStr,
                                     const boost::shared_ptr<CGUIListItem>& item /* = NULL */)
{
  // see if it is a user set string

  //We don't know if there is unsecure information in this yet, so we
  //postpone any logging
  const std::string in_actionStr(actionStr);
  if (item)
    actionStr = GUILIB::GUIINFO::CGUIInfoLabel::GetItemLabel(actionStr, item.get());
  else
    actionStr = GUILIB::GUIINFO::CGUIInfoLabel::GetLabel(actionStr, INFO::DEFAULT_CONTEXT);

  // user has asked for something to be executed
  if (CBuiltins::GetInstance().HasCommand(actionStr))
  {
    CBuiltins::GetInstance().Execute(actionStr);
  }
  else
  {
    // try translating the action from our ButtonTranslator
    int actionID;
    if (CButtonTranslator::TranslateActionString(actionStr.c_str(), actionID))
    {
      OnAction(CAction(actionID));
      return true;
    }
    CFileItem item(actionStr, false);
#ifdef HAS_PYTHON
    if (item.IsPythonScript())
    { // a python script
      CScriptInvocationManager::GetInstance().ExecuteAsync(item.GetPath());
    }
    else
#endif
    if (item.IsAudio() || item.IsVideo())
    { // an audio or video file
      PlayFile(item, "");
    }
    else
    {
      //At this point we have given up to translate, so even though
      //there may be insecure information, we log it.
      CLog::Log(LOGDEBUG, "Tried translating, but failed to understand %s", in_actionStr.c_str());
      return false;
    }
  }
  return true;
}

void CApplication::ConfigureAndEnableAddons()
{
  std::vector<boost::shared_ptr<IAddon> >
      disabledAddons; /*!< Installed addons, but not auto-enabled via manifest */

  ADDON::CAddonMgr &addonMgr = CServiceBroker::GetAddonMgr();

  if (addonMgr.GetDisabledAddons(disabledAddons) && !disabledAddons.empty())
  {
    // this applies to certain platforms only:
    // look at disabled addons with disabledReason == NONE, usually those are installed via package managers or manually.
    // also try to enable add-ons with disabledReason == INCOMPATIBLE at startup for all platforms.

    bool isConfigureAddonsAtStartupEnabled = true;

    for (std::vector<boost::shared_ptr<IAddon> >::const_iterator it = disabledAddons.begin(); it != disabledAddons.end(); ++it)
    {
      const ADDON::AddonPtr &addon = *it;
      if (addonMgr.IsAddonDisabledWithReason(addon->ID(), ADDON::AddonDisabledReason::INCOMPATIBLE))
      {
        ADDON::AddonInfoPtr addonInfo = addonMgr.GetAddonInfo(addon->ID(), AddonType::UNKNOWN);
        if (addonInfo && addonMgr.IsCompatible(addonInfo))
        {
          CLog::Log(LOGDEBUG, "CApplication::%s: enabling the compatible version of [%s].",
                    __FUNCTION__, addon->ID().c_str());
          addonMgr.EnableAddon(addon->ID());
        }
        continue;
      }

      if (addonMgr.IsAddonDisabledExcept(addon->ID(), ADDON::AddonDisabledReason::NONE) ||
          CAddonType::IsDependencyType(addon->MainType()))
      {
        continue;
      }

      if (isConfigureAddonsAtStartupEnabled)
      {
        if (HELPERS::ShowYesNoDialogLines(24039, // Disabled add-ons
                                          24059, // Would you like to enable this add-on?
                                          addon->Name()) == HELPERS::CHOICE_YES)
        {
          if (addon->CanHaveAddonOrInstanceSettings())
          {
            if (CGUIDialogAddonSettings::ShowForAddon(addon))
            {
              // only enable if settings dialog hasn't been cancelled
              addonMgr.EnableAddon(addon->ID());
            }
          }
          else
          {
            addonMgr.EnableAddon(addon->ID());
          }
        }
        else
        {
          // user chose not to configure/enable so we're not asking anymore
          addonMgr.UpdateDisabledReason(addon->ID(), ADDON::AddonDisabledReason::USER);
        }
      }
    }
  }
}

void CApplication::Process()
{
  // dispatch the messages generated by python or other threads to the current window
  CServiceBroker::GetGUI()->GetWindowManager().DispatchThreadMessages();

  // process messages which have to be send to the gui
  // (this can only be done after CServiceBroker::GetGUI()->GetWindowManager().Render())
  CServiceBroker::GetAppMessenger()->ProcessWindowMessages();

  // handle any active scripts

  {
    // Allow processing of script threads to let them shut down properly.
    CSingleExit ex(CServiceBroker::GetWinSystem()->GetGfxContext());
    m_frameMoveGuard.unlock();
    CScriptInvocationManager::GetInstance().Process();
    m_frameMoveGuard.lock();
  }

  // process messages, even if a movie is playing
  CServiceBroker::GetAppMessenger()->ProcessMessages();
  if (m_bStop) return; //we're done, everything has been unloaded

  // check for memory unit changes
  if (g_memoryUnitManager.Update())
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REMOVED_MEDIA);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }

  CServiceBroker::GetGUI()->GetAudioManager().FreeUnused();

  // update sound - removed in Kodi!
  // https://github.com/xbmc/xbmc/pull/24262
  GetComponent<CApplicationPlayer>()->DoAudioWork();

  // process karaoke
  GetComponent<CApplicationXbox>()->ProcessKaraoke();

  // do any processing that isn't needed on each run
  if( m_slowTimer.GetElapsedMilliseconds() > 500 )
  {
    m_slowTimer.Reset();
    ProcessSlow();
  }
}

// We get called every 500ms
void CApplication::ProcessSlow()
{
  // process skin resources (skin timers)
  GetComponent<CApplicationSkinHandling>()->ProcessSkin();

  // Temporarily pause pausable jobs when viewing video/picture
  int currentWindow = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (CurrentFileItem().IsVideo() ||
      CurrentFileItem().IsPicture() ||
      currentWindow == WINDOW_FULLSCREEN_VIDEO ||
      currentWindow == WINDOW_FULLSCREEN_GAME ||
      currentWindow == WINDOW_SLIDESHOW)
  {
    CServiceBroker::GetJobManager()->PauseJobs();
  }
  else
  {
    CServiceBroker::GetJobManager()->UnPauseJobs();
  }

  // Check if we need to activate the screensaver / DPMS.
  const boost::shared_ptr<CApplicationPowerHandling> appPower = GetComponent<CApplicationPowerHandling>();
  appPower->CheckScreenSaverAndDPMS();

  // Check if we need to shutdown (if enabled).
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME))
  {
    appPower->CheckShutdown();
  }

  // check if we should restart the player
  CheckDelayedPlayerRestart();

  // check if we can unload any unreferenced dlls or sections
  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  if (!appPlayer->IsPlayingVideo())
    CSectionLoader::UnloadDelayed();

  // check for any idle curl connections
  g_curlInterface.CheckIdle();

  // check for any needed SNTP update
  if (CNetworkServices::GetInstance().IsTimeServerRunning() && CNetworkServices::GetInstance().IsTimeServerUpdateNeeded())
    CNetworkServices::GetInstance().UpdateTimeServer();

  CServiceBroker::GetGUI()->GetLargeTextureManager().CleanupUnusedImages();

  CServiceBroker::GetGUI()->GetTextureManager().FreeUnusedTextures(5000);

#ifdef HAS_OPTICAL_DRIVE
  // checks whats in the DVD drive and tries to autostart the content (xbox games, dvd, cdda, avi files...)
  if (!appPlayer->IsPlayingVideo())
    m_Autorun->HandleAutorun();
#endif

  // update upnp server/renderer states
#ifdef HAS_UPNP
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SERVICES_UPNP) && UPNP::CUPnP::IsInstantiated())
    UPNP::CUPnP::GetInstance()->UpdateState();
#endif

  // if we don't render the gui there's no reason to start the screensaver.
  // that way the screensaver won't kick in if we maximize the XBMC window
  // after the screensaver start time.
  if (!appPower->GetRenderGUI())
    appPower->ResetScreenSaverTimer();
}

void CApplication::DelayedPlayerRestart()
{
  m_restartPlayerTimer.StartZero();
}

void CApplication::CheckDelayedPlayerRestart()
{
  if (m_restartPlayerTimer.GetElapsedSeconds() > 3)
  {
    m_restartPlayerTimer.Stop();
    m_restartPlayerTimer.Reset();
    Restart(true);
  }
}

void CApplication::Restart(bool bSamePosition)
{
  // this function gets called when the user changes a setting (like noninterleaved)
  // and which means we gotta close & reopen the current playing file

  // first check if we're playing a file
  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  if (!appPlayer->IsPlayingVideo() && !appPlayer->IsPlayingAudio())
    return ;

  if (!appPlayer->HasPlayer())
    return ;

  // do we want to return to the current position in the file
  if (!bSamePosition)
  {
    // no, then just reopen the file and start at the beginning
    PlayFile(*m_itemCurrentFile, "", true);
    return ;
  }

  // else get current position
  double time = GetTime();

  // get player state, needed for dvd's
  std::string state = appPlayer->GetPlayerState();

  // set the requested starttime
  m_itemCurrentFile->SetStartOffset(CUtil::ConvertSecsToMilliSecs(time));

  // reopen the file
  if (PlayFile(*m_itemCurrentFile, "", true))
    appPlayer->SetPlayerState(state);
}

const std::string& CApplication::CurrentFile()
{
  return m_itemCurrentFile->GetPath();
}

boost::shared_ptr<CFileItem> CApplication::CurrentFileItemPtr()
{
  return m_itemCurrentFile;
}

CFileItem& CApplication::CurrentFileItem()
{
  return *m_itemCurrentFile;
}

const CFileItem& CApplication::CurrentUnstackedItem()
{
  const boost::shared_ptr<CApplicationStackHelper> stackHelper = GetComponent<CApplicationStackHelper>();

  if (stackHelper->IsPlayingISOStack() || stackHelper->IsPlayingRegularStack())
    return stackHelper->GetCurrentStackPartFileItem();
  else
    return *m_itemCurrentFile;
}

// Returns the total time in seconds of the current media.  Fractional
// portions of a second are possible - but not necessarily supported by the
// player class.  This returns a double to be consistent with GetTime() and
// SeekTime().
double CApplication::GetTotalTime() const
{
  double rc = 0.0;

  const boost::shared_ptr<const CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  const boost::shared_ptr<const CApplicationStackHelper> stackHelper = GetComponent<CApplicationStackHelper>();

  if (appPlayer->IsPlaying())
  {
    if (stackHelper->IsPlayingRegularStack())
      rc = stackHelper->GetStackTotalTimeMs() * 0.001;
    else
      rc = appPlayer->GetTotalTime() * 0.001;
  }

  return rc;
}

// Returns the current time in seconds of the currently playing media.
// Fractional portions of a second are possible.  This returns a double to
// be consistent with GetTotalTime() and SeekTime().
double CApplication::GetTime() const
{
  double rc = 0.0;

  const boost::shared_ptr<const CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  const boost::shared_ptr<const CApplicationStackHelper> stackHelper = GetComponent<CApplicationStackHelper>();

  if (appPlayer->IsPlaying())
  {
    if (stackHelper->IsPlayingRegularStack())
    {
      uint64_t startOfCurrentFile = stackHelper->GetCurrentStackPartStartTimeMs();
      rc = (startOfCurrentFile + appPlayer->GetTime()) * 0.001;
    }
    else
      rc = appPlayer->GetTime() * 0.001;
  }

  return rc;
}

// Sets the current position of the currently playing media to the specified
// time in seconds.  Fractional portions of a second are valid.  The passed
// time is the time offset from the beginning of the file as opposed to a
// delta from the current position.  This method accepts a double to be
// consistent with GetTime() and GetTotalTime().
void CApplication::SeekTime( double dTime )
{
  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  const boost::shared_ptr<CApplicationStackHelper> stackHelper = GetComponent<CApplicationStackHelper>();

  if (appPlayer->IsPlaying() && (dTime >= 0.0))
  {
    if (!appPlayer->CanSeek())
      return;

    if (stackHelper->IsPlayingRegularStack())
    {
      // find the item in the stack we are seeking to, and load the new
      // file if necessary, and calculate the correct seek within the new
      // file.  Otherwise, just fall through to the usual routine if the
      // time is higher than our total time.
      int partNumberToPlay =
          stackHelper->GetStackPartNumberAtTimeMs(static_cast<uint64_t>(dTime * 1000.0));
      uint64_t startOfNewFile = stackHelper->GetStackPartStartTimeMs(partNumberToPlay);
      if (partNumberToPlay == stackHelper->GetCurrentPartNumber())
        appPlayer->SeekTime(static_cast<uint64_t>(dTime * 1000.0) - startOfNewFile);
      else
      { // seeking to a new file
        stackHelper->SetStackPartCurrentFileItem(partNumberToPlay);
        CFileItem* item = new CFileItem(stackHelper->GetCurrentStackPartFileItem());
        item->SetStartOffset(static_cast<uint64_t>(dTime * 1000.0) - startOfNewFile);
        // don't just call "PlayFile" here, as we are quite likely called from the
        // player thread, so we won't be able to delete ourselves.
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 1, 0, static_cast<void*>(item));
      }
      return;
    }
    // convert to milliseconds and perform seek
    appPlayer->SeekTime(static_cast<int64_t>(dTime * 1000.0));
  }
}

float CApplication::GetPercentage() const
{
  const boost::shared_ptr<const CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  const boost::shared_ptr<const CApplicationStackHelper> stackHelper = GetComponent<CApplicationStackHelper>();

  if (appPlayer->IsPlaying())
  {
    if (appPlayer->GetTotalTime() == 0 && appPlayer->IsPlayingAudio() &&
        m_itemCurrentFile->HasMusicInfoTag())
    {
      const CMusicInfoTag& tag = *m_itemCurrentFile->GetMusicInfoTag();
      if (tag.GetDuration() > 0)
        return (float)(GetTime() / tag.GetDuration() * 100);
    }

    if (stackHelper->IsPlayingRegularStack())
    {
      double totalTime = GetTotalTime();
      if (totalTime > 0.0)
        return (float)(GetTime() / totalTime * 100);
    }
    else
      return appPlayer->GetPercentage();
  }
  return 0.0f;
}

float CApplication::GetCachePercentage() const
{
  const boost::shared_ptr<const CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  const boost::shared_ptr<const CApplicationStackHelper> stackHelper = GetComponent<CApplicationStackHelper>();

  if (appPlayer->IsPlaying())
  {
    // Note that the player returns a relative cache percentage and we want an absolute percentage
    if (stackHelper->IsPlayingRegularStack())
    {
      float stackedTotalTime = (float) GetTotalTime();
      // We need to take into account the stack's total time vs. currently playing file's total time
      if (stackedTotalTime > 0.0f)
        return std::min(100.0f,
                        GetPercentage() + (appPlayer->GetCachePercentage() *
                                           appPlayer->GetTotalTime() * 0.001f / stackedTotalTime));
    }
    else
      return std::min(100.0f, appPlayer->GetPercentage() + appPlayer->GetCachePercentage());
  }
  return 0.0f;
}

void CApplication::SeekPercentage(float percent)
{
  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  const boost::shared_ptr<CApplicationStackHelper> stackHelper = GetComponent<CApplicationStackHelper>();

  if (appPlayer->IsPlaying() && (percent >= 0.0f))
  {
    if (!appPlayer->CanSeek())
      return;
    if (stackHelper->IsPlayingRegularStack())
      SeekTime(static_cast<double>(percent) * 0.01 * GetTotalTime());
    else
      appPlayer->SeekPercentage(percent);
  }
}

std::string CApplication::GetCurrentPlayer()
{
  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  return appPlayer->GetCurrentPlayer();
}

void CApplication::UpdateLibraries()
{
  const boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_UPDATEONSTARTUP))
  {
    CLog::Log(LOGINFO, "Starting video library startup scan");
    CVideoLibraryQueue::GetInstance().ScanLibrary(
        "", false, !settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_BACKGROUNDUPDATE));
  }

  if (settings->GetBool(CSettings::SETTING_MUSICLIBRARY_UPDATEONSTARTUP))
  {
    CLog::Log(LOGINFO, "Starting music library startup scan");
    CMusicLibraryQueue::GetInstance().ScanLibrary(
        "", MUSIC_INFO::CMusicInfoScanner::SCAN_NORMAL,
        !settings->GetBool(CSettings::SETTING_MUSICLIBRARY_BACKGROUNDUPDATE));
  }
}

void CApplication::UpdateCurrentPlayArt()
{
  const boost::shared_ptr<CApplicationPlayer> appPlayer = GetComponent<CApplicationPlayer>();
  if (!appPlayer->IsPlayingAudio())
    return;
  //Clear and reload the art for the currently playing item to show updated art on OSD
  m_itemCurrentFile->ClearArt();
  CMusicThumbLoader loader;
  loader.LoadItem(m_itemCurrentFile.get());
  // Mirror changes to GUI item
  CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_itemCurrentFile);
}

bool CApplication::ProcessAndStartPlaylist(const std::string& strPlayList,
                                           PLAYLIST::CPlayList& playlist,
                                           PLAYLIST::Id playlistId,
                                           int track)
{
  CLog::Log(LOGDEBUG, "CApplication::ProcessAndStartPlaylist(%s, %i)", strPlayList.c_str(), playlistId);

  // initial exit conditions
  // no songs in playlist just return
  if (playlist.size() == 0)
    return false;

  // illegal playlist
  if (playlistId == PLAYLIST::TYPE_NONE || playlistId == PLAYLIST::TYPE_PICTURE)
    return false;

  // setup correct playlist
  CServiceBroker::GetPlaylistPlayer().ClearPlaylist(playlistId);

  // if the playlist contains an internet stream, this file will be used
  // to generate a thumbnail for musicplayer.cover
  m_strPlayListFile = strPlayList;

  // add the items to the playlist player
  CServiceBroker::GetPlaylistPlayer().Add(playlistId, playlist);

  // if we have a playlist
  if (CServiceBroker::GetPlaylistPlayer().GetPlaylist(playlistId).size())
  {
    // start playing it
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(playlistId);
    CServiceBroker::GetPlaylistPlayer().Reset();
    CServiceBroker::GetPlaylistPlayer().Play(track, "");
    return true;
  }
  return false;
}

bool CApplication::GetRenderGUI() const
{
  return GetComponent<CApplicationPowerHandling>()->GetRenderGUI();
}

bool CApplication::SetLanguage(const std::string &strLanguage)
{
  // nothing to be done if the language hasn't changed
  if (strLanguage == CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_LANGUAGE))
    return true;

  return CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_LOCALE_LANGUAGE, strLanguage);
}

bool CApplication::LoadLanguage(bool reload)
{
  // load the configured language
  if (!g_langInfo.SetLanguage("", reload))
    return false;

  // set the proper audio and subtitle languages
  const boost::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  g_langInfo.SetAudioLanguage(settings->GetString(CSettings::SETTING_LOCALE_AUDIOLANGUAGE));
  g_langInfo.SetSubtitleLanguage(settings->GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE));

  return true;
}

void CApplication::SetLoggingIn(bool switchingProfiles)
{
  // don't save skin settings on unloading when logging into another profile
  // because in that case we have already loaded the new profile and
  // would therefore write the previous skin's settings into the new profile
  // instead of into the previous one
  GetComponent<CApplicationSkinHandling>()->m_saveSkinOnUnloading = !switchingProfiles;
}

void CApplication::PrintStartupLog()
{
  CLog::Log(LOGINFO, "-----------------------------------------------------------------------");
  CLog::Log(LOGINFO, "Starting Xodi. Built on %s", __DATE__);
  CSpecialProtocol::LogPaths();
  CLog::Log(LOGINFO, "-----------------------------------------------------------------------");
}
