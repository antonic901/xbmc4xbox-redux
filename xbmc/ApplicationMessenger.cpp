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
#include "ApplicationMessenger.h"
#include "Application.h"
#ifdef _XBOX
#include "xbox/XKUtils.h"
#include "xbox/XKHDD.h"
#include "libGoAhead/XBMChttp.h"
#include "SectionLoader.h"
#endif

#include "LangInfo.h"
#include "PlayListPlayer.h"
#include "Util.h"
#include "pictures/GUIWindowSlideShow.h"
#include "interfaces/Builtins.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "xbox/network.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/GUIDialog.h"
#include "guilib/Key.h"
#include "utils/Splash.h"

#include "guilib/LocalizeStrings.h"
#include "threads/SingleLock.h"

#include "playlists/PlayList.h"

#include "utils/GlobalsHandling.h"

using namespace std;

extern HWND g_hWnd;

CDelayedMessage::CDelayedMessage(ThreadMessage& msg, unsigned int delay) : CThread("DelayedMessage")
{
  m_msg.dwMessage  = msg.dwMessage;
  m_msg.dwParam1   = msg.dwParam1;
  m_msg.dwParam2   = msg.dwParam2;
  m_msg.waitEvent  = msg.waitEvent;
  m_msg.lpVoid     = msg.lpVoid;
  m_msg.strParam   = msg.strParam;
  m_msg.params     = msg.params;

  m_delay = delay;
}

void CDelayedMessage::Process()
{
  Sleep(m_delay);

  if (!m_bStop)
    CApplicationMessenger::Get().SendMessage(m_msg, false);
}

CApplicationMessenger& CApplicationMessenger::Get()
{
  return s_messenger;
}

CApplicationMessenger::CApplicationMessenger()
{
}

CApplicationMessenger::~CApplicationMessenger()
{
  Cleanup();
}

void CApplicationMessenger::Cleanup()
{
  CSingleLock lock (m_critSection);

  while (m_vecMessages.size() > 0)
  {
    ThreadMessage* pMsg = m_vecMessages.front();

    if (pMsg->waitEvent)
      pMsg->waitEvent->Set();

    delete pMsg;
    m_vecMessages.pop();
  }

  while (m_vecWindowMessages.size() > 0)
  {
    ThreadMessage* pMsg = m_vecWindowMessages.front();

    if (pMsg->waitEvent)
      pMsg->waitEvent->Set();

    delete pMsg;
    m_vecWindowMessages.pop();
  }
}

void CApplicationMessenger::SendMessage(ThreadMessage& message, bool wait)
{
  message.waitEvent.reset();
  boost::shared_ptr<CEvent> waitEvent;
  if (wait)
  { // check that we're not being called from our application thread, else we'll be waiting
    // forever!
    if (GetCurrentThreadId() != g_application.GetThreadId())
    {
      message.waitEvent.reset(new CEvent(true));
      waitEvent = message.waitEvent;
    }
    else
    {
      //OutputDebugString("Attempting to wait on a SendMessage() from our application thread will cause lockup!\n");
      //OutputDebugString("Sending immediately\n");
      ProcessMessage(&message);
      return;
    }
  }

  CSingleLock lock (m_critSection);

  if (g_application.m_bStop)
  {
    if (message.waitEvent)
      message.waitEvent.reset();
    return;
  }

  ThreadMessage* msg = new ThreadMessage();
  msg->dwMessage = message.dwMessage;
  msg->dwParam1 = message.dwParam1;
  msg->dwParam2 = message.dwParam2;
  msg->waitEvent = message.waitEvent;
  msg->lpVoid = message.lpVoid;
  msg->strParam = message.strParam;
  msg->params = message.params;

  if (msg->dwMessage == TMSG_DIALOG_DOMODAL ||
      msg->dwMessage == TMSG_WRITE_SCRIPT_OUTPUT)
    m_vecWindowMessages.push(msg);
  else
    m_vecMessages.push(msg);
  lock.Leave();  // this releases the lock on the vec of messages and
                 //   allows the ProcessMessage to execute and therefore
                 //   delete the message itself. Therefore any accesss
                 //   of the message itself after this point consittutes
                 //   a race condition (yarc - "yet another race condition")
                 //
  if (waitEvent) // ... it just so happens we have a spare reference to the
                 //  waitEvent ... just for such contingencies :)
  { 
    // ensure the thread doesn't hold the graphics lock
    CSingleExit exit(g_graphicsContext);
    waitEvent->Wait();
  }
}

