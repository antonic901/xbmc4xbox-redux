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
/*!
 \ingroup controls
 \brief
 */
class CGUIWrappingListContainer : public CGUIBaseContainer
{
public:
  CGUIWrappingListContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems, int fixedPosition);
  virtual ~CGUIWrappingListContainer(void);
  virtual CGUIWrappingListContainer* Clone() const { return new CGUIWrappingListContainer(*this); }

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual int GetSelectedItem() const;

protected:
  virtual void Scroll(int amount);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual bool GetOffsetRange(int &minOffset, int &maxOffset) const;
  virtual void ValidateOffset();
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual void SelectItem(int item);
  virtual void Reset();
  virtual size_t GetNumItems() const { return m_items.size() - m_extraItems; }
  virtual int GetCurrentPage() const;
  virtual void SetPageControlRange();
  virtual void UpdatePageControl(int offset);

  void ResetExtraItems();
  unsigned int m_extraItems;
};

