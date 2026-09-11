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

template<>
class atomic<__int64>
{
public:
  atomic()
  {
    m_value = 0;
  }

  void set(__int64 value)
  {
    LARGE_INTEGER new_value;
    new_value.QuadPart = value;

    __asm
    {
      mov ebx, new_value.LowPart
      mov ecx, new_value.HighPart
      mov esi, this
      mov eax, dword ptr [esi]
      mov edx, dword ptr [esi + 4]
    retry:
      lock cmpxchg8b qword ptr [esi]
      jnz retry
    }
  }

  __int64 value() const
  {
    LARGE_INTEGER result;

    __asm
    {
      mov esi, this
      mov eax, dword ptr [esi]
      mov edx, dword ptr [esi + 4]
    retry:
      mov ebx, eax
      mov ecx, edx
      lock cmpxchg8b qword ptr [esi]
      jnz retry
      mov result.LowPart, eax
      mov result.HighPart, edx
    }

    return result.QuadPart;
  }

private:
  __declspec(align(8)) mutable __int64 m_value;
};

typedef atomic<bool> atomic_bool;
typedef atomic<long> atomic_long;