void CApplicationMessenger::ProcessMessages()
{
  // process threadmessages
  CSingleLock lock (m_critSection);
  while (m_vecMessages.size() > 0)
  {
    ThreadMessage* pMsg = m_vecMessages.front();
    //first remove the message from the queue, else the message could be processed more then once
    m_vecMessages.pop();

    //Leave here as the message might make another
    //thread call processmessages or sendmessage

    boost::shared_ptr<CEvent> waitEvent = pMsg->waitEvent; 
    lock.Leave(); // <- see the large comment in SendMessage ^

    ProcessMessage(pMsg);
    if (waitEvent)
      waitEvent->Set();
    delete pMsg;

    //Reenter here again, to not ruin message vector
    lock.Enter();
  }
}

void CApplicationMessenger::ProcessMessage(ThreadMessage *pMsg)
{
  switch (pMsg->dwMessage)
  {
    case TMSG_SHUTDOWN:
#ifdef _XBOX
    case TMSG_POWERDOWN:
#endif
      {
        g_application.Stop();
        Sleep(200);
#ifdef _XBOX
#ifndef _DEBUG  // don't actually shut off if debug build, it hangs VS for a long time
        XKHDD::SpindownHarddisk(); // Spindown the Harddisk
        XKUtils::XBOXPowerOff();
        while(1){Sleep(0);}
#endif
#else
        // send the WM_CLOSE window message
        ::SendMessage( g_hWnd, WM_CLOSE, 0, 0 );
#endif
      }
      break;

#ifndef _XBOX
case TMSG_POWERDOWN:
      {
#if !defined(_LINUX)
#ifdef _WIN32PC
        if (CWIN32Util::PowerManagement(POWERSTATE_SHUTDOWN))
#endif
#endif
#ifdef HAS_HAL
        if (CHalManager::PowerManagement(POWERSTATE_SHUTDOWN))
#endif
        {
          g_application.Stop();
          exit(0);
        }
      }
      break;
#endif

#ifdef _XBOX
    case TMSG_QUIT:
      {
        CBuiltins::Execute("XBMC.Dashboard()");
      }
      break;
#else
    case TMSG_QUIT:
      {
        g_application.Stop();
        Sleep(200);
        exit(0);
      }
      break;
#endif
    case TMSG_HIBERNATE:
      {
#ifdef HAS_HAL
        CHalManager::PowerManagement(POWERSTATE_HIBERNATE);
#elif defined(_WIN32PC)
        CWIN32Util::PowerManagement(POWERSTATE_HIBERNATE);
#endif
      }
      break;

    case TMSG_SUSPEND:
      {
#ifdef HAS_HAL
        CHalManager::PowerManagement(POWERSTATE_SUSPEND);
#elif defined(_WIN32PC)
        CWIN32Util::PowerManagement(POWERSTATE_SUSPEND);
#endif
      }
      break;

    case TMSG_RESTART:
      {
        g_application.Stop();
        Sleep(200);
#ifdef _XBOX
#ifndef _DEBUG  // don't actually shut off if debug build, it hangs VS for a long time
        XKUtils::XBOXPowerCycle();
        while(1){Sleep(0);}
#endif
#else
        // send the WM_CLOSE window message
        ::SendMessage( g_hWnd, WM_CLOSE, 0, 0 );
#endif
      }
      break;

    case TMSG_RESET:
      {
        g_application.Stop();
        Sleep(200);
#ifdef _XBOX
#ifndef _DEBUG  // don't actually shut off if debug build, it hangs VS for a long time
        XKUtils::XBOXReset();
        while(1){Sleep(0);}
#endif
#else
        // send the WM_CLOSE window message
        ::SendMessage( g_hWnd, WM_CLOSE, 0, 0 );
#endif
      }
      break;

    case TMSG_RESTARTAPP:
      {
        char szXBEFileName[1024];

        CIoSupport::GetXbePath(szXBEFileName);
        CUtil::RunXBE(szXBEFileName);
      }
      break;

    case TMSG_MEDIA_PLAY:
      {
        // first check if we were called from the PlayFile() function
        if (pMsg->lpVoid && pMsg->dwParam2 == 0)
        {
          CFileItem *item = (CFileItem *)pMsg->lpVoid;
          g_application.PlayFile(*item, pMsg->dwParam1 != 0);
          delete item;
          return;
        }
        // restore to previous window if needed
        if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
            g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
            g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
          g_windowManager.PreviousWindow();

        g_application.ResetScreenSaver();
        g_application.ResetScreenSaverWindow();

        //g_application.StopPlaying();
        // play file
        if(pMsg->lpVoid)
        {
          CFileItemList *list = (CFileItemList *)pMsg->lpVoid;

          if (list->Size() > 0)
          {
            int playlist = PLAYLIST_MUSIC;
            for (int i = 0; i < list->Size(); i++)
            {
              if ((*list)[i]->IsVideo())
              {
                playlist = PLAYLIST_VIDEO;
                break;
              }
            }

            //For single item lists try PlayMedia. This covers some more cases where a playlist is not appropriate
            //It will fall through to PlayFile
            if (list->Size() == 1 && !(*list)[0]->IsPlayList())
              g_application.PlayMedia(*((*list)[0]), playlist);
            else
            {
              // Handle "shuffled" option if present
              if (list->HasProperty("shuffled") && list->GetProperty("shuffled").isBoolean())
                g_playlistPlayer.SetShuffle(playlist, list->GetProperty("shuffled").asBoolean(), false);
              // Handle "repeat" option if present
              if (list->HasProperty("repeat") && list->GetProperty("repeat").isInteger())
                g_playlistPlayer.SetRepeat(playlist, (PLAYLIST::REPEAT_STATE)list->GetProperty("repeat").asInteger(), false);

              g_playlistPlayer.ClearPlaylist(playlist);
              g_playlistPlayer.Add(playlist, (*list));
              g_playlistPlayer.SetCurrentPlaylist(playlist);
              g_playlistPlayer.Play(pMsg->dwParam1);
            }
          }

          delete list;
        }
      }
      break;

    case TMSG_MEDIA_RESTART:
      g_application.Restart(true);
      break;

    case TMSG_PICTURE_SHOW:
      {
        CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (!pSlideShow) return ;

        // stop playing file
        if (g_application.IsPlayingVideo()) g_application.StopPlaying();

        if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
          g_windowManager.PreviousWindow();

        g_application.ResetScreenSaver();
        g_application.ResetScreenSaverWindow();

        g_graphicsContext.Lock();
        pSlideShow->Reset();
        if (g_windowManager.GetActiveWindow() != WINDOW_SLIDESHOW)
          g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
        if (URIUtils::IsZIP(pMsg->strParam) || URIUtils::IsRAR(pMsg->strParam)) // actually a cbz/cbr
        {
          CFileItemList items;
          CStdString strPath;
          if (URIUtils::IsZIP(pMsg->strParam))
            URIUtils::CreateArchivePath(strPath, "zip", pMsg->strParam.c_str(), "");
          else
            URIUtils::CreateArchivePath(strPath, "rar", pMsg->strParam.c_str(), "");

          CUtil::GetRecursiveListing(strPath, items, g_advancedSettings.m_pictureExtensions);
          if (items.Size() > 0)
          {
            for (int i=0;i<items.Size();++i)
            {
              pSlideShow->Add(items[i].get());
            }
            pSlideShow->Select(items[0]->GetPath());
          }
        }
        else
        {
          CFileItem item(pMsg->strParam, false);
          pSlideShow->Add(&item);
          pSlideShow->Select(pMsg->strParam);
        }
        g_graphicsContext.Unlock();
      }
      break;

    case TMSG_SLIDESHOW_SCREENSAVER:
    case TMSG_PICTURE_SLIDESHOW:
      {
        CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (!pSlideShow) return ;

        if (g_application.IsPlayingVideo())
          g_application.StopPlaying();

        g_graphicsContext.Lock();
        pSlideShow->Reset();

        CFileItemList items;
        CStdString strPath = pMsg->strParam;
        CStdString extensions = g_advancedSettings.m_pictureExtensions;
        if (pMsg->dwParam1)
          extensions += "|.tbn";
        CUtil::GetRecursiveListing(strPath, items, extensions);

        if (items.Size() > 0)
        {
          for (int i=0;i<items.Size();++i)
            pSlideShow->Add(items[i].get());
          pSlideShow->StartSlideShow(pMsg->dwMessage == TMSG_SLIDESHOW_SCREENSAVER); //Start the slideshow!
        }
        if (pMsg->dwMessage == TMSG_SLIDESHOW_SCREENSAVER)
          pSlideShow->Shuffle();

        if (g_windowManager.GetActiveWindow() != WINDOW_SLIDESHOW)
        {
          if(items.Size() == 0)
          {
            CSettings::Get().SetString("screensaver.mode", "screensaver.xbmc.builtin.dim");
            g_application.ActivateScreenSaver();
          }
          else
            g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
        }

        g_graphicsContext.Unlock();
      }
      break;

    case TMSG_MEDIA_STOP:
      {
        // restore to previous window if needed
        if (g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW ||
            g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
            g_windowManager.GetActiveWindow() == WINDOW_VISUALISATION)
          g_windowManager.PreviousWindow();

        g_application.ResetScreenSaver();
        g_application.ResetScreenSaverWindow();

        // stop playing file
        if (g_application.IsPlaying()) g_application.StopPlaying();
      }
      break;

    case TMSG_MEDIA_PAUSE:
      if (g_application.m_pPlayer)
      {
        g_application.ResetScreenSaver();
        g_application.ResetScreenSaverWindow();
        g_application.m_pPlayer->Pause();
      }
      break;

    case TMSG_SWITCHTOFULLSCREEN:
      if( g_windowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO )
        g_application.SwitchToFullScreen();
      break;

    case TMSG_HTTPAPI:
	{
      if (!m_pXbmcHttp)
      {
	    CSectionLoader::Load("LIBHTTP");
        m_pXbmcHttp = new CXbmcHttp();
      }
      switch (m_pXbmcHttp->xbmcCommand(pMsg->strParam))
      {
      case 1:
        Restart();
        break;
      case 2:
        Shutdown();
        break;
      case 3:
        RebootToDashBoard();
        break;
      case 4:
        Reset();
        break;
      case 5:
        RestartApp();
        break;
      }
    }
    break;

    case TMSG_EXECUTE_SCRIPT:
      CScriptInvocationManager::Get().Execute(pMsg->strParam);
      break;

    case TMSG_EXECUTE_BUILT_IN:
      CBuiltins::Execute(pMsg->strParam.c_str());
      break;

    case TMSG_PLAYLISTPLAYER_PLAY:
      if (pMsg->dwParam1 != (DWORD) -1)
        g_playlistPlayer.Play(pMsg->dwParam1);
      else
        g_playlistPlayer.Play();
      break;

    case TMSG_PLAYLISTPLAYER_PLAY_SONG_ID:
      if (pMsg->dwParam1 != (DWORD) -1)
        g_playlistPlayer.PlaySongId(pMsg->dwParam1);
      else
        g_playlistPlayer.Play();
      break;

    case TMSG_PLAYLISTPLAYER_NEXT:
      g_playlistPlayer.PlayNext();
      break;

    case TMSG_PLAYLISTPLAYER_PREV:
      g_playlistPlayer.PlayPrevious();
      break;

    case TMSG_PLAYLISTPLAYER_ADD:
      if(pMsg->lpVoid)
      {
        CFileItemList *list = (CFileItemList *)pMsg->lpVoid;

        g_playlistPlayer.Add(pMsg->dwParam1, (*list));
        delete list;
      }
      break;

    case TMSG_PLAYLISTPLAYER_CLEAR:
      g_playlistPlayer.ClearPlaylist(pMsg->dwParam1);
      break;

    case TMSG_PLAYLISTPLAYER_SHUFFLE:
      g_playlistPlayer.SetShuffle(pMsg->dwParam1, pMsg->dwParam2 > 0);
      break;

    case TMSG_PLAYLISTPLAYER_GET_ITEMS:
      if (pMsg->lpVoid)
      {
        PLAYLIST::CPlayList playlist = g_playlistPlayer.GetPlaylist(pMsg->dwParam1);
        CFileItemList *list = (CFileItemList *)pMsg->lpVoid; //DO NOT DELETE THIS!

        for (int i = 0; i < playlist.size(); i++)
          list->Add(playlist[i]);
      }
      break;

    case TMSG_PLAYLISTPLAYER_INSERT:
      if (pMsg->lpVoid)
      {
        CFileItemList *list = (CFileItemList *)pMsg->lpVoid;
        g_playlistPlayer.Insert(pMsg->dwParam1, (*list), pMsg->dwParam2);
        delete list;
      }
      break;

    case TMSG_PLAYLISTPLAYER_REMOVE:
      if (pMsg->dwParam1 != (DWORD) -1)
        g_playlistPlayer.Remove(pMsg->dwParam1,pMsg->dwParam2);
      break;

    // Window messages below here...
    case TMSG_DIALOG_DOMODAL:  //doModel of window
      {
        CGUIDialog* pDialog = (CGUIDialog*)g_windowManager.GetWindow(pMsg->dwParam1);
        if (!pDialog) return ;
        pDialog->DoModal();
      }
      break;

    case TMSG_WRITE_SCRIPT_OUTPUT:
      {
        CGUIMessage msg(GUI_MSG_USER, 0, 0);
        msg.SetLabel(pMsg->strParam);
        CGUIWindow* pWindowScripts = g_windowManager.GetWindow(WINDOW_DIALOG_TEXT_VIEWER);
        if (pWindowScripts) pWindowScripts->OnMessage(msg);
      }
      break;

    case TMSG_NETWORKMESSAGE:
      {
        g_application.getNetwork().NetworkMessage((CNetwork::EMESSAGE)pMsg->dwParam1, pMsg->dwParam2);
      }
      break;

    case TMSG_GUI_DO_MODAL:
      {
        CGUIDialog *pDialog = (CGUIDialog *)pMsg->lpVoid;
        if (pDialog)
          pDialog->DoModal_Internal((int)pMsg->dwParam1, pMsg->strParam);
      }
      break;

    case TMSG_GUI_SHOW:
      {
        CGUIDialog *pDialog = (CGUIDialog *)pMsg->lpVoid;
        if (pDialog)
          pDialog->Show_Internal();
      }
      break;

    case TMSG_GUI_DIALOG_CLOSE:
      {
        CGUIDialog *dialog = (CGUIDialog *)pMsg->lpVoid;
        if (dialog)
          dialog->Close_Internal(pMsg->dwParam1 > 0);
      }
      break;

    case TMSG_GUI_ACTIVATE_WINDOW:
      {
        g_windowManager.ActivateWindow(pMsg->dwParam1, pMsg->params, pMsg->dwParam2 > 0);
      }
      break;

    case TMSG_GUI_PYTHON_DIALOG:
      {
        // This hack is not much better but at least I don't need to make ApplicationMessenger
        //  know about Addon (Python) specific classes.
        CAction caction(pMsg->dwParam1);
        ((CGUIWindow*)pMsg->lpVoid)->OnAction(caction);
      }
      break;

    case TMSG_GUI_ACTION:
      {
        if (pMsg->lpVoid)
        {
          CAction *action = (CAction *)pMsg->lpVoid;
          if (pMsg->dwParam1 == WINDOW_INVALID)
            g_application.OnAction(*action);
          else
          {
            CGUIWindow *pWindow = g_windowManager.GetWindow(pMsg->dwParam1);  
            if (pWindow)
              pWindow->OnAction(*action);
            else
              CLog::Log(LOGWARNING, "Failed to get window with ID %i to send an action to", pMsg->dwParam1);
          }
          delete action;
        }
      }
      break;

    case TMSG_GUI_MESSAGE:
      {
        if (pMsg->lpVoid)
        {
          CGUIMessage *message = (CGUIMessage *)pMsg->lpVoid;
          g_windowManager.SendMessage(*message, pMsg->dwParam1);
          delete message;
        }
      }
      break;

    case TMSG_VOLUME_SHOW:
      {
        CAction action((int)pMsg->dwParam1);
        g_application.ShowVolumeBar(&action);
      }
    case TMSG_SPLASH_MESSAGE:
      {
        if (g_application.GetSplash())
          g_application.GetSplash()->Show(pMsg->strParam);
      }
  }
}

