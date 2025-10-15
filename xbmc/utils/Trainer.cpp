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

// trainer loading generously donated by team xored - thanks!

#include "Trainer.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "filesystem/RarManager.h"
#include "dialogs/GUIDialogProgress.h"
#include "programs/ProgramDatabase.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "xbox/Undocumented.h"

#include "defs_from_settings.h"

using namespace XFILE;

// header offsets etm
#define ETM_SELECTIONS_OFFSET 0x0A // option states (0 or 1)
#define ETM_SELECTIONS_TEXT_OFFSET 0x0E // option labels
#define ETM_ID_LIST 0x12 // TitleID(s) trainer is meant for
#define ETM_ENTRYPOINT 0x16 // entry point for etm file (really just com file)

// header offsets xbtf
#define XBTF_SELECTIONS_OFFSET 0x0A // option states (0 or 1)
#define XBTF_SELECTIONS_TEXT_OFFSET 0x0E // option labels
#define XBTF_ID_LIST 0x12 // TitleID(s) trainer is meant for
#define XBTF_SECTION 0x16 // section to patch the locations in memory our xbtf support functions end up
#define XBTF_ENTRYPOINT 0x1A // entry point for xbtf file (really com).

#define ETM_HEAP_SIZE 2400  // just enough room to match evox's etm spec limit (no need to give them more room then evox does)
#define XBTF_HEAP_SIZE 15360 // plenty of room for trainer + xbtf support functions

#define KERNEL_STORE_ADDRESS 0x8000000C // this is address in kernel we store the address of our allocated memory block
#define KERNEL_START_ADDRESS 0x80010000 // base addy of kernel
#define KERNEL_ALLOCATE_ADDRESS 0x7FFD2200 // where we want to put our allocated memory block (under kernel so it works retail)
#define KERNEL_SEARCH_RANGE 0x02AF90 // used for loop control base + search range to look xbe entry point bytes

// magic kernel patch (asm included w/ source)
static unsigned char trainerloaderdata[167] =
{
       0x60, 0xBA, 0x34, 0x12, 0x00, 0x00, 0x60, 0x6A, 0x01, 0x6A, 0x07, 0xE8, 0x67, 0x00, 0x00, 0x00,
       0x6A, 0x0C, 0x6A, 0x08, 0xE8, 0x5E, 0x00, 0x00, 0x00, 0x61, 0x8B, 0x35, 0x18, 0x01, 0x01, 0x00,
       0x83, 0xC6, 0x08, 0x8B, 0x06, 0x8B, 0x72, 0x12, 0x03, 0xF2, 0xB9, 0x03, 0x00, 0x00, 0x00, 0x3B,
       0x06, 0x74, 0x0C, 0x83, 0xC6, 0x04, 0xE2, 0xF7, 0x68, 0xF0, 0x00, 0x00, 0x00, 0xEB, 0x29, 0x8B,
       0xEA, 0x83, 0x7A, 0x1A, 0x00, 0x74, 0x05, 0x8B, 0x4A, 0x1A, 0xEB, 0x03, 0x8B, 0x4A, 0x16, 0x03,
       0xCA, 0x0F, 0x20, 0xC0, 0x50, 0x25, 0xFF, 0xFF, 0xFE, 0xFF, 0x0F, 0x22, 0xC0, 0xFF, 0xD1, 0x58,
       0x0F, 0x22, 0xC0, 0x68, 0xFF, 0x00, 0x00, 0x00, 0x6A, 0x08, 0xE8, 0x08, 0x00, 0x00, 0x00, 0x61,
       0xFF, 0x15, 0x28, 0x01, 0x01, 0x00, 0xC3, 0x55, 0x8B, 0xEC, 0x66, 0xBA, 0x04, 0xC0, 0xB0, 0x20,
       0xEE, 0x66, 0xBA, 0x08, 0xC0, 0x8A, 0x45, 0x08, 0xEE, 0x66, 0xBA, 0x06, 0xC0, 0x8A, 0x45, 0x0C,
       0xEE, 0xEE, 0x66, 0xBA, 0x02, 0xC0, 0xB0, 0x1A, 0xEE, 0x50, 0xB8, 0x40, 0x42, 0x0F, 0x00, 0x48,
       0x75, 0xFD, 0x58, 0xC9, 0xC2, 0x08, 0x00,
};

#define SIZEOFLOADERDATA 167// loaderdata is our kernel hack to handle if trainer (com file) is executed for title about to run

static unsigned char trainerdata[XBTF_HEAP_SIZE] = { NULL }; // buffer to hold trainer in mem - needs to be global?
// SM_Bus function for xbtf trainers
static unsigned char sm_bus[48] =
{
  0x55, 0x8B, 0xEC, 0x66, 0xBA, 0x04, 0xC0, 0xB0, 0x20, 0xEE, 0x66, 0xBA, 0x08, 0xC0, 0x8A, 0x45,
  0x08, 0xEE, 0x66, 0xBA, 0x06, 0xC0, 0x8A, 0x45, 0x0C, 0xEE, 0xEE, 0x66, 0xBA, 0x02, 0xC0, 0xB0,
  0x1A, 0xEE, 0x50, 0xB8, 0x40, 0x42, 0x0F, 0x00, 0x48, 0x75, 0xFD, 0x58, 0xC9, 0xC2, 0x08, 0x00,
};

