/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIPanelContainer.h
\brief
*/

#include "GUIBaseContainer.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIPanelContainer : public CGUIBaseContainer
{
public:
  CGUIPanelContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems);
  virtual ~CGUIPanelContainer(void);
  virtual CGUIPanelContainer* Clone() const { return new CGUIPanelContainer(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnLeft();
  virtual void OnRight();
  virtual void OnUp();
  virtual void OnDown();
  virtual bool GetCondition(int condition, int data) const;
  virtual std::string GetLabel(int info) const;
protected:
  virtual bool MoveUp(bool wrapAround);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveLeft(bool wrapAround);
  virtual bool MoveRight(bool wrapAround);
  virtual void Scroll(int amount);
  float AnalogScrollSpeed() const;
  virtual void ValidateOffset();
  virtual void CalculateLayout();
  virtual unsigned int GetRows() const;
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual int GetCursorFromPoint(const CPoint &point, CPoint *itemPoint = NULL) const;
  virtual void SetCursor(int cursor);
  virtual void SelectItem(int item);
  virtual bool HasPreviousPage() const;
  virtual bool HasNextPage() const;
  virtual void ScrollToOffset(int offset);

  int GetCurrentRow() const;
  int GetCurrentColumn() const;

  int m_itemsPerRow;
};

