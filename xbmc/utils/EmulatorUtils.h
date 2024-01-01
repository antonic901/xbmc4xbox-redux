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

#ifndef __EMULATORUTILS_H_
#define __EMULATORUTILS_H_

#include <string>

typedef struct
{
  const char* name;
  const char* shortname;
  const char* extension;
} SystemMapping;

static const SystemMapping systems[] =
  {{"Nintendo Entertainment System",         "nes",                  ".zip|.nes"},
   {"Sega Master System",                    "mastersystem",         ".zip|.sms"},
   {"Sega Megadrive",                        "megadrive",            ".zip|.md" },
   {"Super Nintendo Entertainment System",   "snes",                 ".zip|.sfc"}};

class EmulatorUtils
{
public:
  static bool GetSystemFromFilename(std::string strFilename, SystemMapping& system);
  static bool ChooseEmulatorAndLaunch(std::string strPath);
  static bool LaunchROM(std::string strRomPath, std::string strEmuPath);
};

#endif