// PatchIt dynamic patching
static unsigned char patch_it_toy[33] =
{
  0x55, 0x8B, 0xEC, 0x60, 0x0F, 0x20, 0xC0, 0x50, 0x25, 0xFF, 0xFF, 0xFE, 0xFF, 0x0F, 0x22, 0xC0,
  0x8B, 0x4D, 0x0C, 0x8B, 0x55, 0x08, 0x89, 0x0A, 0x58, 0x0F, 0x22, 0xC0, 0x61, 0xC9, 0xC2, 0x08,
  0x00,
};

// HookIt
static unsigned char hookit_toy[43] =
{
  0x55, 0x8B, 0xEC, 0x60, 0x8B, 0x55, 0x08, 0x8B, 0x45, 0x0C, 0xBD, 0x0C, 0x00, 0x00, 0x80, 0x8B,
  0x6D, 0x00, 0x83, 0xC5, 0x02, 0x8B, 0x6D, 0x00, 0x8D, 0x44, 0x05, 0x00, 0xC6, 0x02, 0x68, 0x89,
  0x42, 0x01, 0xC6, 0x42, 0x05, 0xC3, 0x61, 0xC9, 0xC2, 0x08, 0x00,
};

// in game keys (main)
static unsigned char igk_main_toy[403] =
{
  0x81, 0x3D, 0x4C, 0x01, 0x01, 0x00, 0xA4, 0x01, 0x00, 0x00, 0x74, 0x30, 0xC7, 0x05, 0x4C, 0x01,
  0x01, 0x00, 0xA4, 0x01, 0x00, 0x00, 0xC7, 0x05, 0x14, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
  0x60, 0xBA, 0x50, 0x01, 0x01, 0x00, 0xB9, 0x05, 0x00, 0x00, 0x00, 0xC7, 0x02, 0x00, 0x00, 0x00,
  0x00, 0x83, 0xC2, 0x04, 0xE2, 0xF5, 0x61, 0xE9, 0x4B, 0x01, 0x00, 0x00, 0x51, 0x8D, 0x4A, 0x08,
  0x60, 0x33, 0xC0, 0x8A, 0x41, 0x0C, 0x50, 0x8D, 0x15, 0x14, 0x00, 0x00, 0x80, 0x8B, 0x0A, 0x89,
  0x42, 0x04, 0x3B, 0xC1, 0x0F, 0x84, 0x1F, 0x01, 0x00, 0x00, 0x66, 0x25, 0x81, 0x00, 0x66, 0x3D,
  0x81, 0x00, 0x75, 0x0C, 0x80, 0x35, 0x51, 0x01, 0x01, 0x00, 0x01, 0xE9, 0x09, 0x01, 0x00, 0x00,
  0x58, 0x50, 0x66, 0x25, 0x82, 0x00, 0x66, 0x3D, 0x82, 0x00, 0x75, 0x0C, 0x80, 0x35, 0x52, 0x01,
  0x01, 0x00, 0x01, 0xE9, 0xF1, 0x00, 0x00, 0x00, 0x58, 0x50, 0x66, 0x25, 0x84, 0x00, 0x66, 0x3D,
  0x84, 0x00, 0x75, 0x0C, 0x80, 0x35, 0x53, 0x01, 0x01, 0x00, 0x01, 0xE9, 0xD9, 0x00, 0x00, 0x00,
  0x58, 0x50, 0x66, 0x25, 0x88, 0x00, 0x66, 0x3D, 0x88, 0x00, 0x75, 0x0C, 0x80, 0x35, 0x54, 0x01,
  0x01, 0x00, 0x01, 0xE9, 0xC1, 0x00, 0x00, 0x00, 0x58, 0x50, 0x66, 0x83, 0xE0, 0x41, 0x66, 0x83,
  0xF8, 0x41, 0x75, 0x0C, 0x80, 0x35, 0x55, 0x01, 0x01, 0x00, 0x01, 0xE9, 0xA9, 0x00, 0x00, 0x00,
  0x58, 0x50, 0x66, 0x83, 0xE0, 0x42, 0x66, 0x83, 0xF8, 0x42, 0x75, 0x0C, 0x80, 0x35, 0x56, 0x01,
  0x01, 0x00, 0x01, 0xE9, 0x91, 0x00, 0x00, 0x00, 0x58, 0x50, 0x66, 0x83, 0xE0, 0x44, 0x66, 0x83,
  0xF8, 0x44, 0x75, 0x09, 0x80, 0x35, 0x57, 0x01, 0x01, 0x00, 0x01, 0xEB, 0x7C, 0x58, 0x50, 0x66,
  0x83, 0xE0, 0x48, 0x66, 0x83, 0xF8, 0x48, 0x75, 0x09, 0x80, 0x35, 0x58, 0x01, 0x01, 0x00, 0x01,
  0xEB, 0x67, 0x58, 0x50, 0x66, 0x25, 0xC0, 0x00, 0x66, 0x3D, 0xC0, 0x00, 0x75, 0x09, 0x80, 0x35,
  0x59, 0x01, 0x01, 0x00, 0x01, 0xEB, 0x52, 0x58, 0x50, 0x66, 0x83, 0xE0, 0x60, 0x66, 0x83, 0xF8,
  0x60, 0x75, 0x09, 0x80, 0x35, 0x5A, 0x01, 0x01, 0x00, 0x01, 0xEB, 0x3D, 0x58, 0x50, 0x66, 0x83,
  0xE0, 0x50, 0x66, 0x83, 0xF8, 0x50, 0x75, 0x09, 0x80, 0x35, 0x5B, 0x01, 0x01, 0x00, 0x01, 0xEB,
  0x28, 0x58, 0x50, 0x66, 0x25, 0xA0, 0x00, 0x66, 0x3D, 0xA0, 0x00, 0x75, 0x09, 0x80, 0x35, 0x5C,
  0x01, 0x01, 0x00, 0x01, 0xEB, 0x13, 0x58, 0x50, 0x66, 0x25, 0x90, 0x00, 0x66, 0x3D, 0x90, 0x00,
  0x75, 0x07, 0x80, 0x35, 0x5D, 0x01, 0x01, 0x00, 0x01, 0x58, 0x8D, 0x15, 0x14, 0x00, 0x00, 0x80,
  0x8B, 0x42, 0x04, 0x89, 0x02, 0x61, 0x59, 0x5B, 0xB9, 0x10, 0x00, 0x00, 0x80, 0xFF, 0x11, 0x53,
  0x33, 0xDB, 0xC3,
};