void CApplicationMessenger::ProcessWindowMessages()
{
  CSingleLock lock (m_critSection);
  //message type is window, process window messages
  while (m_vecWindowMessages.size() > 0)
  {
    ThreadMessage* pMsg = m_vecWindowMessages.front();
    //first remove the message from the queue, else the message could be processed more then once
    m_vecWindowMessages.pop();

    // leave here in case we make more thread messages from this one

    boost::shared_ptr<CEvent> waitEvent = pMsg->waitEvent;
    lock.Leave(); // <- see the large comment in SendMessage ^

    ProcessMessage(pMsg);
    if (waitEvent)
      waitEvent->Set();
    delete pMsg;

    lock.Enter();
  }
}

int CApplicationMessenger::SetResponse(CStdString response)
{
  CSingleLock lock (m_critBuffer);
  bufferResponse=response;
  lock.Leave();
  return 0;
}

CStdString CApplicationMessenger::GetResponse()
{
  CStdString tmp;
  CSingleLock lock (m_critBuffer);
  tmp=bufferResponse;
  lock.Leave();
  return tmp;
}

void CApplicationMessenger::HttpApi(string cmd, bool wait)
{
  SetResponse("");
  ThreadMessage tMsg = {TMSG_HTTPAPI};
  tMsg.strParam = cmd;
  SendMessage(tMsg, wait);
}

