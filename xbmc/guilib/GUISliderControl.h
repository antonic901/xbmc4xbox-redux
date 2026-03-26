/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUISliderControl.h
\brief
*/

#include "GUIControl.h"
#include "GUITexture.h"

#define SLIDER_CONTROL_TYPE_INT         1
#define SLIDER_CONTROL_TYPE_FLOAT       2
#define SLIDER_CONTROL_TYPE_PERCENTAGE  3

typedef struct
{
  const char *action;
  const char *formatString;
  int         infoCode;
  bool        fireOnDrag;
} SliderAction;

/*!
 \ingroup controls
 \brief
 */
class CGUISliderControl :
      public CGUIControl
{
public:
  typedef enum {
    RangeSelectorLower = 0,
    RangeSelectorUpper = 1
  } RangeSelector;

  CGUISliderControl(int parentID,
                    int controlID,
                    float posX,
                    float posY,
                    float width,
                    float height,
                    const CTextureInfo& backGroundTexture,
                    const CTextureInfo& backGroundTextureDisabled,
                    const CTextureInfo& mibTexture,
                    const CTextureInfo& nibTextureFocus,
                    const CTextureInfo& nibTextureDisabled,
                    int iType,
                    ORIENTATION orientationconst);
  virtual ~CGUISliderControl() {}
  virtual CGUISliderControl* Clone() const { return new CGUISliderControl(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual bool IsActive() const { return true; }
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual void SetRange(int iStart, int iEnd);
  virtual void SetFloatRange(float fStart, float fEnd);
  virtual bool OnMessage(CGUIMessage& message);
  bool ProcessSelector(CGUITexture* background,
                       CGUITexture* nib,
                       unsigned int currentTime,
                       float fScale,
                       RangeSelector selector);
  void SetRangeSelection(bool rangeSelection);
  bool GetRangeSelection() const { return m_rangeSelection; }
  void SetRangeSelector(RangeSelector selector);
  void SwitchRangeSelector();
  void SetInfo(int iInfo);
  void SetPercentage(float iPercent, RangeSelector selector = RangeSelectorLower, bool updateCurrent = false);
  float GetPercentage(RangeSelector selector = RangeSelectorLower) const;
  void SetIntValue(int iValue, RangeSelector selector = RangeSelectorLower, bool updateCurrent = false);
  int GetIntValue(RangeSelector selector = RangeSelectorLower) const;
  void SetFloatValue(float fValue, RangeSelector selector = RangeSelectorLower, bool updateCurrent = false);
  float GetFloatValue(RangeSelector selector = RangeSelectorLower) const;
  void SetIntInterval(int iInterval);
  void SetFloatInterval(float fInterval);
  void SetType(int iType) { m_iType = iType; }
  int GetType() const { return m_iType; }
  virtual std::string GetDescription() const;
  void SetTextValue(const std::string& textValue) { m_textValue = textValue; }
  void SetAction(const std::string &action);

protected:
  CGUISliderControl(const CGUISliderControl& control);

  virtual bool HitTest(const CPoint &point) const;
  virtual bool UpdateColors(const CGUIListItem* item);
  virtual void Move(int iNumSteps);
  virtual void SetFromPosition(const CPoint &point, bool guessSelector = false);
  /*! \brief Get the current position of the slider as a proportion
   \return slider position in the range [0,1]
   */
  float GetProportion(RangeSelector selector = RangeSelectorLower) const;

  /*! \brief Send a click message (and/or action) to the app in response to a slider move
   */
  void SendClick();

  boost::movelib::unique_ptr<CGUITexture> m_guiBackground;
  boost::movelib::unique_ptr<CGUITexture> m_guiBackgroundDisabled;
  boost::movelib::unique_ptr<CGUITexture> m_guiSelectorLower;
  boost::movelib::unique_ptr<CGUITexture> m_guiSelectorUpper;
  boost::movelib::unique_ptr<CGUITexture> m_guiSelectorLowerFocus;
  boost::movelib::unique_ptr<CGUITexture> m_guiSelectorUpperFocus;
  boost::movelib::unique_ptr<CGUITexture> m_guiSelectorLowerDisabled;
  boost::movelib::unique_ptr<CGUITexture> m_guiSelectorUpperDisabled;
  int m_iType;

  bool m_rangeSelection;
  RangeSelector m_currentSelector;

  float m_percentValues[2];

  int m_intValues[2];
  int m_iStart;
  int m_iInterval;
  int m_iEnd;

  float m_floatValues[2];
  float m_fStart;
  float m_fInterval;
  float m_fEnd;

  int m_iInfoCode;
  std::string m_textValue; ///< Allows overriding of the text value to be displayed (parent must update when the slider updates)
  const SliderAction *m_action; ///< Allows the skin to configure the action of a click on the slider \sa SendClick
  bool m_dragging; ///< Whether we're in a (mouse/touch) drag operation or not - some actions are sent only on release.
  ORIENTATION m_orientation;
};

