/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system.h" // <xtl.h>
#include "video/guilib/VideoPlayActionProcessor.h"

#include <boost/shared_ptr.hpp>

class CFileItem;

namespace VIDEO
{
namespace GUILIB
{
class CVideoSelectActionProcessorBase : public CVideoPlayActionProcessorBase
{
public:
  explicit CVideoSelectActionProcessorBase(const boost::shared_ptr<CFileItem>& item)
    : CVideoPlayActionProcessorBase(item)
  {
  }

  virtual ~CVideoSelectActionProcessorBase() {}

  static Action GetDefaultSelectAction();

protected:
  virtual Action GetDefaultAction();
  virtual bool Process(Action action);

  virtual bool OnPlayPartSelected(unsigned int part) = 0;
  virtual bool OnQueueSelected() = 0;
  virtual bool OnInfoSelected() = 0;
  virtual bool OnChooseSelected() = 0;

private:
  CVideoSelectActionProcessorBase();
  unsigned int ChooseStackItemPartNumber() const;
};
} // namespace GUILIB
} // namespace VIDEO
