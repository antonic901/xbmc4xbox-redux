/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#define LOG_LEVEL_NONE -1 // nothing at all is logged
#define LOG_LEVEL_NORMAL 0 // shows notice, error, severe and fatal
#define LOG_LEVEL_DEBUG 1 // shows all
#define LOG_LEVEL_DEBUG_FREEMEM 2 // shows all + shows freemem on screen
#define LOG_LEVEL_MAX LOG_LEVEL_DEBUG_FREEMEM

// ones we use in the code
#define LOGDEBUG 0
#define LOGINFO 1
#define LOGWARNING 2
#define LOGERROR 3
#define LOGFATAL 4
#define LOGNONE 5

// extra masks - from bit 5
#define LOGMASKBIT 5
#define LOGMASK ((1 << LOGMASKBIT) - 1)

#define LOGSAMBA (1 << (LOGMASKBIT + 0))
#define LOGCURL (1 << (LOGMASKBIT + 1))
#define LOGFFMPEG (1 << (LOGMASKBIT + 2))
#define LOGRTMP (1 << (LOGMASKBIT + 4))