void CApplicationMessenger::ExecBuiltIn(const CStdString &command, bool wait)
{
  ThreadMessage tMsg = {TMSG_EXECUTE_BUILT_IN};
  tMsg.strParam = command;
  SendMessage(tMsg, wait);
}

void CApplicationMessenger::MediaPlay(string filename)
{
  CFileItem item;
  item.SetPath(filename);
  item.m_bIsFolder = false;
  MediaPlay(item);
}

void CApplicationMessenger::MediaPlay(const CFileItem &item)
{
  CFileItemList list;
  list.Add(CFileItemPtr(new CFileItem(item)));

  MediaPlay(list);
}

void CApplicationMessenger::MediaPlay(const CFileItemList &list, int song)
{
  ThreadMessage tMsg = {TMSG_MEDIA_PLAY};
  CFileItemList* listcopy = new CFileItemList();
  listcopy->Copy(list);
  tMsg.lpVoid = (void*)listcopy;
  tMsg.dwParam1 = song;
  tMsg.dwParam2 = 1;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayFile(const CFileItem &item, bool bRestart /*= false*/)
{
  ThreadMessage tMsg = {TMSG_MEDIA_PLAY};
  CFileItem *pItem = new CFileItem(item);
  tMsg.lpVoid = (void *)pItem;
  tMsg.dwParam1 = bRestart ? 1 : 0;
  tMsg.dwParam2 = 0;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::MediaStop()
{
  ThreadMessage tMsg = {TMSG_MEDIA_STOP};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::MediaPause()
{
  ThreadMessage tMsg = {TMSG_MEDIA_PAUSE};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::MediaRestart(bool bWait)
{
  ThreadMessage tMsg = {TMSG_MEDIA_RESTART};
  SendMessage(tMsg, bWait);
}

void CApplicationMessenger::PlayListPlayerPlay()
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY, (DWORD) -1};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerPlay(int iSong)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY, iSong};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerPlaySongId(int songId)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY_SONG_ID, songId};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerNext()
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_NEXT};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerPrevious()
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PREV};
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerAdd(int playlist, const CFileItem &item)
{
  CFileItemList list;
  list.Add(CFileItemPtr(new CFileItem(item)));

  PlayListPlayerAdd(playlist, list);
}

