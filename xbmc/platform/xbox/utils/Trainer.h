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

// parts generously donated by team xored - thanks!

#include <vector>
#include <string>

class CTrainer
{
public:
  CTrainer(int idTrainer = 0);
  ~CTrainer();

  static bool InstallTrainer(CTrainer& trainer);
  static bool RemoveTrainer();

  bool Load(const std::string& strPath);
  void GetTitleIds(unsigned int& title1, unsigned int& title2, unsigned int& title3) const;
  void GetOptionLabels(std::vector<std::string>& vecOptionLabels) const;

  void SetOptions(unsigned char* options); // copies 100 entries!!!

  inline const std::string& GetName() const { return m_vecText[0]; }
  inline int GetNumberOfOptions() const { return m_vecText.size()-2; }
  inline unsigned char* GetOptions() { return m_pTrainerData+m_iOptions; }
  inline const std::string& GetPath() { return m_strPath; }
  inline const int GetTrainerId() { return m_idTrainer; }

  inline bool IsXBTF() const { return m_bIsXBTF; }
  inline int Size() const { return m_iSize; }

  inline const unsigned char* data() const { return m_pTrainerData; }
  inline unsigned char* data() { return m_pTrainerData; }

  /*! \brief Look for trainers
   Search for trainers and import new trainers into database.
   */
  static bool ScanTrainers();

protected:
  unsigned char* m_pData;
  unsigned char* m_pTrainerData; // no alloc
  unsigned int m_iSize;
  unsigned int m_iNumOptions;
  unsigned int m_iOptions;
  int m_idTrainer;
  bool m_bIsXBTF;
  char m_szCreationKey[200];
  std::vector<std::string> m_vecText;
  std::string m_strPath;
};
