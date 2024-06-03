#pragma once

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

#include "threads/CriticalSection.h"
#include "guilib/Key.h"
#include "threads/Thread.h"

#include "xbox/PlatformDefs.h"

#include <queue>
#include "utils/GlobalsHandling.h"

class CFileItem;
class CFileItemList;
class CGUIDialog;
class CGUIMessage;
class CGUIWindow;

// defines here
#define TMSG_WRITE_SCRIPT_OUTPUT  101
#define TMSG_EXECUTE_SCRIPT       102
#define TMSG_EXECUTE_BUILT_IN     103

#define TMSG_MEDIA_PLAY           200
#define TMSG_MEDIA_STOP           201
#define TMSG_MEDIA_PAUSE          202
#define TMSG_MEDIA_RESTART        203

#define TMSG_PLAYLISTPLAYER_PLAY  210
#define TMSG_PLAYLISTPLAYER_NEXT  211
#define TMSG_PLAYLISTPLAYER_PREV  212
#define TMSG_PLAYLISTPLAYER_ADD   213
#define TMSG_PLAYLISTPLAYER_CLEAR 214
#define TMSG_PLAYLISTPLAYER_SHUFFLE   215
#define TMSG_PLAYLISTPLAYER_GET_ITEMS 216
#define TMSG_PLAYLISTPLAYER_PLAY_SONG_ID 217
#define TMSG_PLAYLISTPLAYER_INSERT 218
#define TMSG_PLAYLISTPLAYER_REMOVE 219

#define TMSG_PICTURE_SHOW         220
#define TMSG_PICTURE_SLIDESHOW    221
#define TMSG_SLIDESHOW_SCREENSAVER  222

#define TMSG_SHUTDOWN             300
#define TMSG_POWERDOWN            301
#define TMSG_QUIT                 302
#define TMSG_DASHBOARD            TMSG_QUIT
#define TMSG_HIBERNATE            303
#define TMSG_SUSPEND              304
#define TMSG_RESTART              305
#define TMSG_RESET                306
#define TMSG_RESTARTAPP           307
#define TMSG_SWITCHTOFULLSCREEN   308

#define TMSG_HTTPAPI              400

#define TMSG_NETWORKMESSAGE         500

#define TMSG_GUI_DIALOG_OPEN          600
#define TMSG_GUI_ACTIVATE_WINDOW      604
#define TMSG_GUI_PYTHON_DIALOG        605
#define TMSG_GUI_WINDOW_CLOSE         606
#define TMSG_GUI_ACTION               607
#define TMSG_GUI_MESSAGE              610

#define TMSG_VOLUME_SHOW          900
#define TMSG_SPLASH_MESSAGE       901

typedef struct
{
  DWORD dwMessage;
  DWORD dwParam1;
  DWORD dwParam2;
  CStdString strParam;
  std::vector<std::string> params;
  boost::shared_ptr<CEvent> waitEvent;
  LPVOID lpVoid;
}
ThreadMessage;

class CDelayedMessage : public CThread
{
  public:
    CDelayedMessage(ThreadMessage& msg, unsigned int delay);
    virtual void Process();

  private:
    unsigned int   m_delay;
    ThreadMessage  m_msg;
};

class CApplicationMessenger;
namespace xbmcutil
{
   template<class T> class GlobalsSingleton;
}

class CApplicationMessenger
{
public:
  /*!
   \brief The only way through which the global instance of the CApplicationMessenger should be accessed.
   \return the global instance.
   */
  static CApplicationMessenger& Get();

  void Cleanup();
  // if a message has to be send to the gui, use MSG_TYPE_WINDOW instead
  void SendMessage(ThreadMessage& msg, bool wait = false);
  void ProcessMessages(); // only call from main thread.
  void ProcessWindowMessages();


  void MediaPlay(std::string filename);
  void MediaPlay(const CFileItem &item);
  void MediaPlay(const CFileItemList &item, int song = 0);
  void MediaStop();
  void MediaPause();
  void MediaRestart(bool bWait);

  void PlayListPlayerPlay();
  void PlayListPlayerPlay(int iSong);
  void PlayListPlayerPlaySongId(int songId, bool &returnState);
  void PlayListPlayerNext();
  void PlayListPlayerPrevious();
  void PlayListPlayerAdd(int playlist, const CFileItem &item);
  void PlayListPlayerAdd(int playlist, const CFileItemList &list);
  void PlayListPlayerClear(int playlist);
  void PlayListPlayerShuffle(int playlist, bool shuffle);
  void PlayListPlayerGetItems(int playlist, CFileItemList &list);
  void PlayListPlayerInsert(int playlist, const CFileItem &item, int position); 
  void PlayListPlayerInsert(int playlist, const CFileItemList &list, int position);
  void PlayListPlayerRemove(int playlist, int position);

  void PlayFile(const CFileItem &item, bool bRestart = false); // thread safe version of g_application.PlayFile()
  void PictureShow(std::string filename);
  void PictureSlideShow(std::string pathname, bool bScreensaver = false, bool addTBN = false);
  void Shutdown();
  void Powerdown();
  void Quit();
  void Hibernate();
  void Suspend();
  void Restart();
  void RebootToDashBoard();
  void RestartApp();
  void Reset();
  void SwitchToFullscreen(); //

  CStdString GetResponse();
  int SetResponse(CStdString response);
  void HttpApi(std::string cmd, bool wait = false);
  void ExecBuiltIn(const CStdString &command, bool wait = false);

  void NetworkMessage(DWORD dwMessage, DWORD dwParam = 0);

  void Open(CGUIDialog *pDialog);
  void Close(CGUIWindow *window, bool forceClose, bool waitResult = true, int nextWindowID = 0, bool enableSound = true);
  void ActivateWindow(int windowID, const std::vector<std::string> &params, bool swappingWindows, bool force = false);
  void SendAction(const CAction &action, int windowID = WINDOW_INVALID, bool waitResult=true);

  /*! \brief Send a GUIMessage, optionally waiting before it's processed to return.
   Should be used to send messages to the GUI from other threads.
   \param msg the GUIMessage to send.
   \param windowID optional window to send the message to (defaults to no specified window).
   \param waitResult whether to wait for the result (defaults to false).
   */
  void SendGUIMessage(const CGUIMessage &msg, int windowID = WINDOW_INVALID, bool waitResult=false);

  void ShowVolumeBar(bool up);

  void SetSplashMessage(const CStdString& message);
  void SetSplashMessage(int stringID);

  virtual ~CApplicationMessenger();
private:
  // private construction, and no assignements; use the provided singleton methods
   friend class xbmcutil::GlobalsSingleton<CApplicationMessenger>;
  CApplicationMessenger();
  CApplicationMessenger(const CApplicationMessenger&);
  CApplicationMessenger const& operator=(CApplicationMessenger const&);
  void ProcessMessage(ThreadMessage *pMsg);

  std::queue<ThreadMessage*> m_vecMessages;
  std::queue<ThreadMessage*> m_vecWindowMessages;
  CCriticalSection m_critSection;
  CCriticalSection m_critBuffer;
  CStdString bufferResponse;
};

XBMC_GLOBAL_REF(CApplicationMessenger,s_messenger);
#define s_messenger XBMC_GLOBAL_USE(CApplicationMessenger)
