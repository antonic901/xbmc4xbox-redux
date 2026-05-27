/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef __GNUC__
// under gcc, inline will only take place if optimizations are applied (-O). this will force inline even with optimizations.
#define XBMC_FORCE_INLINE __attribute__((always_inline))
#else
#define XBMC_FORCE_INLINE
#endif

#include <algorithm>
#include <stdexcept>
#include <vector>

template <typename T> class CPointGen
{
public:
  typedef CPointGen<T> this_type;

  CPointGen()  { x = y = static_cast<T>(0); }

  CPointGen(T a, T b)
  : x(a), y(b)
  {}

  template<class U> explicit CPointGen(const CPointGen<U>& rhs)
  : x(static_cast<T> (rhs.x)), y(static_cast<T> (rhs.y))
  {}

  this_type operator+(const this_type &point) const
  {
    return this_type(x + point.x, y + point.y);
  };

  this_type& operator+=(const this_type &point)
  {
    x += point.x;
    y += point.y;
    return *this;
  };

  this_type operator-(const this_type &point) const
  {
    return this_type(x - point.x, y - point.y);
  };

  this_type& operator-=(const this_type &point)
  {
    x -= point.x;
    y -= point.y;
    return *this;
  };

  this_type operator*(T factor) const
  {
    return this_type(x * factor, y * factor);
  }

  this_type& operator*=(T factor)
  {
    x *= factor;
    y *= factor;
    return *this;
  }

  this_type operator/(T factor) const
  {
    return this_type(x / factor, y / factor);
  }

  this_type& operator/=(T factor)
  {
    x /= factor;
    y /= factor;
    return *this;
  }

  T x, y;
};

template<typename T>
bool operator==(const CPointGen<T> &point1, const CPointGen<T> &point2)
{
  return (point1.x == point2.x && point1.y == point2.y);
}

template<typename T>
bool operator!=(const CPointGen<T> &point1, const CPointGen<T> &point2)
{
  return !(point1 == point2);
}

typedef CPointGen<float> CPoint;


template <typename T> class CRectGen
{
public:
  typedef CRectGen<T> this_type;
  typedef CPointGen<T> point_type;

  CRectGen()  { x1 = y1 = x2 = y2 = static_cast<T>(0); }

  CRectGen(T left, T top, T right, T bottom)
  : x1(left), y1(top), x2(right), y2(bottom)
  {}

  CRectGen(const point_type &p1, const point_type &p2)
  : x1(p1.x), y1(p1.y), x2(p2.x), y2(p2.y)
  {}

  template<class U> explicit CRectGen(const CRectGen<U>& rhs)
  : x1(static_cast<T> (rhs.x1)), y1(static_cast<T> (rhs.y1)), x2(static_cast<T> (rhs.x2)), y2(static_cast<T> (rhs.y2))
  {}

  void SetRect(T left, T top, T right, T bottom)
  {
    x1 = left;
    y1 = top;
    x2 = right;
    y2 = bottom;
  }

  bool PtInRect(const point_type &point) const
  {
    return (x1 <= point.x && point.x <= x2 && y1 <= point.y && point.y <= y2);
  };

  this_type& operator-=(const point_type &point) XBMC_FORCE_INLINE
  {
    x1 -= point.x;
    y1 -= point.y;
    x2 -= point.x;
    y2 -= point.y;
    return *this;
  };

  this_type operator-(const point_type &point) const
  {
    return this_type(x1 - point.x, y1 - point.y, x2 - point.x, y2 - point.y);
  }

  this_type& operator+=(const point_type &point) XBMC_FORCE_INLINE
  {
    x1 += point.x;
    y1 += point.y;
    x2 += point.x;
    y2 += point.y;
    return *this;
  };

  this_type operator+(const point_type &point) const
  {
    return this_type(x1 + point.x, y1 + point.y, x2 + point.x, y2 + point.y);
  }

  this_type& Intersect(const this_type &rect)
  {
    x1 = clamp_range(x1, rect.x1, rect.x2);
    x2 = clamp_range(x2, rect.x1, rect.x2);
    y1 = clamp_range(y1, rect.y1, rect.y2);
    y2 = clamp_range(y2, rect.y1, rect.y2);
    return *this;
  };

  this_type& Union(const this_type &rect)
  {
    if (IsEmpty())
      *this = rect;
    else if (!rect.IsEmpty())
    {
      x1 = std::min(x1,rect.x1);
      y1 = std::min(y1,rect.y1);

      x2 = std::max(x2,rect.x2);
      y2 = std::max(y2,rect.y2);
    }

    return *this;
  };

  bool IsEmpty() const XBMC_FORCE_INLINE
  {
    return (x2 - x1) * (y2 - y1) == 0;
  };

  T Width() const XBMC_FORCE_INLINE
  {
    return x2 - x1;
  };

  T Height() const XBMC_FORCE_INLINE
  {
    return y2 - y1;
  };

  T Area() const XBMC_FORCE_INLINE
  {
    return Width() * Height();
  };

  T x1, y1, x2, y2;
private:
  static T clamp_range(T x, T l, T h) XBMC_FORCE_INLINE
  {
    return (x > h) ? h : ((x < l) ? l : x);
  }
};

template<typename T>
bool operator==(const CRectGen<T> &rect1, const CRectGen<T> &rect2)
{
  return (rect1.x1 == rect2.x1 && rect1.y1 == rect2.y1 && rect1.x2 == rect2.x2 && rect1.y2 == rect2.y2);
}

template<typename T>
bool operator!=(const CRectGen<T> &rect1, const CRectGen<T> &rect2)
{
  return !(rect1 == rect2);
}

typedef CRectGen<float> CRect;
