/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once


/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef WORDS_BIGENDIAN
#define Endian_SwapLE16(X) (X)
#define Endian_SwapLE32(X) (X)
#else
#error Xbox 360 is big endian! Add the from SDL_endian.h
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

