/*!
 \file Key.h
 \brief
 */

#ifndef GUILIB_KEY
#define GUILIB_KEY

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

#include "system.h"

#ifndef SWIG


namespace KODI
{
namespace MOUSE
{
/*!
  \ingroup actionkeys, mouse
  \brief Simple class for mouse events
  */
class CMouseEvent
{
public:
  CMouseEvent(int actionID, int state = 0, float offsetX = 0, float offsetY = 0)
  {
    m_id = actionID;
    m_state = state;
    m_offsetX = offsetX;
    m_offsetY = offsetY;
  };

  int    m_id;
  int    m_state;
  float  m_offsetX;
  float  m_offsetY;
};
}
}

/*!
  \ingroup actionkeys
  \brief
  */
class CKey
{
public:
  CKey(void);
  CKey(uint32_t buttonCode, uint8_t leftTrigger = 0, uint8_t rightTrigger = 0, float leftThumbX = 0.0f, float leftThumbY = 0.0f, float rightThumbX = 0.0f, float rightThumbY = 0.0f, float repeat = 0.0f);
  CKey(const CKey& key);

  virtual ~CKey(void);
  const CKey& operator=(const CKey& key);
  uint32_t GetButtonCode() const; // for backwards compatibility only
  wchar_t GetUnicode() const; // http api does not really support unicode till now. It only simulates unicode when ascii codes are available:
  uint8_t GetLeftTrigger() const;
  uint8_t GetRightTrigger() const;
  float GetLeftThumbX() const;
  float GetLeftThumbY() const;
  float GetRightThumbX() const;
  float GetRightThumbY() const;
  float GetRepeat() const;
  bool FromKeyboard() const;
  bool IsAnalogButton() const;
  bool IsIRRemote() const;
  void SetFromHttpApi(bool);
  bool GetFromHttpApi() const;

  void SetHeld(unsigned int held);
  unsigned int GetHeld() const;
private:
  uint32_t m_buttonCode;
  uint8_t m_leftTrigger;
  uint8_t m_rightTrigger;
  float m_leftThumbX;
  float m_leftThumbY;
  float m_rightThumbX;
  float m_rightThumbY;
  float m_repeat; // time since last keypress
  bool m_fromHttpApi;
  unsigned int m_held;
};
#endif //undef SWIG

#endif

