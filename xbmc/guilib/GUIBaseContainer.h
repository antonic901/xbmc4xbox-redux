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

#include "GUIAction.h"
#include "IGUIContainer.h"
#include "utils/Stopwatch.h"

#include <list>
#include <boost/move/unique_ptr.hpp>
#include <utility>
#include <vector>

/*!
 \ingroup controls
 \brief
 */

class IListProvider;
class TiXmlNode;
class CGUIListItemLayout;

class CGUIBaseContainer : public IGUIContainer
{
public:
  CGUIBaseContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems);
  explicit CGUIBaseContainer(const CGUIBaseContainer& other);
  virtual ~CGUIBaseContainer(void);

  virtual bool OnAction(const CAction &action);
  virtual void OnDown();
  virtual void OnUp();
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetFocus(bool bOnOff);
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);

  virtual unsigned int GetRows() const;

  virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;

  void SetPageControl(int id);

  virtual std::string GetDescription() const;
  virtual void SaveStates(std::vector<CControlState> &states);
  virtual int GetSelectedItem() const;

  virtual void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);

  void LoadLayout(TiXmlElement *layout);
  void LoadListProvider(TiXmlElement *content, int defaultItem, bool defaultAlways);

  virtual boost::shared_ptr<CGUIListItem> GetListItem(int offset, unsigned int flag = 0) const;

  virtual bool GetCondition(int condition, int data) const;
  virtual std::string GetLabel(int info) const;

  /*! \brief Set the list provider for this container (for python).
   \param provider the list provider to use for this container.
   */
  void SetListProvider(boost::movelib::unique_ptr<IListProvider> provider);

  /*! \brief Set the offset of the first item in the container from the container's position
   Useful for lists/panels where the focused item may be larger than the non-focused items and thus
   normally cut off from the clipping window defined by the container's position + size.
   \param offset CPoint holding the offset in skin coordinates.
   */
  void SetRenderOffset(const CPoint &offset);

  void SetClickActions(const CGUIAction& clickActions) { m_clickActions = clickActions; }
  void SetFocusActions(const CGUIAction& focusActions) { m_focusActions = focusActions; }
  void SetUnFocusActions(const CGUIAction& unfocusActions) { m_unfocusActions = unfocusActions; }

  void SetAutoScrolling(const TiXmlNode *node);
  void ResetAutoScrolling();
  void UpdateAutoScrolling(unsigned int currentTime);

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  bool OnClick(int actionID);

  virtual void ProcessItem(float posX,
                           float posY,
                           boost::shared_ptr<CGUIListItem>& item,
                           bool focused,
                           unsigned int currentTime,
                           CDirtyRegionList& dirtyregions);

  virtual void Render();
  virtual void RenderItem(float posX, float posY, CGUIListItem *item, bool focused);
  virtual void Scroll(int amount);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual bool GetOffsetRange(int &minOffset, int &maxOffset) const;
  virtual void ValidateOffset();
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual void UpdateLayout(bool refreshAllItems = false);
  virtual void SetPageControlRange();
  virtual void UpdatePageControl(int offset);
  virtual void CalculateLayout();
  virtual void SelectItem(int item) {}
  virtual bool SelectItemFromPoint(const CPoint& point) { return false; }
  virtual int GetCursorFromPoint(const CPoint& point, CPoint* itemPoint = NULL) const { return -1; }
  virtual void Reset();
  virtual size_t GetNumItems() const { return m_items.size(); }
  virtual int GetCurrentPage() const;
  bool InsideLayout(const CGUIListItemLayout *layout, const CPoint &point) const;
  virtual void OnFocus();
  virtual void OnUnFocus();
  void UpdateListProvider(bool forceRefresh = false);

  int ScrollCorrectionRange() const;
  inline float Size() const;
  void FreeMemory(int keepStart, int keepEnd);
  void GetCurrentLayouts();
  CGUIListItemLayout *GetFocusedLayout() const;

  CPoint m_renderOffset; ///< \brief render offset of the first item in the list \sa SetRenderOffset

  float m_analogScrollCount;
  unsigned int m_lastHoldTime;

  ORIENTATION m_orientation;
  int m_itemsPerPage;

  std::vector<boost::shared_ptr<CGUIListItem> > m_items;
  typedef std::vector<boost::shared_ptr<CGUIListItem> >::iterator iItems;
  boost::shared_ptr<CGUIListItem> m_lastItem;

  int m_pageControl;

  std::list<CGUIListItemLayout> m_layouts;
  std::list<CGUIListItemLayout> m_focusedLayouts;

  CGUIListItemLayout* m_layout;
  CGUIListItemLayout* m_focusedLayout;
  bool m_layoutCondition;
  bool m_focusedLayoutCondition;

  virtual void ScrollToOffset(int offset);
  void SetContainerMoving(int direction);
  void UpdateScrollOffset(unsigned int currentTime);

  CScroller m_scroller;

  boost::movelib::unique_ptr<IListProvider> m_listProvider;

  bool m_wasReset;  // true if we've received a Reset message until we've rendered once.  Allows
                    // us to make sure we don't tell the infomanager that we've been moving when
                    // the "movement" was simply due to the list being repopulated (thus cursor position
                    // changing around)

  void UpdateScrollByLetter();
  void GetCacheOffsets(int &cacheBefore, int &cacheAfter) const;
  int GetCacheCount() const { return m_cacheItems; }
  bool ScrollingDown() const { return m_scroller.IsScrollingDown(); }
  bool ScrollingUp() const { return m_scroller.IsScrollingUp(); }
  void OnNextLetter();
  void OnPrevLetter();
  void OnJumpLetter(const std::string& letter, bool skip = false);
  void OnJumpSMS(int letter);
  std::vector< std::pair<int, std::string> > m_letterOffsets;

  /*! \brief Set the cursor position
   Should be used by all base classes rather than directly setting it, as
   this also marks the control as dirty (if needed)
   */
  virtual void SetCursor(int cursor);
  inline int GetCursor() const { return m_cursor; }

  /*! \brief Set the container offset
   Should be used by all base classes rather than directly setting it, as
   this also marks the control as dirty (if needed)
   */
  void SetOffset(int offset);
  /*! \brief Returns the index of the first visible row
   returns the first row. This may be outside of the range of available items. Use GetItemOffset() to retrieve the first visible item in the list.
   \sa GetItemOffset
  */
  inline int GetOffset() const { return m_offset; }
  /*! \brief Returns the index of the first visible item
   returns the first visible item. This will always be in the range of available items. Use GetOffset() to retrieve the first visible row in the list.
   \sa GetOffset
  */
  inline int GetItemOffset() const { return CorrectOffset(GetOffset(), 0); }

  // autoscrolling
  INFO::InfoPtr m_autoScrollCondition;
  int           m_autoScrollMoveTime;   // time between to moves
  unsigned int  m_autoScrollDelayTime;  // current offset into the delay
  bool          m_autoScrollIsReversed; // scroll backwards

  unsigned int m_lastRenderTime;

private:
  bool OnContextMenu();

  int m_cursor;
  int m_offset;
  int m_cacheItems;
  CStopWatch m_scrollTimer;
  CStopWatch m_lastScrollStartTimer;
  CStopWatch m_pageChangeTimer;

  CGUIAction m_clickActions;
  CGUIAction m_focusActions;
  CGUIAction m_unfocusActions;

  // letter match searching
  CStopWatch m_matchTimer;
  std::string m_match;
  float m_scrollItemsPerFrame;
  static const int letter_match_timeout = 1000;

  bool m_gestureActive;

  // early inertial scroll cancellation
  bool m_waitForScrollEnd;
  float m_lastScrollValue;
};


