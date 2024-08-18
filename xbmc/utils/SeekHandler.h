#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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

#include <map>
#include <vector>

#include "guilib/Key.h"
#include "interfaces/IActionListener.h"
#include "settings/Settings.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "utils/Stopwatch.h"

enum SeekType
{
  SEEK_TYPE_VIDEO = 0,
  SEEK_TYPE_MUSIC = 1
};

class CSeekHandler : public ISettingCallback, public IActionListener
{
public:
  static CSeekHandler& Get();

  static void SettingOptionsSeekStepsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

  virtual void OnSettingChanged(const CSetting *setting);
  virtual bool OnAction(const CAction &action);

  void Seek(bool forward, float amount, float duration = 0, bool analogSeek = false, SeekType type = SEEK_TYPE_VIDEO);
  void SeekSeconds(int seconds);
  void FrameMove();
  void Reset();
  void Configure();

  int GetSeekSize() const;
  bool InProgress() const;

  bool HasTimeCode() const { return m_timeCodePosition > 0; }
  int GetTimeCodeSeconds() const;

protected:
  CSeekHandler();
  CSeekHandler(const CSeekHandler&);
  CSeekHandler& operator=(CSeekHandler const&);
  virtual ~CSeekHandler();
  bool SeekTimeCode(const CAction &action);
  void ChangeTimeCode(int remote);

private:
  static const int analogSeekDelay = 500;

  int GetSeekStepSize(SeekType type, int step);
  int m_seekDelay;
  std::map<SeekType, int > m_seekDelays;
  bool m_requireSeek;
  bool m_analogSeek;
  int m_seekSize;
  int m_seekStep;
  std::map<SeekType, std::vector<int> > m_forwardSeekSteps;
  std::map<SeekType, std::vector<int> > m_backwardSeekSteps;
  CStopWatch m_timer;
  CStopWatch m_timerTimeCode;
  int m_timeCodeStamp[6];
  int m_timeCodePosition;

  CCriticalSection m_critSection;
};
