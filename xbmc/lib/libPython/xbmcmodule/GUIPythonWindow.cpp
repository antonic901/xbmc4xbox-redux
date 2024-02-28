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

#include "GUIPythonWindow.h"
#include "pyutil.h"
#include "window.h"
#include "control.h"
#include "action.h"
#include "GUIWindowManager.h"
#include "../XBPython.h"
#include "utils/log.h"

using namespace PYXBMC;

PyXBMCAction::~PyXBMCAction() {
     if (pObject) {
       Py_DECREF(pObject);
     }

     pObject = NULL;
     Py_DECREF(pCallbackWindow);
}

CGUIPythonWindow::CGUIPythonWindow(int id)
  : CGUIWindow(id, ""), m_actionEvent(true)
{
  pCallbackWindow = NULL;
  m_threadState = NULL;
  m_loadType = LOAD_ON_GUI_INIT;
}

CGUIPythonWindow::~CGUIPythonWindow(void)
{
}

bool CGUIPythonWindow::OnAction(const CAction &action)
{
  // call the base class first, then call python
  bool ret = CGUIWindow::OnAction(action);

  // workaround - for scripts which try to access the active control (focused) when there is none.
  // for example - the case when the mouse enters the screen.
  CGUIControl *pControl = GetFocusedControl();
  if (action.IsMouse() && !pControl)
     return ret;

  if(pCallbackWindow)
  {
    PyXBMCAction* inf = new PyXBMCAction(pCallbackWindow);
    inf->pObject = Action_FromAction(action);

    // aquire lock?
    PyXBMC_AddPendingCall(m_threadState, Py_XBMC_Event_OnAction, inf);
    PulseActionEvent();
  }
  return ret;
}

bool CGUIPythonWindow::OnBack(int actionID)
{
  // if we have a callback window then python handles the closing
  if (!pCallbackWindow)
    return CGUIWindow::OnBack(actionID);
  return true;
}

bool CGUIPythonWindow::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      g_windowManager.ShowOverlay(OVERLAY_STATE_SHOWN);
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      g_windowManager.ShowOverlay(OVERLAY_STATE_HIDDEN);
      return true;
    }
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      if(pCallbackWindow)
      {
        PyXBMCAction* inf = new PyXBMCAction(pCallbackWindow);
        // find python control object with same iControl
        std::vector<Control*>::iterator it = ((PYXBMC::Window*)pCallbackWindow)->vecControls.begin();
        while (it != ((PYXBMC::Window*)pCallbackWindow)->vecControls.end())
        {
          Control* pControl = *it;
          if (pControl->iControlId == iControl)
          {
            inf->pObject = (PyObject*)pControl;
            Py_INCREF(inf->pObject);
            break;
          }
          ++it;
        }
        // did we find our control?
        if (inf->pObject)
        {
          // currently we only accept messages from a button or controllist with a select action
          if ((ControlList_CheckExact(inf->pObject) && (message.GetParam1() == ACTION_SELECT_ITEM || message.GetParam1() == ACTION_MOUSE_LEFT_CLICK)) ||
            ControlButton_CheckExact(inf->pObject) || ControlRadioButton_CheckExact(inf->pObject) ||
            ControlCheckMark_CheckExact(inf->pObject))
          {
            // aquire lock?
            PyXBMC_AddPendingCall(m_threadState, Py_XBMC_Event_OnControl, inf);
            PulseActionEvent();

            // return true here as we are handling the event
            return true;
          }
        }

        // if we get here, we didn't add the action
        delete inf;
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIPythonWindow::SetCallbackWindow(PyThreadState *state, PyObject *object)
{
  pCallbackWindow = object;
  m_threadState   = state;
}

void CGUIPythonWindow::WaitForActionEvent()
{
  g_pythonParser.WaitForEvent(m_actionEvent);
  m_actionEvent.Reset();
}

void CGUIPythonWindow::PulseActionEvent()
{
  m_actionEvent.Set();
}

/*
 * called from python library!
 */
int Py_XBMC_Event_OnControl(void* arg)
{
  if (arg != NULL)
  {
    PyXBMCAction* action = (PyXBMCAction*)arg;
    PyObject *ret = PyObject_CallMethod(action->pCallbackWindow, (char*)"onControl", (char*)"(O)", action->pObject);
    if (ret) {
       Py_DECREF(ret);
    }
    delete action;
  }
  return 0;
}

/*
 * called from python library!
 */
int Py_XBMC_Event_OnAction(void* arg)
{
  if (arg != NULL)
  {
    PyXBMCAction* action = (PyXBMCAction*)arg;
    Action *pAction= (Action *)action->pObject;

    PyObject *ret = PyObject_CallMethod(action->pCallbackWindow, (char*)"onAction", (char*)"(O)", pAction);
    if (ret) {
      Py_DECREF(ret);
    }
    else {
      CLog::Log(LOGERROR,"Exception in python script's onAction");
      PyErr_Print();
    }
    delete action;
  }
  return 0;
}
