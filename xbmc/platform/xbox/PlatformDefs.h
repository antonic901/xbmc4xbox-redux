#ifndef __PLATFORM_DEFS_H__
#define __PLATFORM_DEFS_H__

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

#ifdef _WIN32

#define LINE_ENDING "\r\n"

#define __STDC_FORMAT_MACROS
#include "inttypes.h"

typedef signed   __int8   int8_t;
typedef signed   __int16  int16_t;
typedef signed   __int32  int32_t;
typedef signed   __int64  int64_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
typedef unsigned __int64  uint64_t;

typedef unsigned int        uintptr_t;

typedef int ssize_t;

#ifndef PRIdS
#define PRIdS "Id"
#endif

#ifndef PRId64
#ifdef _MSC_VER
#define PRId64 "I64d"
#endif
#endif

#ifndef strcasecmp
#define strcasecmp _strcmpi
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

#ifndef va_copy
#define va_copy(dst, src) ((dst) = (src))
#endif

#define lrint(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))
#define llrint(x) ((x) >= 0 ? ((__int64)((x) + 0.5)) : ((__int64)((x) - 0.5)))
#define strtoll  _strtoi64
#define strtoull _strtoui64
#define wcstoll  _wcstoi64
#define wcstoull _wcstoui64

#define ETIMEDOUT WSAETIMEDOUT

extern "C" char * strptime(const char *buf, const char *fmt, struct tm *tm);
extern "C" int strverscmp (const char *s1, const char *s2);

/*! \brief This is nullptr implementation from C++11.
 It doesn't work for pointers from boost library. For
 that use boost built in method reset()
 */
const                         /* this is a const object...     */
class nullptr_t
{
public:
   template<class T>          /* convertible to any type       */
   operator T*() const        /* of null non-member            */
      { return 0; }           /* pointer...                    */

   template<class C, class T> /* or any type of null           */
      operator T C::*() const /* member pointer...             */
      { return 0; }

private:
   void operator&() const;    /* Can't take address of nullptr */

} nullptr = {};               /* and whose name is nullptr     */

#include <sstream>
namespace std
{
  template<typename T>
  std::string to_string(const T & value)
  {
    std::ostringstream oss;
    oss << value;
    return oss.str();
  }

  template<typename T>
  std::wstring to_wstring(const T & value)
  {
    std::wostringstream oss;
    oss << value;
    return oss.str();
  }
}

#endif // _WIN32

#endif //__PLATFORM_DEFS_H__

