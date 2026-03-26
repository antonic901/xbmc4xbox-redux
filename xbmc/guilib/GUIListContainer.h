/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIListContainer.h
\brief
*/

#include "GUIBaseContainer.h"

class CLabelInfo;
class CTextureInfo;

/*!
 \ingroup controls
 \brief
 */
class CGUIListContainer : public CGUIBaseContainer
{
public:
  CGUIListContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems);
  explicit CGUIListContainer(const CGUIListContainer& other);
  //#ifdef GUILIB_PYTHON_COMPATIBILITY
  CGUIListContainer(int parentID, int controlID, float posX, float posY, float width, float height,
                         const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                         const CTextureInfo& textureButton, const CTextureInfo& textureButtonFocus,
                         float textureHeight, float itemWidth, float itemHeight, float spaceBetweenItems);
  //#endif
  virtual ~CGUIListContainer(void);
  virtual CGUIListContainer* Clone() const { return new CGUIListContainer(*this); }

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;

protected:
  virtual void Scroll(int amount);
  virtual void SetCursor(int cursor);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual void ValidateOffset();
  virtual void SelectItem(int item);
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual int GetCursorFromPoint(const CPoint &point, CPoint *itemPoint = NULL) const;
};