void CApplicationMessenger::PlayListPlayerAdd(int playlist, const CFileItemList &list)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_ADD};
  CFileItemList* listcopy = new CFileItemList();
  listcopy->Copy(list);
  tMsg.lpVoid = (void*)listcopy;
  tMsg.dwParam1 = playlist;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerClear(int playlist)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_CLEAR};
  tMsg.dwParam1 = playlist;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerShuffle(int playlist, bool shuffle)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_SHUFFLE};
  tMsg.dwParam1 = playlist;
  tMsg.dwParam2 = shuffle ? 1 : 0;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PlayListPlayerGetItems(int playlist, CFileItemList &list)
{
  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_GET_ITEMS};
  tMsg.dwParam1 = playlist;
  tMsg.lpVoid = (void *)&list;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::PictureShow(string filename)
{
  ThreadMessage tMsg = {TMSG_PICTURE_SHOW};
  tMsg.strParam = filename;
  SendMessage(tMsg);
}

void CApplicationMessenger::PictureSlideShow(string pathname, bool bScreensaver /* = false */, bool addTBN /* = false */)
{
  DWORD dwMessage = TMSG_PICTURE_SLIDESHOW;
  if (bScreensaver)
    dwMessage = TMSG_SLIDESHOW_SCREENSAVER;
  ThreadMessage tMsg = {dwMessage};
  tMsg.strParam = pathname;
  tMsg.dwParam1 = addTBN ? 1 : 0;
  SendMessage(tMsg);
}