// HOOKIGK (moves stock pad call and patches game igk_main to use correct register)
static unsigned char hook_igk_toy[76] =
{
  0x55, 0x8B, 0xEC, 0x60, 0xBA, 0x34, 0x12, 0x00, 0x00, 0x8B, 0x45, 0x08, 0xB9, 0x20, 0x00, 0x00,
  0x00, 0x8A, 0x18, 0x80, 0xFB, 0x50, 0x7C, 0x07, 0x80, 0xFB, 0x53, 0x7F, 0x02, 0xEB, 0x05, 0x48,
  0xE2, 0xEF, 0xEB, 0x23, 0x80, 0xEB, 0x08, 0x88, 0x5A, 0x3E, 0x83, 0x45, 0x08, 0x01, 0x8B, 0x45,
  0x08, 0x50, 0x03, 0x00, 0x83, 0xC0, 0x04, 0x2B, 0x55, 0x08, 0x83, 0xEA, 0x04, 0xBB, 0x10, 0x00,
  0x00, 0x80, 0x89, 0x03, 0x58, 0x89, 0x10, 0x61, 0xC9, 0xC2, 0x04, 0x00,
};

// SmartXX / Aladin4 functions
static unsigned char lcd_toy_xx[378] =
{
  0x55, 0x8B, 0xEC, 0x90, 0x66, 0x8B, 0x55, 0x08, 0x90, 0x8A, 0x45, 0x0C, 0x90, 0xEE, 0x90, 0x90,
  0xC9, 0xC2, 0x08, 0x00, 0x55, 0x8B, 0xEC, 0x60, 0x33, 0xC0, 0x33, 0xDB, 0x0F, 0xB6, 0x45, 0x08,
  0xC1, 0xF8, 0x02, 0x83, 0xE0, 0x28, 0xA2, 0x40, 0x01, 0x01, 0x00, 0x0F, 0xB6, 0x45, 0x08, 0x83,
  0xE0, 0x50, 0x0F, 0xB6, 0x0D, 0x40, 0x01, 0x01, 0x00, 0x0B, 0xC8, 0x88, 0x0D, 0x40, 0x01, 0x01,
  0x00, 0x0F, 0xB6, 0x45, 0x08, 0xC1, 0xE0, 0x02, 0x83, 0xE0, 0x28, 0xA2, 0x41, 0x01, 0x01, 0x00,
  0x0F, 0xB6, 0x45, 0x08, 0xC1, 0xE0, 0x04, 0x83, 0xE0, 0x50, 0x0F, 0xB6, 0x0D, 0x41, 0x01, 0x01,
  0x00, 0x0B, 0xC8, 0x88, 0x0D, 0x41, 0x01, 0x01, 0x00, 0x0F, 0xB6, 0x45, 0x0C, 0x83, 0xE0, 0x02,
  0x0F, 0xB6, 0x0D, 0x40, 0x01, 0x01, 0x00, 0x0B, 0xC1, 0x50, 0x68, 0x00, 0xF7, 0x00, 0x00, 0xE8,
  0x7C, 0xFF, 0xFF, 0xFF, 0x0F, 0xB6, 0x45, 0x0C, 0x83, 0xE0, 0x02, 0x83, 0xC8, 0x04, 0x0F, 0xB6,
  0x0D, 0x40, 0x01, 0x01, 0x00, 0x0B, 0xC1, 0x50, 0x68, 0x00, 0xF7, 0x00, 0x00, 0xE8, 0x5E, 0xFF,
  0xFF, 0xFF, 0x0F, 0xB6, 0x45, 0x0C, 0x83, 0xE0, 0x02, 0x0F, 0xB6, 0x0D, 0x40, 0x01, 0x01, 0x00,
  0x0B, 0xC1, 0x50, 0x68, 0x00, 0xF7, 0x00, 0x00, 0xE8, 0x43, 0xFF, 0xFF, 0xFF, 0x0F, 0xB6, 0x45,
  0x0C, 0x83, 0xE0, 0x01, 0x75, 0x6A, 0x0F, 0xB6, 0x45, 0x0C, 0x83, 0xE0, 0x02, 0x0F, 0xB6, 0x0D,
  0x41, 0x01, 0x01, 0x00, 0x0B, 0xC1, 0x50, 0x68, 0x00, 0xF7, 0x00, 0x00, 0xE8, 0x1F, 0xFF, 0xFF,
  0xFF, 0x0F, 0xB6, 0x45, 0x0C, 0x83, 0xE0, 0x02, 0x83, 0xC8, 0x04, 0x0F, 0xB6, 0x0D, 0x41, 0x01,
  0x01, 0x00, 0x0B, 0xC1, 0x50, 0x68, 0x00, 0xF7, 0x00, 0x00, 0xE8, 0x01, 0xFF, 0xFF, 0xFF, 0x0F,
  0xB6, 0x45, 0x0C, 0x83, 0xE0, 0x02, 0x0F, 0xB6, 0x0D, 0x41, 0x01, 0x01, 0x00, 0x0B, 0xC1, 0x50,
  0x68, 0x00, 0xF7, 0x00, 0x00, 0xE8, 0xE6, 0xFE, 0xFF, 0xFF, 0x0F, 0xB6, 0x45, 0x08, 0x25, 0xFC,
  0x00, 0x00, 0x00, 0x75, 0x0B, 0xF4, 0x90, 0x90, 0x90, 0x90, 0x90, 0xF4, 0x90, 0x90, 0x90, 0x90,
  0xF4, 0x90, 0x90, 0x90, 0x90, 0x90, 0xF4, 0x90, 0x90, 0x90, 0x90, 0x90, 0x61, 0xC9, 0xC2, 0x08,
  0x00, 0x6A, 0x00, 0x6A, 0x01, 0xE8, 0xCA, 0xFE, 0xFF, 0xFF, 0xC3, 0x55, 0x8B, 0xEC, 0x60, 0xB1,
  0x80, 0x0A, 0x4D, 0x0C, 0x6A, 0x00, 0x8A, 0xC1, 0x50, 0xE8, 0xB6, 0xFE, 0xFF, 0xFF, 0x33, 0xC9,
  0x8B, 0x55, 0x08, 0x8A, 0x04, 0x11, 0x3C, 0x00, 0x74, 0x0B, 0x6A, 0x02, 0x50, 0xE8, 0xA2, 0xFE,
  0xFF, 0xFF, 0x41, 0xEB, 0xEE, 0x61, 0xC9, 0xC2, 0x08, 0x00,
};

