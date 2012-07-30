/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "utils/TimeUtils.h"
#include "CircularCache.h"

using namespace XFILE;

CCacheCircular::CCacheCircular(size_t front, size_t back)
 : CCacheStrategy()
 , m_beg(0)
 , m_end(0)
 , m_cur(0)
 , m_buf(NULL)
 , m_size(front + back)
 , m_size_back(back)
{
}

CCacheCircular::~CCacheCircular()
{
  Close();
}

int CCacheCircular::Open()
{
  m_buf = new uint8_t[m_size];
  m_beg = 0;
  m_end = 0;
  m_cur = 0;
  return CACHE_RC_OK;
}

void CCacheCircular::Close()
{
  delete[] m_buf;
  m_buf = NULL;
}

/**
 * Function will write to m_buf at m_end % m_size location
 * it will write at maximum m_size, but it will only write
 * as much it can without wrapping around in the buffer
 *
 * It will always leave m_size_back of the backbuffer intact
 * but if the back buffer is less than that, that space is
 * usable to write.
 *
 * If back buffer is filled to an larger extent than
 * m_size_back, it will allow it to be overwritten
 * until only m_size_back data remains.
 *
 * The following always apply:
 *  * m_end <= m_cur <= m_end
 *  * m_end - m_beg <= m_size
 *
 * Multiple calls may be needed to fill buffer completely.
 */
int CCacheCircular::WriteToCache(const char *buf, size_t len)
{
  CSingleLock lock(m_sync);

  // where are we in the buffer
  size_t pos   = m_end % m_size;
  size_t back  = (size_t)(m_cur - m_beg);
  size_t front = (size_t)(m_end - m_cur);

  size_t limit = m_size - std::min(back, m_size_back) - front;
  size_t wrap  = m_size - pos;

  // limit by max forward size
  if(len > limit)
    len = limit;

  // limit to wrap point
  if(len > wrap)
    len = wrap;

  if(len == 0)
    return 0;

  // write the data
  memcpy(m_buf + pos, buf, len);
  m_end += len;

  // drop history that was overwritten
  if(m_end - m_beg > m_size)
    m_beg = m_end - m_size;

  m_written.Set();

  return len;
}

/**
 * Reads data from cache. Will only read up till
 * the buffer wrap point. So multiple calls
 * may be needed to empty the whole cache
 */
int CCacheCircular::ReadFromCache(char *buf, size_t len)
{
  CSingleLock lock(m_sync);

  size_t pos   = m_cur % m_size;
  size_t front = (size_t)(m_end - m_cur);
  size_t avail = std::min(m_size - pos, front);

  if(avail == 0)
  {
    if(IsEndOfInput())
      return CACHE_RC_EOF;
    else
      return CACHE_RC_WOULD_BLOCK;
  }

  if(len > avail)
    len = avail;

  if(len == 0)
    return 0;

  memcpy(buf, m_buf + pos, len);
  m_cur += len;

  m_space.Set();

  return len;
}

__int64 CCacheCircular::WaitForData(unsigned int minumum, unsigned int millis)
{
  CSingleLock lock(m_sync);
  unsigned __int64 avail = m_end - m_cur;

  if(millis == 0 || IsEndOfInput())
    return avail;

  if(minumum > m_size - m_size_back)
    minumum = m_size - m_size_back;

  unsigned int time = CTimeUtils::GetTimeMS() + millis;
  while (!IsEndOfInput() && avail < minumum && CTimeUtils::GetTimeMS() < time )
  {
    lock.Leave();
    m_written.WaitMSec(50); // may miss the deadline. shouldn't be a problem.
    lock.Enter();
    avail = m_end - m_cur;
  }

  return avail;
}

__int64 CCacheCircular::Seek(__int64 pos)
{
  CSingleLock lock(m_sync);

  // if seek is a bit over what we have, try to wait a few seconds for the data to be available.
  // we try to avoid a (heavy) seek on the source
  if ((unsigned __int64)pos >= m_end && (unsigned __int64)pos < m_end + 100000)
  {
    lock.Leave();
    WaitForData((size_t)(pos - m_cur), 5000);
    lock.Enter();
  }

  if((unsigned __int64)pos >= m_beg && (unsigned __int64)pos <= m_end)
  {
    m_cur = pos;
    return pos;
  }

  return CACHE_RC_ERROR;
}

void CCacheCircular::Reset(__int64 pos)
{
  CSingleLock lock(m_sync);
  m_end = pos;
  m_beg = pos;
  m_cur = pos;
}

