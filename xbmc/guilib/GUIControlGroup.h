/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIControlGroup.h
\brief
*/

#include "GUIControlLookup.h"

#include <vector>

/*!
 \ingroup controls
 \brief group of controls, useful for remembering last control + animating/hiding together
 */
class CGUIControlGroup : public CGUIControlLookup
{
public:
  CGUIControlGroup();
  CGUIControlGroup(int parentID, int controlID, float posX, float posY, float width, float height);
  explicit CGUIControlGroup(const CGUIControlGroup& from);
  virtual ~CGUIControlGroup(void);
  virtual CGUIControlGroup* Clone() const { return new CGUIControlGroup(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool SendControlMessage(CGUIMessage& message);
  virtual bool HasFocus() const;
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool CanFocus() const;

  virtual void UnfocusFromPoint(const CPoint &point);

  virtual void SetInitialVisibility();

  virtual bool IsAnimating(ANIMATION_TYPE anim);
  virtual bool HasAnimation(ANIMATION_TYPE anim);
  virtual void QueueAnimation(ANIMATION_TYPE anim);
  virtual void ResetAnimation(ANIMATION_TYPE anim);
  virtual void ResetAnimations();

  int GetFocusedControlID() const;
  CGUIControl *GetFocusedControl() const;
  virtual CGUIControl *GetFirstFocusableControl(int id);

  virtual void AddControl(CGUIControl *control, int position = -1);
  bool InsertControl(CGUIControl *control, const CGUIControl *insertPoint);
  virtual bool RemoveControl(const CGUIControl *control);
  virtual void ClearAll();
  void SetDefaultControl(int id, bool always)
  {
    m_defaultControl = id;
    m_defaultAlways = always;
  }
  void SetRenderFocusedLast(bool renderLast) { m_renderFocusedLast = renderLast; }

  virtual void SaveStates(std::vector<CControlState> &states);

  virtual bool IsGroup() const { return true; }

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  // sub controls
  std::vector<CGUIControl *> m_children;

  typedef std::vector<CGUIControl *>::iterator iControls;
  typedef std::vector<CGUIControl *>::const_iterator ciControls;
  typedef std::vector<CGUIControl *>::reverse_iterator rControls;
  typedef std::vector<CGUIControl *>::const_reverse_iterator crControls;

  int  m_defaultControl;
  bool m_defaultAlways;
  int m_focusedControl;
  bool m_renderFocusedLast;
private:
  typedef std::vector< std::vector<CGUIControl *> * > COLLECTORTYPE;

  struct IDCollectorList
  {
    IDCollectorList() : m_stackDepth(0) {}
    ~IDCollectorList()
    {
      for (COLLECTORTYPE::iterator it = m_items.begin(); it != m_items.end(); ++it)
        delete *it;
    }

    std::vector<CGUIControl *> *Get() {
      if (++m_stackDepth > m_items.size())
        m_items.push_back(new std::vector<CGUIControl *>());
      return m_items[m_stackDepth - 1];
    }

    void Release() { --m_stackDepth; }

    COLLECTORTYPE m_items;
    size_t m_stackDepth;
  }m_idCollector;

  struct IDCollector
  {
    explicit IDCollector(IDCollectorList& list) : m_list(list), m_collector(list.Get()) {}

    ~IDCollector() { m_list.Release(); }

    IDCollectorList &m_list;
    std::vector<CGUIControl *> *m_collector;
  };
};

