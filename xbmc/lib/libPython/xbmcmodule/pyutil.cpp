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

#include "pyutil.h"
#include "pythreadstate.h"
#include <wchar.h>
#include "addons/Skin.h"
#include "tinyXML/tinyxml.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "CharsetConverter.h"

using namespace std;

static int iPyXBMCGUILockRef = 0;
static TiXmlDocument pySkinReferences; 

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

namespace PYXBMC
{
  int PyXBMCGetUnicodeString(string& buf, PyObject* pObject, int pos)
  {
    // It's okay for a string to be "None". In this case the buf returned
    // will be the emptyString.
    if (pObject == Py_None)
    {
      buf = "";
      return 1;
    }

    // TODO: UTF-8: Does python use UTF-16?
    //              Do we need to convert from the string charset to UTF-8
    //              for non-unicode data?
    if (PyUnicode_Check(pObject))
    {
      CStdString utf8String;
      g_charsetConverter.wToUTF8(PyUnicode_AsUnicode(pObject), utf8String);
      buf = utf8String;
      return 1;
    }
    if (PyString_Check(pObject))
    {
      CStdString utf8String;
      g_charsetConverter.unknownToUTF8(PyString_AsString(pObject), utf8String);
      buf = utf8String;
      return 1;
    }

    PyObject* pyStrCast = PyObject_Str(pObject);
    if (pyStrCast)
    {
       int ret = PyXBMCGetUnicodeString(buf, pyStrCast, pos);
       Py_DECREF(pyStrCast);
       return ret;
    }

    // Object is not a unicode or a normal string.
    buf = "";
    if (pos != -1) PyErr_Format(PyExc_TypeError, "argument %.200i must be unicode or str", pos);
    return 0;
  }

  void PyXBMCGUILock()
  {
    if (iPyXBMCGUILockRef == 0) g_graphicsContext.Lock();
    iPyXBMCGUILockRef++;
  }

  void PyXBMCGUIUnlock()
  {
    if (iPyXBMCGUILockRef > 0)
    {
      iPyXBMCGUILockRef--;
      if (iPyXBMCGUILockRef == 0) g_graphicsContext.Unlock();
    }
  }

  void PyXBMCWaitForThreadMessage(int message, int param1, int param2)
  {
    CPyThreadState pyState;
    ThreadMessage tMsg = {message, param1, param2};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
  }

  static char defaultImage[1024];
  /*
   * Looks in references.xml for image name
   * If none exist return default image name
   */
  const char *PyXBMCGetDefaultImage(char* cControlType, char* cTextureType, char* cDefault)
  {
    // create an xml block so that we can resolve our defaults
    // <control type="type">
    //   <description />
    // </control>
    TiXmlElement control("control");
    control.SetAttribute("type", cControlType);
    TiXmlElement filler("description");
    control.InsertEndChild(filler);
    g_SkinInfo->ResolveIncludes(&control);

    // ok, now check for our texture type
    TiXmlElement *pTexture = control.FirstChildElement(cTextureType);
    if (pTexture)
    {
      // found our textureType
      TiXmlNode *pNode = pTexture->FirstChild();
      if (pNode && pNode->Value()[0] != '-')
      {
        strncpy(defaultImage, pNode->Value(), 1024);
        return defaultImage;
      }
    }
    return cDefault;
  }

  bool PyXBMCWindowIsNull(void* pWindow)
  {
    if (pWindow == NULL)
    {
      PyErr_SetString(PyExc_SystemError, "Error: Window is NULL, this is not possible :-)");
      return true;
    }
    return false;
  }

  void PyXBMCInitializeTypeObject(PyTypeObject* type_object)
  {
    static PyTypeObject py_type_object_header = { PyObject_HEAD_INIT(NULL) 0};
    int size = (long*)&(py_type_object_header.tp_name) - (long*)&py_type_object_header;

    memset(type_object, 0, sizeof(PyTypeObject));
    memcpy(type_object, &py_type_object_header, size);
  }

  long PyXBMCLongAsStringOrLong(PyObject *value)
  {
    if (PyString_Check(value) || PyUnicode_Check(value))
    {
      const char *s;
      if ((s = PyString_AsString(value)) != NULL)
        return atol(s);
      else
        return 0;
    }
    else
      return PyLong_AsLong(value);
  }

  bool PyXBMCGetAddonId(std::string &addonId)
  {
    // we need to retrieve the addon's ID from python using
    // __xbmcaddonid__ so let's get a reference to the main
    // module and global dictionary
    PyObject* main_module = PyImport_AddModule((char*)"__main__");
    PyObject* global_dict = PyModule_GetDict(main_module);

    // extract a reference to the function "func_name"
    // from the global dictionary
    PyObject* pyid = PyDict_GetItemString(global_dict, "__xbmcaddonid__");

    // if we were unable to retrieve the addon's ID return false
    if (pyid == NULL)
     return false;

    addonId = PyString_AsString(pyid);
    return true;
  }
}

struct SPending
{
  int(*func)(void*);
  void*          args;
  PyThreadState* state;
};

typedef std::vector<SPending> CallQueue;
static CallQueue g_callQueue;
static CCriticalSection g_critSectionPyCall;

void _PyXBMC_AddPendingCall(PyThreadState* state, int(*func)(void*), void *arg)
{
  CSingleLock lock(g_critSectionPyCall);
  SPending p;
  p.func  = func;
  p.args  = arg;
  p.state = state;
  g_callQueue.push_back(p);
}

void _PyXBMC_ClearPendingCalls(PyThreadState* state)
{
  CSingleLock lock(g_critSectionPyCall);
  for(CallQueue::iterator it = g_callQueue.begin(); it!= g_callQueue.end();)
  {
    if(it->state = state)
      it = g_callQueue.erase(it);
    else
      it++;
  }
}

void _PyXBMC_MakePendingCalls()
{
  CSingleLock lock(g_critSectionPyCall);
  CallQueue::iterator iter = g_callQueue.begin();
  while (iter != g_callQueue.end())
  {
    SPending p(*iter);
    // only call when we are in the right thread state
    if(p.state != PyThreadState_Get())
    {
      iter++;
      continue;
    }
    g_callQueue.erase(iter);
    lock.Leave();
    if (p.func)
      p.func(p.args);
    //(*((*iter).first))((*iter).second);
    lock.Enter();
    iter = g_callQueue.begin();
  }
}
