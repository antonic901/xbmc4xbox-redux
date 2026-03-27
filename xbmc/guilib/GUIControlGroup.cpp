/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIControlGroup.h"

#include "GUIMessage.h"

#include <cassert>
#include <utility>

using namespace KODI;

CGUIControlGroup::CGUIControlGroup()
{
  m_defaultControl = 0;
  m_defaultAlways = false;
  m_focusedControl = 0;
  m_renderFocusedLast = false;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::CGUIControlGroup(int parentID, int controlID, float posX, float posY, float width, float height)
: CGUIControlLookup(parentID, controlID, posX, posY, width, height)
{
  m_defaultControl = 0;
  m_defaultAlways = false;
  m_focusedControl = 0;
  m_renderFocusedLast = false;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::CGUIControlGroup(const CGUIControlGroup &from)
: CGUIControlLookup(from)
{
  m_defaultControl = from.m_defaultControl;
  m_defaultAlways = from.m_defaultAlways;
  m_renderFocusedLast = from.m_renderFocusedLast;

  // run through and add our controls
  for (ciControls it = from.m_children.begin(); it != from.m_children.end(); ++it)
    AddControl((*it)->Clone());

  // defaults
  m_focusedControl = 0;
  ControlType = GUICONTROL_GROUP;
}

CGUIControlGroup::~CGUIControlGroup(void)
{
  ClearAll();
}

void CGUIControlGroup::AllocResources()
{
  CGUIControl::AllocResources();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    if (!control->IsDynamicallyAllocated())
      control->AllocResources();
  }
}

void CGUIControlGroup::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    control->FreeResources(immediately);
  }
}

void CGUIControlGroup::DynamicResourceAlloc(bool bOnOff)
{
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    control->DynamicResourceAlloc(bOnOff);
  }
}

void CGUIControlGroup::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CPoint pos(GetPosition());
  CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(pos.x, pos.y);

  CRect rect;
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    control->UpdateVisibility(nullptr);
    unsigned int oldDirty = dirtyregions.size();
    control->DoProcess(currentTime, dirtyregions);
    if (control->IsVisible() || (oldDirty != dirtyregions.size())) // visible or dirty (was visible?)
      rect.Union(control->GetRenderRegion());
  }

  CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
  CGUIControl::Process(currentTime, dirtyregions);
  m_renderRegion = rect;
}

void CGUIControlGroup::Render()
{
  CPoint pos(GetPosition());
  CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(pos.x, pos.y);
  CGUIControl *focusedControl = NULL;
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    if (m_renderFocusedLast && control->HasFocus())
      focusedControl = control;
    else
      control->DoRender();
  }
  if (focusedControl)
    focusedControl->DoRender();
  CGUIControl::Render();
  CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
}

bool CGUIControlGroup::OnAction(const CAction &action)
{
  return false;
}

bool CGUIControlGroup::HasFocus() const
{
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    if (control->HasFocus())
      return true;
  }
  return false;
}