// X3 LCD functions
static unsigned char lcd_toy_x3[246] =
{
  0x55, 0x8B, 0xEC, 0x90, 0x66, 0x8B, 0x55, 0x08, 0x90, 0x8A, 0x45, 0x0C, 0x90, 0xEE, 0x90, 0x90,
  0xC9, 0xC2, 0x08, 0x00, 0x55, 0x8B, 0xEC, 0x60, 0x50, 0x33, 0xC0, 0x8A, 0x45, 0x0C, 0x24, 0x02,
  0x3C, 0x00, 0x74, 0x05, 0x32, 0xE4, 0x80, 0xCC, 0x01, 0x8A, 0x45, 0x08, 0x24, 0xF0, 0x50, 0x68,
  0x04, 0xF5, 0x00, 0x00, 0xE8, 0xC7, 0xFF, 0xFF, 0xFF, 0x8A, 0xC4, 0x50, 0x68, 0x05, 0xF5, 0x00,
  0x00, 0xE8, 0xBA, 0xFF, 0xFF, 0xFF, 0x50, 0x80, 0xCC, 0x04, 0x8A, 0xC4, 0x50, 0x68, 0x05, 0xF5,
  0x00, 0x00, 0xE8, 0xA9, 0xFF, 0xFF, 0xFF, 0x58, 0x8A, 0xC4, 0x50, 0x68, 0x05, 0xF5, 0x00, 0x00,
  0xE8, 0x9B, 0xFF, 0xFF, 0xFF, 0x8A, 0x45, 0x0C, 0x24, 0x01, 0x3C, 0x00, 0x75, 0x3D, 0x8A, 0x45,
  0x08, 0xC0, 0xE0, 0x04, 0x50, 0x68, 0x04, 0xF5, 0x00, 0x00, 0xE8, 0x81, 0xFF, 0xFF, 0xFF, 0x8A,
  0xC4, 0x50, 0x68, 0x05, 0xF5, 0x00, 0x00, 0xE8, 0x74, 0xFF, 0xFF, 0xFF, 0x50, 0x80, 0xCC, 0x04,
  0x8A, 0xC4, 0x50, 0x68, 0x05, 0xF5, 0x00, 0x00, 0xE8, 0x63, 0xFF, 0xFF, 0xFF, 0x58, 0x8A, 0xC4,
  0x50, 0x68, 0x05, 0xF5, 0x00, 0x00, 0xE8, 0x55, 0xFF, 0xFF, 0xFF, 0x58, 0xF4, 0x90, 0x90, 0x90,
  0x90, 0x90, 0xF4, 0x90, 0x90, 0x90, 0x90, 0x90, 0x61, 0xC9, 0xC2, 0x08, 0x00, 0x6A, 0x00, 0x6A,
  0x01, 0xE8, 0x4E, 0xFF, 0xFF, 0xFF, 0xC3, 0x55, 0x8B, 0xEC, 0x60, 0xB1, 0x80, 0x0A, 0x4D, 0x0C,
  0x6A, 0x00, 0x8A, 0xC1, 0x50, 0xE8, 0x3A, 0xFF, 0xFF, 0xFF, 0x33, 0xC9, 0x8B, 0x55, 0x08, 0x8A,
  0x04, 0x11, 0x3C, 0x00, 0x74, 0x0B, 0x6A, 0x02, 0x50, 0xE8, 0x26, 0xFF, 0xFF, 0xFF, 0x41, 0xEB,
  0xEE, 0x61, 0xC9, 0xC2, 0x08, 0x00,
};

