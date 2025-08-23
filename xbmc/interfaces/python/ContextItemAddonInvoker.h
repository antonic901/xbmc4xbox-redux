/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/python/AddonPythonInvoker.h"

#include <memory>

class CFileItem;
typedef boost::shared_ptr<CFileItem> CFileItemPtr;

class CContextItemAddonInvoker : public CAddonPythonInvoker
{
public:
  explicit CContextItemAddonInvoker(ILanguageInvocationHandler *invocationHandler,
                                    const CFileItemPtr& item);
  virtual ~CContextItemAddonInvoker();

protected:
  virtual void onPythonModuleInitialization(void* moduleDict);

private:
  const CFileItemPtr m_item;
};
