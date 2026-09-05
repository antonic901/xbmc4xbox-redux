/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system.h" // <xtl.h>

#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <string>
#include <utility>
#include <vector>

class CEvent;

namespace KODI
{
namespace MESSAGING
{

class CApplicationMessenger;

class ThreadMessage
{
  friend CApplicationMessenger;
public:
  ThreadMessage()
    : dwMessage(0)
    , param1(-1)
    , param2(-1)
    , param3(0)
    , lpVoid(NULL)
  {
  }

  explicit ThreadMessage(uint32_t messageId)
    : dwMessage(messageId)
    , param1(-1)
    , param2(-1)
    , param3(0)
    , lpVoid(NULL)
  {
  }

  ThreadMessage(uint32_t messageId, int64_t p3)
    : dwMessage(messageId)
    , param1(-1)
    , param2(-1)
    , param3(p3)
    , lpVoid(NULL)
  {
  }

  ThreadMessage(uint32_t messageId, int p1, int p2, void* payload, int64_t p3 = 0)
    : dwMessage( messageId )
    , param1( p1 )
    , param2( p2 )
    , param3( p3 )
    , lpVoid( payload )
  {
  }

  ThreadMessage(uint32_t messageId,
                int p1,
                int p2,
                void* payload,
                std::string param,
                std::vector<std::string> vecParams)
    : dwMessage(messageId),
      param1(p1),
      param2(p2),
      param3(0),
      lpVoid(payload),
      strParam(param),
      params(vecParams)
  {
  }

  ThreadMessage(const ThreadMessage& other)
    : dwMessage(other.dwMessage),
      param1(other.param1),
      param2(other.param2),
      param3(other.param3),
      lpVoid(other.lpVoid),
      strParam(other.strParam),
      params(other.params),
      waitEvent(other.waitEvent),
      result(other.result)
  {
  }

  ThreadMessage& operator=(const ThreadMessage& other)
  {
    if (this == &other)
      return *this;
    dwMessage = other.dwMessage;
    param1 = other.param1;
    param2 = other.param2;
    param3 = other.param3;
    lpVoid = other.lpVoid;
    strParam = other.strParam;
    params = other.params;
    waitEvent = other.waitEvent;
    result = other.result;
    return *this;
  }

  uint32_t dwMessage;
  int param1;
  int param2;
  int64_t param3;
  void* lpVoid;
  std::string strParam;
  std::vector<std::string> params;

  /*!
   * \brief set the message return value, will only be returned when
   *        the message is sent using SendMsg
   * \param [in] res the return value or a result status code that is returned to the caller
   */
  void SetResult(int res) const
  {
    //On posted messages result will be zero, since they can't
    //retrieve the response we silently ignore this to let message
    //handlers not have to worry about it
    if (result)
      *result = res;
  }
protected:
  boost::shared_ptr<CEvent> waitEvent;
  boost::shared_ptr<int> result;
};
}
}
