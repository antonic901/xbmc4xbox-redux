/*
 *  Copyright (C) Nikola Antonic
 *  This file is part of Xbox Media Center - https://github.com/antonic901/xbmc4xbox-redux
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

template<typename T1>
class atomic
{
  virtual void set(T1 value) = 0;
  virtual void value() = 0;

private:
  T1 m_value;
};

template<>
class atomic<bool>
{
public:
  atomic()
  {
    m_value = 0;
  }

  void set(bool value)
  {
    InterlockedExchange(&m_value, value ? 1 : 0);
  }

  bool value() const
  {
    return InterlockedCompareExchange(&m_value, 0, 0) != 0;
  }

private:
  mutable LONG m_value;
};

template<>
class atomic<long>
{
public:
  atomic()
  {
    m_value = 0;
  }

  void set(long value)
  {
    InterlockedExchange(&m_value, value);
  }

  long value() const
  {
    return InterlockedCompareExchange(&m_value, 0, 0);
  }

private:
  mutable LONG m_value;
};

typedef atomic<bool> atomic_bool;
typedef atomic<long> atomic_long;