void CApplicationMessenger::Shutdown()
{
  ThreadMessage tMsg = {TMSG_SHUTDOWN};
  SendMessage(tMsg);
}

void CApplicationMessenger::Powerdown()
{
  ThreadMessage tMsg = {TMSG_POWERDOWN};
  SendMessage(tMsg);
}

void CApplicationMessenger::Quit()
{
  ThreadMessage tMsg = {TMSG_QUIT};
  SendMessage(tMsg);
}

void CApplicationMessenger::Hibernate()
{
  ThreadMessage tMsg = {TMSG_HIBERNATE};
  SendMessage(tMsg);
}

void CApplicationMessenger::Suspend()
{
  ThreadMessage tMsg = {TMSG_SUSPEND};
  SendMessage(tMsg);
}

void CApplicationMessenger::Restart()
{
  ThreadMessage tMsg = {TMSG_RESTART};
  SendMessage(tMsg);
}

void CApplicationMessenger::Reset()
{
  ThreadMessage tMsg = {TMSG_RESET};
  SendMessage(tMsg);
}

void CApplicationMessenger::RestartApp()
{
  ThreadMessage tMsg = {TMSG_RESTARTAPP};
  SendMessage(tMsg);
}

void CApplicationMessenger::RebootToDashBoard()
{
  ThreadMessage tMsg = {TMSG_DASHBOARD};
  SendMessage(tMsg);
}