CTrainer::CTrainer(int idTrainer /* = 0 */)
{
  m_pData = m_pTrainerData = NULL;
  m_iSize = 0;
  m_bIsXBTF = false;
  m_idTrainer = idTrainer;
}

CTrainer::~CTrainer()
{
  if (m_pData)
    delete[] m_pData;
  m_pData = NULL;
  m_pTrainerData = NULL;
  m_iSize = 0;
  m_idTrainer = 0;
}

bool CTrainer::InstallTrainer(CTrainer& trainer)
{
  bool Found = false;
#ifdef HAS_XBOX_HARDWARE
  unsigned char *xboxkrnl = (unsigned char *)KERNEL_START_ADDRESS;
  unsigned char *hackptr = (unsigned char *)KERNEL_STORE_ADDRESS;
  void *ourmemaddr = NULL; // pointer used to allocated trainer mem
  unsigned int i = 0;
  DWORD memsize;

  CLog::Log(LOGDEBUG,"installing trainer %s",trainer.GetPath().c_str());

  if (trainer.IsXBTF()) // size of our allocation buffer for trainer
    memsize = XBTF_HEAP_SIZE;
  else
    memsize = ETM_HEAP_SIZE;

  unsigned char xbe_entry_point[] = {0xff,0x15,0x28,0x01,0x01,0x00}; // xbe entry point bytes in kernel
  unsigned char evox_tsr_hook[] = {0xff,0x15,0x10,0x00,0x00,0x80}; // check for evox's evil tsr hook

  for(i = 0; i < KERNEL_SEARCH_RANGE; i++)
  {
    if (memcmp(&xboxkrnl[i], xbe_entry_point, sizeof(xbe_entry_point)) == 0 ||
      memcmp(&xboxkrnl[i], evox_tsr_hook, sizeof(evox_tsr_hook)) == 0)
    {
      Found = true;
      break;
    }
  }

  if(Found)
  {
    unsigned char *patchlocation = xboxkrnl;

    patchlocation += i + 2; // adjust to xbe entry point bytes in kernel (skipping actual call opcodes)
    _asm
    {
      pushad

      mov eax, cr0
      push eax
      and eax, 0FFFEFFFFh
      mov cr0, eax // disable memory write prot

      mov	edi, patchlocation // address of call to xbe entry point in kernel
      mov	dword ptr [edi], KERNEL_STORE_ADDRESS // patch with address of where we store loaderdata+trainer buffer address

      pop eax
      mov cr0, eax // restore memory write prot

      popad
    }
  }
  else
  {
    __asm // recycle check
    {
      pushad

      mov edx, KERNEL_STORE_ADDRESS
      mov ecx, DWORD ptr [edx]
      cmp ecx, 0 // just in case :)
      jz cleanup

      cmp word ptr [ecx], 0BA60h // address point to valid loaderdata?
      jnz cleanup

      mov Found, 1 // yes! flag it found

      push ecx
      call MmFreeContiguousMemory // release old memory
cleanup:
      popad
    }
  }

  // allocate our memory space BELOW the kernel (so we can access buffer from game's scope)
  // if you allocate above kernel our buffer is out of scope and only debug bio will allow
  // game to access it
  ourmemaddr = MmAllocateContiguousMemoryEx(memsize, 0, -1, KERNEL_ALLOCATE_ADDRESS, PAGE_NOCACHE | PAGE_READWRITE);
  if ((DWORD)ourmemaddr > 0)
  {
    MmPersistContiguousMemory(ourmemaddr, memsize, true); // so we survive soft boots
    memcpy(hackptr, &ourmemaddr, 4); // store location of ourmemaddr in kernel

    memset(ourmemaddr, 0xFF, memsize); // init trainer buffer
    memcpy(ourmemaddr, trainerloaderdata, sizeof(trainerloaderdata)); // copy loader data (actual kernel hack)

    // patch loaderdata with trainer base address
    _asm
    {
      pushad

      mov eax, ourmemaddr
      mov ebx, eax
      add ebx, SIZEOFLOADERDATA
      mov dword ptr [eax+2], ebx

      popad
    }

    // adjust ourmemaddr pointer past loaderdata
    ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(trainerloaderdata));

    // copy our trainer data into allocated mem
    memcpy(ourmemaddr, trainer.data(), trainer.Size());

    if (trainer.IsXBTF())
    {
      DWORD dwSection = 0;

      // get address of XBTF_Section
      _asm
      {
        pushad

        mov eax, ourmemaddr

        cmp dword ptr [eax+0x1A], 0 // real xbtf or just a converted etm? - XBTF_ENTRYPOINT
        je converted_etm

        push eax
        mov ebx, 0x16
        add eax, ebx
        mov ecx, DWORD PTR [eax]
        pop	eax
        add eax, ecx
        mov dwSection, eax // get address of xbtf_section

      converted_etm:
        popad
      }

      if (dwSection == 0)
        return Found; // its a converted etm so we do not have toys section :)

      // adjust past trainer
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + trainer.Size());

      // inject SMBus code
      memcpy(ourmemaddr, sm_bus, sizeof(sm_bus));
      _asm
      {
        pushad

        mov eax, dwSection
        mov ebx, ourmemaddr
        cmp dword ptr [eax], 0
        jne nosmbus
        mov DWORD PTR [eax], ebx
      nosmbus:
        popad
      }
      // adjust past SMBus
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(sm_bus));

      // PatchIt
      memcpy(ourmemaddr, patch_it_toy, sizeof(patch_it_toy));
      _asm
      {
        pushad

        mov eax, dwSection
        add eax, 4 // 2nd dword in XBTF_Section
        mov ebx, ourmemaddr
        cmp dword PTR [eax], 0
        jne nopatchit
        mov DWORD PTR [eax], ebx
      nopatchit:
        popad
      }

      // adjust past PatchIt
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(patch_it_toy));

      // HookIt
      memcpy(ourmemaddr, hookit_toy, sizeof(hookit_toy));
      _asm
      {
        pushad

        mov eax, dwSection
        add eax, 8 // 3rd dword in XBTF_Section
        mov ebx, ourmemaddr
        cmp dword PTR [eax], 0
        jne nohookit
        mov DWORD PTR [eax], ebx
      nohookit:
        popad
      }

      // adjust past HookIt
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(hookit_toy));

      // igk_main_toy
      memcpy(ourmemaddr, igk_main_toy, sizeof(igk_main_toy));
      _asm
      {
        // patch hook_igk_toy w/ address
        pushad

        mov edx, offset hook_igk_toy
        add edx, 5
        mov ecx, ourmemaddr
        mov dword PTR [edx], ecx

        popad
      }

      // adjust past igk_main_toy
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(igk_main_toy));

      // hook_igk_toy
      memcpy(ourmemaddr, hook_igk_toy, sizeof(hook_igk_toy));
      _asm
      {
        pushad

        mov eax, dwSection
        add eax, 0ch // 4th dword in XBTF_Section
        mov ebx, ourmemaddr
        cmp dword PTR [eax], 0
        jne nohookigk
        mov DWORD PTR [eax], ebx
      nohookigk:
        popad
      }
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(igk_main_toy));

      if (CSettings::GetInstance().GetInt("lcd.mode") > 0 && CSettings::GetInstance().GetInt("lcd.type") == MODCHIP_SMARTXX)
      {
        memcpy(ourmemaddr, lcd_toy_xx, sizeof(lcd_toy_xx));
        _asm
        {
          pushad

          mov ecx, ourmemaddr
          add ecx, 0141h // lcd clear

          mov eax, dwSection
          add eax, 010h // 5th dword

          cmp dword PTR [eax], 0
          jne nolcdxx

          mov dword PTR [eax], ecx
          add ecx, 0ah // lcd writestring
          add eax, 4 // 6th dword

          cmp dword ptr [eax], 0
          jne nolcd

          mov dword ptr [eax], ecx
        nolcdxx:
          popad
        }
        ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(lcd_toy_xx));
      }
      else
      {
        // lcd toy
        memcpy(ourmemaddr, lcd_toy_x3, sizeof(lcd_toy_x3));
        _asm
        {
          pushad

          mov ecx, ourmemaddr
          add ecx, 0bdh // lcd clear

          mov eax, dwSection
          add eax, 010h // 5th dword

          cmp dword PTR [eax], 0
          jne nolcd

          mov dword PTR [eax], ecx
          add ecx, 0ah // lcd writestring
          add eax, 4 // 6th dword

          cmp dword ptr [eax], 0
          jne nolcd

          mov dword ptr [eax], ecx
        nolcd:
          popad
        }
        ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(lcd_toy_x3));
      }
    }
  }