bool CGUIControlGroup::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage() )
  {
  case GUI_MSG_ITEM_SELECT:
    {
      if (message.GetControlId() == GetID())
      {
        m_focusedControl = message.GetParam1();
        return true;
      }
      break;
    }
  case GUI_MSG_ITEM_SELECTED:
    {
      if (message.GetControlId() == GetID())
      {
        message.SetParam1(m_focusedControl);
        return true;
      }
      break;
    }
  case GUI_MSG_FOCUSED:
    { // a control has been focused
      m_focusedControl = message.GetControlId();
      SetFocus(true);
      // tell our parent thatwe have focus
      if (m_parentControl)
        m_parentControl->OnMessage(message);
      return true;
    }
  case GUI_MSG_SETFOCUS:
    {
      // first try our last focused control...
      if (!m_defaultAlways && m_focusedControl)
      {
        CGUIControl *control = GetFirstFocusableControl(m_focusedControl);
        if (control)
        {
          CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), control->GetID());
          return control->OnMessage(msg);
        }
      }
      // ok, no previously focused control, try the default control first
      if (m_defaultControl)
      {
        CGUIControl *control = GetFirstFocusableControl(m_defaultControl);
        if (control)
        {
          CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), control->GetID());
          return control->OnMessage(msg);
        }
      }
      // no success with the default control, so just find one to focus
      CGUIControl *control = GetFirstFocusableControl(0);
      if (control)
      {
        CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), control->GetID());
        return control->OnMessage(msg);
      }
      // unsuccessful
      return false;
      break;
    }
  case GUI_MSG_LOSTFOCUS:
    {
      // set all subcontrols unfocused
      for (iControls it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->SetFocus(false);
      if (!GetControl(message.GetParam1()))
      { // we don't have the new id, so unfocus
        SetFocus(false);
        if (m_parentControl)
          m_parentControl->OnMessage(message);
      }
      return true;
    }
    break;
  case GUI_MSG_PAGE_CHANGE:
  case GUI_MSG_REFRESH_THUMBS:
  case GUI_MSG_REFRESH_LIST:
  case GUI_MSG_WINDOW_RESIZE:
    { // send to all child controls (make sure the target is the control id)
      for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      {
        CGUIControl* control = *it;
        CGUIMessage msg(message.GetMessage(), message.GetSenderId(), control->GetID(), message.GetParam1());
        control->OnMessage(msg);
      }
      return true;
    }
    break;
  case GUI_MSG_REFRESH_TIMER:
    if (!IsVisible() || !IsVisibleFromSkin())
      return true;
    break;
  }
  bool handled(false);
  //not intended for any specific control, send to all childs and our base handler.
  if (message.GetControlId() == 0)
  {
    for (iControls it = m_children.begin();it != m_children.end(); ++it)
    {
      CGUIControl* control = *it;
      handled |= control->OnMessage(message);
    }
    return CGUIControl::OnMessage(message) || handled;
  }
  // if it's intended for us, then so be it
  if (message.GetControlId() == GetID())
    return CGUIControl::OnMessage(message);

  return SendControlMessage(message);
}

bool CGUIControlGroup::SendControlMessage(CGUIMessage &message)
{
  IDCollector collector(m_idCollector);

  CGUIControl *ctrl(GetControl(message.GetControlId(), collector.m_collector));
  // see if a child matches, and send to the child control if so
  if (ctrl && ctrl->OnMessage(message))
    return true;

  // Unhandled - send to all matching invisible controls as well
  bool handled(false);
  for (iControls it = (*collector.m_collector).begin(); it != (*collector.m_collector).end(); ++it)
    if ((*it)->OnMessage(message))
      handled = true;

  return handled;
}

bool CGUIControlGroup::CanFocus() const
{
  if (!CGUIControl::CanFocus()) return false;
  // see if we have any children that can be focused
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    const CGUIControl *control = *it;
    if (control->CanFocus())
      return true;
  }
  return false;
}

void CGUIControlGroup::SetInitialVisibility()
{
  CGUIControl::SetInitialVisibility();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->SetInitialVisibility();
}

void CGUIControlGroup::QueueAnimation(ANIMATION_TYPE animType)
{
  CGUIControl::QueueAnimation(animType);
  // send window level animations to our children as well
  if (animType == ANIM_TYPE_WINDOW_OPEN || animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      (*it)->QueueAnimation(animType);
  }
}

void CGUIControlGroup::ResetAnimation(ANIMATION_TYPE animType)
{
  CGUIControl::ResetAnimation(animType);
  // send window level animations to our children as well
  if (animType == ANIM_TYPE_WINDOW_OPEN || animType == ANIM_TYPE_WINDOW_CLOSE)
  {
    for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      (*it)->ResetAnimation(animType);
  }
}

void CGUIControlGroup::ResetAnimations()
{ // resets all animations, regardless of condition
  CGUIControl::ResetAnimations();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->ResetAnimations();
}

bool CGUIControlGroup::IsAnimating(ANIMATION_TYPE animType)
{
  if (CGUIControl::IsAnimating(animType))
    return true;

  if (IsVisible())
  {
    for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    {
      CGUIControl *control = *it;
      if (control->IsAnimating(animType))
        return true;
    }
  }
  return false;
}

bool CGUIControlGroup::HasAnimation(ANIMATION_TYPE animType)
{
  if (CGUIControl::HasAnimation(animType))
    return true;

  if (IsVisible())
  {
    for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    {
      CGUIControl *control = *it;
      if (control->HasAnimation(animType))
        return true;
    }
  }
  return false;
}

