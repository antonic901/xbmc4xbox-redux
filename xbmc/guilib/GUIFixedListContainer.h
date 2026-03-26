/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIFixedListContainer.h
\brief
*/

#include "GUIBaseContainer.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIFixedListContainer : public CGUIBaseContainer
{
public:
  CGUIFixedListContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems, int fixedPosition, int cursorRange);
  virtual ~CGUIFixedListContainer(void);
  virtual CGUIFixedListContainer* Clone() const { return new CGUIFixedListContainer(*this); }

  virtual bool OnAction(const CAction &action);

protected:
  virtual void Scroll(int amount);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual bool GetOffsetRange(int &minOffset, int &maxOffset) const;
  virtual void ValidateOffset();
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual int GetCursorFromPoint(const CPoint &point, CPoint *itemPoint = NULL) const;
  virtual void SelectItem(int item);
  virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;
  virtual int GetCurrentPage() const;

private:
  /*!
   \brief Get the minimal and maximal cursor positions in the list

   As the fixed list cursor position may vary at the ends of the list based
   on the cursor range, this function computes the minimal and maximal positions
   based on the number of items in the list
   \param minCursor the minimal cursor position
   \param maxCursor the maximal cursor position
   \sa m_fixedCursor, m_cursorRange
   */
  void GetCursorRange(int &minCursor, int &maxCursor) const;

  int m_fixedCursor;    ///< default position the skinner wishes to use for the focused item
  int m_cursorRange;    ///< range that the focused item can vary when at the ends of the list
};