#endif

  return Found;
}

bool CTrainer::RemoveTrainer()
{
  bool Found = false;
#ifdef HAS_XBOX_HARDWARE
  unsigned char *xboxkrnl = (unsigned char *)KERNEL_START_ADDRESS;
  unsigned int i = 0;

  unsigned char xbe_entry_point[] = {0xff,0x15,0x80,0x00,0x00,0x0c}; // xbe entry point bytes in kernel
  *((DWORD*)(xbe_entry_point+2)) = KERNEL_STORE_ADDRESS;

  for(i = 0; i < KERNEL_SEARCH_RANGE; i++)
  {
    if (memcmp(&xboxkrnl[i], xbe_entry_point, 6) == 0)
    {
      Found = true;
      break;
    }
  }

  if(Found)
  {
    unsigned char *patchlocation = xboxkrnl;
    patchlocation += i + 2; // adjust to xbe entry point bytes in kernel (skipping actual call opcodes)
    __asm // recycle check
    {
        pushad

        mov eax, cr0
        push eax
        and eax, 0FFFEFFFFh
        mov cr0, eax // disable memory write prot

        mov	edi, patchlocation // address of call to xbe entry point in kernel
        mov	dword ptr [edi], 0x00010128 // patch with address of where we store loaderdata+trainer buffer address

        pop eax
        mov cr0, eax // restore memory write prot

        popad
    }
  }
#endif
  return Found;
}

