#pragma once

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

#include "threads/Thread.h"
#include "Temperature.h"
#include "settings/ISettingCallback.h"

class CFanController : public ISettingCallback, public CThread
{
public:

  void Start(int targetTemperature, int minFanspeed);
  void Stop();

  virtual void OnSettingChanged(const CSetting *setting);

  int GetFanSpeed();
  void SetFanSpeed(const int fanspeed, const bool force = true);
  const CTemperature& GetGPUTemp();
  const CTemperature& GetCPUTemp();
  void SetTargetTemperature(int targetTemperature);
  void SetMinFanSpeed(int minFanspeed);
  void RestoreStartupSpeed();

  static CFanController* Instance();
  static void RemoveInstance();
  virtual ~CFanController();

  static void SettingOptionsSpeedsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current); 
  static void SettingOptionsTemperaturesFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current);

private:
  enum SensorType {
    ST_GPU = 0,
    ST_CPU = 1
  };

  static CFanController* _Instance;

  int targetTemp;
  int m_minFanspeed;
  int systemFanSpeed;
  unsigned long currentFanSpeed;
  int calculatedFanSpeed;
  int tooHotLoopCount;
  int tooColdLoopCount;
  bool inCustomMode;
  bool bIs16Box;
  CTemperature cpuTemp;
  float cpuFrac;
  int   cpuTempCount;
  CTemperature cpuLastTemp;
  CTemperature gpuTemp;
  CTemperature gpuLastTemp;

  SensorType sensor;

  CFanController();

  void GetFanSpeedInternal();
  void GetGPUTempInternal();
  void GetCPUTempInternal();

  void CalcSpeed(int targetTemp);

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
};
