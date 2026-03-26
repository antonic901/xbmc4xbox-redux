/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIMultiImage.h
\brief
*/

#include "GUIImage.h"
#include "threads/CriticalSection.h"
#include "utils/Job.h"
#include "utils/Stopwatch.h"

#include <vector>

/*!
 \ingroup controls
 \brief
 */
class CGUIMultiImage : public CGUIControl, public IJobCallback
{
public:
  CGUIMultiImage(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture, unsigned int timePerImage, unsigned int fadeTime, bool randomized, bool loop, unsigned int timeToPauseAtEnd);
  CGUIMultiImage(const CGUIMultiImage &from);
  virtual ~CGUIMultiImage(void);
  virtual CGUIMultiImage* Clone() const { return new CGUIMultiImage(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual void UpdateInfo(const CGUIListItem *item = NULL);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; }
  virtual void SetInvalid();
  virtual bool CanFocus() const;
  virtual std::string GetDescription() const;

  void SetInfo(const KODI::GUILIB::GUIINFO::CGUIInfoLabel &info);
  void SetAspectRatio(const CAspectRatio &ratio);

protected:
  void LoadDirectory();
  void OnDirectoryLoaded();
  void CancelLoading();
  void ResetMultiImage();

  enum DIRECTORY_STATUS { UNLOADED = 0, LOADING, LOADED, READY };
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

  class CMultiImageJob : public CJob
  {
  public:
    explicit CMultiImageJob(const std::string &path);
    virtual bool DoWork();
    virtual const char* GetType() const { return "multiimage"; }

    std::vector<std::string> m_files;
    std::string              m_path;
  };

  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_texturePath;
  std::string m_currentPath;
  unsigned int m_currentImage;
  CStopWatch m_imageTimer;
  unsigned int m_timePerImage;
  unsigned int m_timeToPauseAtEnd;
  bool m_randomized;
  bool m_loop;

  bool m_bDynamicResourceAlloc;
  std::vector<std::string> m_files;

  CGUIImage m_image;

  CCriticalSection m_section;
  DIRECTORY_STATUS m_directoryStatus;
  unsigned int m_jobID;
};