bool CTrainer::Load(const std::string& strPath)
{
  CFile file;
  if (!file.Open(strPath))
    return false;

  if (URIUtils::HasExtension(strPath, ".xbtf"))
    m_bIsXBTF = true;
  else
    m_bIsXBTF = false;

  m_iSize = (unsigned int)file.GetLength();
  if (m_iSize < ETM_SELECTIONS_OFFSET)
  {
    CLog::Log(LOGINFO,"CTrainer::Load: Broken trainer %s",strPath.c_str());
    return false;
  }
  m_pData = new unsigned char[(unsigned int)file.GetLength()+1];
  m_pData[file.GetLength()] = '\0'; // to make sure strlen doesn't crash
  file.Read(m_pData,m_iSize);
  file.Close();

  unsigned int iTextOffset;
  if (m_bIsXBTF)
  {
    void* buffer = m_pData;
    unsigned int trainerbytesread = m_iSize;

    __asm // unmangle trainer
    {
      pushad

      mov esi, buffer
      xor eax, eax
      add al, byte ptr [esi+027h]
      add al, byte ptr [esi+02Fh]
      add al, byte ptr [esi+037h]
      mov	ecx, 0FFFFFFh
      imul ecx
      xor dword ptr [esi], eax
      mov ebx, dword ptr [esi]
      add esi, 4
      xor eax, eax
      mov ecx, trainerbytesread
      sub ecx, 4
    loopme:
      xor byte ptr [esi], bl
      sub byte ptr [esi], al
      add eax, 3
      add eax, ecx
      inc esi
      loop loopme

      popad
    }

    strncpy(m_szCreationKey,(char*)(m_pData+4),200);
    unsigned int iKeyLength = strlen(m_szCreationKey)+1;
    if (m_szCreationKey[6] != '-')
    {
      CLog::Log(LOGERROR,"CTrainer::Load: Broken trainer %s",strPath.c_str());
      return false;
    }

    m_pTrainerData = m_pData+4+iKeyLength;
    unsigned int iTextLength = strlen((char*)m_pTrainerData)+1;
    // read scroller text here if interested
    m_pTrainerData += iTextLength;
    m_iSize -= 4+iKeyLength+iTextLength;
    iTextOffset = *((unsigned int*)(m_pTrainerData+XBTF_SELECTIONS_TEXT_OFFSET));
    m_iOptions = *((unsigned int*)(m_pTrainerData+XBTF_SELECTIONS_OFFSET));
  }
  else
  {
    iTextOffset = *((unsigned int*)(m_pData+ETM_SELECTIONS_TEXT_OFFSET));
    m_iOptions = *((unsigned int*)(m_pData+ETM_SELECTIONS_OFFSET));
    m_pTrainerData = m_pData;
  }

  if (iTextOffset > m_iSize)
  {
    CLog::Log(LOGINFO,"CTrainer::Load: Broken trainer %s",strPath.c_str());
    return false;
  }

  m_iNumOptions = iTextOffset-m_iOptions;

  char temp[85];
  unsigned int i;
  for (i=0;i<m_iNumOptions+2;++i)
  {
    unsigned int iOffset;
    memcpy(&iOffset,m_pTrainerData+iTextOffset+4*i,4);
    if (!iOffset)
      break;

    if (iOffset > m_iSize || iTextOffset+4*i > m_iSize)
    {
      CLog::Log(LOGINFO,"CTrainer::Load: Broken trainer %s",strPath.c_str());
      return false;
    }
    strcpy(temp,(char*)(m_pTrainerData+iOffset));
    m_vecText.push_back(temp);
  }

  m_iNumOptions = i-1;

  m_strPath = strPath;
  return true;
}

void CTrainer::GetTitleIds(unsigned int& title1, unsigned int& title2, unsigned int& title3) const
{
  if (m_pData)
  {
    DWORD ID_List;
    unsigned char* pList = m_pTrainerData+(m_bIsXBTF?XBTF_ID_LIST:ETM_ID_LIST);
    memcpy(&ID_List, pList, 4);
    memcpy(&title1,m_pTrainerData+ID_List,4);
    memcpy(&title2,m_pTrainerData+ID_List+4,4);
    memcpy(&title3,m_pTrainerData+ID_List+8,4);
  }
  else
    title1 = title2 = title3 = 0;
}

