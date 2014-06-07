// CriticalSection.h: interface for the CCriticalSection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CRITICALSECTION_H__FC7E6698_33C9_42D6_81EA_F7869FEE11B7__INCLUDED_)
#define AFX_CRITICALSECTION_H__FC7E6698_33C9_42D6_81EA_F7869FEE11B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

class CCriticalSection
{
public:
  CCriticalSection();

  // Conversion operator
  operator LPCRITICAL_SECTION();
  virtual ~CCriticalSection();
  CCriticalSection(const CCriticalSection& section);
  CCriticalSection& operator=(const CCriticalSection& section);

private:

  CRITICAL_SECTION m_critSection;
};

BOOL  NTAPI OwningCriticalSection(LPCRITICAL_SECTION section);               /* checks if current thread owns the critical section */
DWORD NTAPI ExitCriticalSection(LPCRITICAL_SECTION section);                 /* leaves critical section fully, and returns count */ 
VOID  NTAPI RestoreCriticalSection(LPCRITICAL_SECTION section, DWORD count); /* restores critical section count */

#endif // !defined(AFX_CRITICALSECTION_H__FC7E6698_33C9_42D6_81EA_F7869FEE11B7__INCLUDED_)