void CGUIControlGroup::UnfocusFromPoint(const CPoint &point)
{
  CPoint controlCoords(point);
  m_transform.InverseTransformPosition(controlCoords.x, controlCoords.y);
  controlCoords -= GetPosition();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    (*it)->UnfocusFromPoint(controlCoords);
  }
  CGUIControl::UnfocusFromPoint(point);
}

int CGUIControlGroup::GetFocusedControlID() const
{
  if (m_focusedControl) return m_focusedControl;
  CGUIControl *control = GetFocusedControl();
  if (control) return control->GetID();
  return 0;
}

CGUIControl *CGUIControlGroup::GetFocusedControl() const
{
  // try lookup first
  if (m_focusedControl)
  {
    // we may have multiple controls with same id - we pick first that has focus
    std::pair<LookupMap::const_iterator, LookupMap::const_iterator> range = GetLookupControls(m_focusedControl);

    for (LookupMap::const_iterator i = range.first; i != range.second; ++i)
    {
      if (i->second->HasFocus())
        return i->second;
    }
  }

  // if lookup didn't find focused control, iterate m_children to find it
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    // Avoid calling HasFocus() on control group as it will (possibly) recursively
    // traverse entire group tree just to check if there is focused control.
    // We are recursively traversing it here so no point in doing it twice.
    CGUIControlGroup *groupControl(dynamic_cast<CGUIControlGroup*>(control));
    if (groupControl)
    {
      CGUIControl* focusedControl = groupControl->GetFocusedControl();
      if (focusedControl)
        return focusedControl;
    }
    else if (control->HasFocus())
      return control;
  }
  return NULL;
}

// in the case of id == 0, we don't match id
CGUIControl *CGUIControlGroup::GetFirstFocusableControl(int id)
{
  if (!CanFocus()) return NULL;
  if (id && id == GetID()) return this; // we're focusable and they want us
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl* pControl = *it;
    CGUIControlGroup *group(dynamic_cast<CGUIControlGroup*>(pControl));
    if (group)
    {
      CGUIControl *control = group->GetFirstFocusableControl(id);
      if (control) return control;
    }
    if ((!id || pControl->GetID() == id) && pControl->CanFocus())
      return pControl;
  }
  return NULL;
}

void CGUIControlGroup::AddControl(CGUIControl *control, int position /* = -1*/)
{
  if (!control) return;
  if (position < 0 || position > (int)m_children.size())
    position = (int)m_children.size();
  m_children.insert(m_children.begin() + position, control);
  control->SetParentControl(this);
  control->SetControlStats(m_controlStats);
  control->SetPushUpdates(m_pushedUpdates);
  AddLookup(control);
  SetInvalid();
}

bool CGUIControlGroup::InsertControl(CGUIControl *control, const CGUIControl *insertPoint)
{
  // find our position
  for (unsigned int i = 0; i < m_children.size(); i++)
  {
    CGUIControl *child = m_children[i];
    CGUIControlGroup *group(dynamic_cast<CGUIControlGroup*>(child));
    if (group && group->InsertControl(control, insertPoint))
      return true;
    else if (child == insertPoint)
    {
      AddControl(control, i);
      return true;
    }
  }
  return false;
}

void CGUIControlGroup::SaveStates(std::vector<CControlState> &states)
{
  // save our state, and that of our children
  states.push_back(CControlState(GetID(), m_focusedControl));
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->SaveStates(states);
}

// Note: This routine doesn't delete the control.  It just removes it from the control list
bool CGUIControlGroup::RemoveControl(const CGUIControl *control)
{
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    CGUIControlGroup *group(dynamic_cast<CGUIControlGroup*>(child));
    if (group && group->RemoveControl(control))
      return true;
    if (control == child)
    {
      m_children.erase(it);
      RemoveLookup(child);
      SetInvalid();
      return true;
    }
  }
  return false;
}

void CGUIControlGroup::ClearAll()
{
  // first remove from the lookup table
  RemoveLookup();

  // and delete all our children
  for (iControls it = m_children.begin(); it != m_children.end(); it++)
  {
    delete (*it);
  }
  m_focusedControl = 0;
  m_children.clear();
  ClearLookup();
  SetInvalid();
}

#ifdef _DEBUG
void CGUIControlGroup::DumpTextureUse()
{
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->DumpTextureUse();
}
#endif