void CTrainer::GetOptionLabels(std::vector<std::string>& vecOptionLabels) const
{
  vecOptionLabels.clear();
  for (int i=0;i<int(m_vecText.size())-2;++i)
  {
    vecOptionLabels.push_back(m_vecText[i+2]);
  }
}

void CTrainer::SetOptions(unsigned char* options)
{
  memcpy(m_pTrainerData+m_iOptions,options,100);
}

bool CTrainer::ScanTrainers()
{
  CGUIDialogProgress* progress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!progress)
    return false;

  CProgramDatabase database;
  if (!database.Open())
    return false;

  std::string strTrainersPath = CSettings::GetInstance().GetString("myprograms.trainerpath");
  if (strTrainersPath.empty())
    return false;

  CLog::Log(LOGDEBUG, "Looking for trainers inside: %s", strTrainersPath.c_str());

  // first, remove any dead items
  CFileItemList items;
  database.GetTrainers(items);

  progress->SetHeading(38709);
  progress->SetLine(0, 38715);
  progress->SetLine(1, "");
  progress->SetLine(2, "");
  progress->SetPercentage(0);
  progress->Open();
  progress->ShowProgressBar(true);
  progress->Progress();

  database.BeginTransaction();
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    std::string strLine = StringUtils::Format("%s %i / %i", g_localizeStrings.Get(38710).c_str(), i+1, items.Size());

    progress->SetPercentage((int)((float)i / (float)items.Size() * 100.f));
    progress->SetLine(1, strLine);
    progress->Progress();

    if (!CFile::Exists(item->GetPath()) || item->GetPath().find(strTrainersPath) == std::string::npos)
      database.RemoveTrainer(item->GetProperty("idtrainer").asInteger32());

    if (progress->IsCanceled())
    {
      database.RollbackTransaction();
      return false;
    }
  }

  CFileItemList trainers, archives;
  CDirectory::GetDirectory(strTrainersPath, trainers, ".xbtf|.etm", DIR_FLAG_DEFAULTS);
  CDirectory::GetDirectory(strTrainersPath, archives, ".rar|.zip", DIR_FLAG_DEFAULTS);
  for(int i = 0; i < archives.Size(); i++)
  {
    if (URIUtils::HasExtension(archives[i]->GetPath(), ".rar"))
    { // add trainers in rar
      CFileItemList inArchives;
      g_RarManager.GetFilesInRar(inArchives, archives[i]->GetPath());
      for (int j = 0; j < inArchives.Size(); j++)
      {
        if (URIUtils::HasExtension(inArchives[j]->GetURL().Get(), ".xbtf|.etm"))
        {
          CFileItemPtr item(new CFileItem(*inArchives[j]));
          std::string strPathInArchive = item->GetPath();
          CURL url = URIUtils::CreateArchivePath("zip", CURL(archives[i]->GetPath()), strPathInArchive);
          item->SetURL(url);
          trainers.Add(item);
        }
      }
    }
    else if (URIUtils::HasExtension(archives[i]->GetPath(), ".zip"))
    { // add trainers in zip
      CFileItemList zipTrainers;
      CURL url = URIUtils::CreateArchivePath("zip", CURL(archives[i]->GetPath()));
      CDirectory::GetDirectory(url, zipTrainers, ".xbtf|.etm", DIR_FLAG_DEFAULTS);
      for (int j = 0; j < zipTrainers.Size(); j++)
      {
        CFileItemPtr item(new CFileItem(*zipTrainers[j]));
        trainers.Add(item);
      }
    }
  }

  // Now, look for new trainers
  progress->SetLine(1, "");
  progress->SetPercentage(0);
  progress->ShowProgressBar(true);

  // remove folders
  for (int j = 0; j < trainers.Size(); j++)
  {
    if (trainers[j]->m_bIsFolder)
    {
      trainers.Remove(j);
      j--; // don't confuse loop
    }
  }
  CLog::Log(LOGDEBUG,"Found %i trainers", trainers.Size());

  for (int i = 0; i < trainers.Size(); i++)
  {
    std::string strLine = StringUtils::Format("%s %i / %i", g_localizeStrings.Get(38710).c_str(), i+1, trainers.Size());
    progress->SetLine(0, strLine);
    progress->SetPercentage((int)((float)(i) / trainers.Size() *100.f));
    progress->Progress();

    // skip existing trainers
    if (database.HasTrainer(trainers[i]->GetPath()))
      continue;

    CTrainer trainer;
    if (trainer.Load(trainers[i]->GetPath()))
    {
      progress->SetLine(1, trainer.GetName());
      progress->Progress();
      unsigned int iTitle1, iTitle2, iTitle3;
      trainer.GetTitleIds(iTitle1, iTitle2, iTitle3);
      if (iTitle1)
        database.AddTrainer(iTitle1, trainer);
      if (iTitle2)
        database.AddTrainer(iTitle2, trainer);
      if (iTitle3)
        database.AddTrainer(iTitle3, trainer);
    }

    if (progress->IsCanceled())
    {
      database.RollbackTransaction();
      return false;
    }
  }

  database.CommitTransaction();
  database.Close();
  progress->Close();
  return true;
}