void CApplicationMessenger::NetworkMessage(DWORD dwMessage, DWORD dwParam)
{
  ThreadMessage tMsg = {TMSG_NETWORKMESSAGE, dwMessage, dwParam};
  SendMessage(tMsg);
}

void CApplicationMessenger::SwitchToFullscreen()
{
  /* FIXME: ideally this call should return upon a successfull switch but currently
     is causing deadlocks between the dvdplayer destructor and the rendermanager
  */
  ThreadMessage tMsg = {TMSG_SWITCHTOFULLSCREEN};
  SendMessage(tMsg, false);
}

void CApplicationMessenger::DoModal(CGUIDialog *pDialog, int iWindowID, const CStdString &param)
{
  ThreadMessage tMsg = {TMSG_GUI_DO_MODAL};
  tMsg.lpVoid = pDialog;
  tMsg.dwParam1 = (DWORD)iWindowID;
  tMsg.strParam = param;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::Show(CGUIDialog *pDialog)
{
  ThreadMessage tMsg = {TMSG_GUI_SHOW};
  tMsg.lpVoid = pDialog;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::Close(CGUIDialog *dialog, bool forceClose,
                                  bool waitResult)
{
  ThreadMessage tMsg = {TMSG_GUI_DIALOG_CLOSE, forceClose ? 1 : 0};
  tMsg.lpVoid = dialog;
  SendMessage(tMsg, waitResult);
}

void CApplicationMessenger::ActivateWindow(int windowID, const vector<CStdString> &params, bool swappingWindows)
{
  ThreadMessage tMsg = {TMSG_GUI_ACTIVATE_WINDOW, windowID, swappingWindows ? 1 : 0};
  tMsg.params = params;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::SendAction(const CAction &action, int windowID, bool waitResult)
{
  ThreadMessage tMsg = {TMSG_GUI_ACTION};
  tMsg.dwParam1 = windowID;
  tMsg.lpVoid = new CAction(action);
  SendMessage(tMsg, waitResult);
}

void CApplicationMessenger::SendGUIMessage(const CGUIMessage &message, int windowID, bool waitResult)
{
  ThreadMessage tMsg = {TMSG_GUI_MESSAGE};
  tMsg.dwParam1 = windowID == WINDOW_INVALID ? 0 : windowID;
  tMsg.lpVoid = new CGUIMessage(message);
  SendMessage(tMsg, waitResult);
}

void CApplicationMessenger::ShowVolumeBar(bool up)
{
  ThreadMessage tMsg = {TMSG_VOLUME_SHOW};
  tMsg.dwParam1 = up ? ACTION_VOLUME_UP : ACTION_VOLUME_DOWN;
  SendMessage(tMsg, false);
}

void CApplicationMessenger::SetSplashMessage(const CStdString& message)
{
  ThreadMessage tMsg = {TMSG_SPLASH_MESSAGE};
  tMsg.strParam = message;
  SendMessage(tMsg, true);
}

void CApplicationMessenger::SetSplashMessage(int stringID)
{
  SetSplashMessage(g_localizeStrings.Get(stringID));
}
