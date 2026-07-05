/*
 *  Copyright (C) 2005-2018 Nikola Antonic
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/IApplicationComponent.h"
#include "utils/Idle.h"

class CCdgParser;
class CGUITextLayout;
class ILCD;

struct VOICE_MASK
{
  float energy;
  float pitch;
  float robotic;
  float whisper;
};

/*!
 * \brief Class handling application support for audio volume management.
 */
class CApplicationXbox : public IApplicationComponent
{
public:
  CApplicationXbox();

  float GetCPUUsage();

  bool HasMemoryUpgrade() const;

  void RenderMemoryStatus();

  /*!
   \brief Checks if HDD spindown must be blocked

   Returns the true if spindown of HDD must be blocked.
   */
  bool MustBlockHDSpinDown(bool bCheckThisForNormalSpinDown = true);
  void CheckNetworkHDSpinDown(bool playbackStarted = false);
  void CheckHDSpindown();

  VOICE_MASK GetKaraokeVoiceMask(int iPort) const { return m_karaokeVoiceMask[iPort]; }
  CCdgParser* GetCdgParser() const { return m_pCdgParser; }

  ILCD* GetLCD() const { return g_lcd; }
  bool HasLCD() const { return g_lcd != NULL; }
  void PrintXBETitleToLCD(const std::string& strXbePath);

  bool Load(const TiXmlNode* settings);
  bool Save(TiXmlNode* settings) const;
  bool OnSettingChanged(const CSetting& setting);
  bool OnSettingAction(const CSetting& setting);

private:
  CIdleThread m_idleThread;

  bool m_hasMemoryUpgrade;

  bool m_bSpinDown;
  bool m_bNetworkSpinDown;
  unsigned int m_dwSpinDownTime;

  CCdgParser *m_pCdgParser;
  VOICE_MASK m_karaokeVoiceMask[4];

  ILCD *g_lcd;

  CGUITextLayout *m_debugLayout;
};
