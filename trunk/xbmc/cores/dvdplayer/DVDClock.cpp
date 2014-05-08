/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "DVDClock.h"
#include "utils/SingleLock.h"

#include <math.h>

LARGE_INTEGER CDVDClock::m_systemFrequency;
CCriticalSection CDVDClock::m_systemsection;

CDVDClock::CDVDClock()
{
  CSingleLock lock(m_systemsection);
  CheckSystemClock();

  m_systemUsed = m_systemFrequency;
  m_pauseClock.QuadPart = 0;
  m_bReset = true;
  m_iDisc = 0;
}

CDVDClock::~CDVDClock()
{}

// Returns the current absolute clock in units of DVD_TIME_BASE (usually microseconds).
double CDVDClock::GetAbsoluteClock()
{
  CheckSystemClock();

  LARGE_INTEGER current;
  QueryPerformanceCounter(&current);

  return SystemToAbsolute(current);
}

double CDVDClock::GetClock()
{
  CSharedLock lock(m_critSection);
  LARGE_INTEGER current;
  QueryPerformanceCounter(&current);
  return SystemToPlaying(current);
}

double CDVDClock::GetClock(double& absolute)
{
  LARGE_INTEGER current;
  QueryPerformanceCounter(&current);
  {
    CSingleLock lock(m_systemsection);
    CheckSystemClock();
    absolute = SystemToAbsolute(current);
  }

  CSharedLock lock(m_critSection);
  return SystemToPlaying(current);
}

void CDVDClock::SetSpeed(int iSpeed)
{
  // this will sometimes be a little bit of due to rounding errors, ie clock might jump abit when changing speed
  CExclusiveLock lock(m_critSection);

  if(iSpeed == DVD_PLAYSPEED_PAUSE)
  {
    if(!m_pauseClock.QuadPart)
      QueryPerformanceCounter(&m_pauseClock);
    return;
  }
  
  LARGE_INTEGER current;
  int64_t newfreq = m_systemFrequency.QuadPart * DVD_PLAYSPEED_NORMAL / iSpeed;
  
  QueryPerformanceCounter(&current);
  if( m_pauseClock.QuadPart )
  {
    m_startClock.QuadPart += current.QuadPart - m_pauseClock.QuadPart;
    m_pauseClock.QuadPart = 0;
  }

  m_startClock.QuadPart = current.QuadPart - (int64_t)((double)(current.QuadPart - m_startClock.QuadPart) * newfreq / m_systemUsed.QuadPart);
  m_systemUsed.QuadPart = newfreq;
}

void CDVDClock::Discontinuity(double currentPts)
{
  CExclusiveLock lock(m_critSection);
  QueryPerformanceCounter(&m_startClock);
  if(m_pauseClock.QuadPart)
    m_pauseClock.QuadPart = m_startClock.QuadPart;
  m_iDisc = currentPts;
  m_bReset = false;
}

void CDVDClock::Pause()
{
  CExclusiveLock lock(m_critSection);
  if(!m_pauseClock.QuadPart)
    QueryPerformanceCounter(&m_pauseClock);
}

void CDVDClock::Resume()
{
  CExclusiveLock lock(m_critSection);
  if( m_pauseClock.QuadPart )
  {
    LARGE_INTEGER current;
    QueryPerformanceCounter(&current);

    m_startClock.QuadPart += current.QuadPart - m_pauseClock.QuadPart;
    m_pauseClock.QuadPart = 0;
  }  
}

void CDVDClock::CheckSystemClock()
{
  if(!m_systemFrequency.QuadPart)
    QueryPerformanceFrequency(&m_systemFrequency);
}

double CDVDClock::SystemToAbsolute(LARGE_INTEGER system)
{
  return DVD_TIME_BASE * (double)system.QuadPart / m_systemFrequency.QuadPart;
}

double CDVDClock::SystemToPlaying(LARGE_INTEGER system)
{
  LARGE_INTEGER current;

  if (m_bReset)
  {
    m_startClock = system;
    m_systemUsed = m_systemFrequency;
    m_pauseClock.QuadPart = 0;
    m_iDisc = 0;
    m_bReset = false;
  }
  
  if (m_pauseClock.QuadPart)
    current = m_pauseClock;
  else
    current = system;

  return DVD_TIME_BASE * (double)(current.QuadPart - m_startClock.QuadPart) / m_systemUsed.QuadPart + m_iDisc;
}

