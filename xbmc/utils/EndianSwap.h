/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>


/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

static uint16_t Endian_Swap16(uint16_t x) {
  return((x<<8)|(x>>8));
}

static uint32_t Endian_Swap32(uint32_t x) {
  return((x<<24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x>>24));
}

static uint64_t Endian_Swap64(uint64_t x) {
  uint32_t hi, lo;

  /* Separate into high and low 32-bit values and swap them */
  lo = (uint32_t)(x&0xFFFFFFFF);
  x >>= 32;
  hi = (uint32_t)(x&0xFFFFFFFF);
  x = Endian_Swap32(lo);
  x <<= 32;
  x |= Endian_Swap32(hi);
  return(x);
}

#ifndef WORDS_BIGENDIAN
#define Endian_SwapLE16(X) (X)
#define Endian_SwapLE32(X) (X)
#define Endian_SwapBE16(X) Endian_Swap16(X)
#define Endian_SwapBE32(X) Endian_Swap32(X)
#define Endian_SwapBE64(X) Endian_Swap64(X)
#else
#error Xbox 360 is big endian! Add the from SDL_endian.h
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

