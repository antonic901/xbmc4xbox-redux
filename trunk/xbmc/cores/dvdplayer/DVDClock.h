#pragma once

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

#include "utils/SharedSection.h"
#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and int64_t

#define DVD_TIME_TO_SEC(x)  ((int)((double)(x) / DVD_TIME_BASE))
#define DVD_TIME_TO_MSEC(x) ((int)((double)(x) * 1000 / DVD_TIME_BASE))
#define DVD_SEC_TO_TIME(x)  ((double)(x) * DVD_TIME_BASE)
#define DVD_MSEC_TO_TIME(x) ((double)(x) * DVD_TIME_BASE / 1000)

#define DVD_PLAYSPEED_PAUSE       0       // frame stepping
#define DVD_PLAYSPEED_NORMAL      1000

class CDVDClock
{
public:
  CDVDClock();
  ~CDVDClock();

  double GetClock();

  void Discontinuity(double currentPts = 0LL);
 
  void Reset() { m_bReset = true; }
  void Pause();
  void Resume();
  void SetSpeed(int iSpeed);

  static double GetAbsoluteClock();
  static double GetFrequency() { return (double)m_systemFrequency.QuadPart ; }
protected:
  CSharedSection m_critSection;
  LARGE_INTEGER m_systemUsed;  
  LARGE_INTEGER m_startClock;
  LARGE_INTEGER m_pauseClock;
  double m_iDisc;
  bool m_bReset;
  
  static LARGE_INTEGER m_systemFrequency;
};
