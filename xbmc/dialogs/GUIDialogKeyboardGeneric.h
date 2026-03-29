/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "guilib/GUIKeyboard.h"
#include "input/KeyboardLayout.h"

#include <memory>
#include <string>
#include <vector>

class CGUIFont;

enum KEY_TYPE
{
  CAPS,
  LOWER,
  SYMBOLS
};

class CGUIDialogKeyboardGeneric : public CGUIDialog, public CGUIKeyboard
{
  public:
    CGUIDialogKeyboardGeneric();

    //CGUIKeyboard Interface
    virtual bool ShowAndGetInput(char_callback_t pCallback, const std::string &initialString, std::string &typedString, const std::string &heading, bool bHiddenInput);
    virtual void Cancel();
    virtual int GetWindowId() const;

    void SetHeading(const std::string& heading);
    void SetText(const std::string& text);
    const std::string &GetText() const;
    bool IsConfirmed() { return m_bIsConfirmed; }
    void SetHiddenInput(bool hiddenInput) { m_hiddenInput = hiddenInput; }
    bool IsInputHidden() const { return m_hiddenInput; }

  protected:
    virtual void OnWindowLoaded();
    virtual void OnInitWindow();
    virtual bool OnAction(const CAction &action);
    virtual bool OnMessage(CGUIMessage& message);
    virtual void OnDeinitWindow(int nextWindowID);
    void SetControlLabel(int id, const std::string &label);
    void OnShift();
    void MoveCursor(int iAmount);
    void OnLayout();
    void OnReveal();
    void OnSymbols();
    void OnIPAddress();
    void OnOK();

  private:
    void OnClickButton(int iButtonControl);
    void UpdateButtons();
    void Character(const std::string &ch);
    void Backspace();
    void SetEditText(const std::string& text);
    float GetStringWidth(const std::wstring& utf16);
    void ChangeWordList(int direct);  // direct: 0 - first page, 1 - next page, -1 - prev page
    void ShowWordList(int which); // which: 0 - current page, 1 - next page, -1 -prev page
    bool CodingCharacter(const std::string &ch);
    void NormalCharacter(const std::string &ch);

    bool m_bIsConfirmed;
    KEY_TYPE m_keyType;
    bool m_bShift;
    bool m_hiddenInput;
    bool m_isKeyboardNavigationMode;
    int m_previouslyFocusedButton;

    std::vector<KODI::KEYBOARD::CKeyboardLayout> m_layouts;
    unsigned int                 m_currentLayout;

    std::string m_strHeading;
    std::string m_text;       ///< current text

    IInputCodingTablePtr m_codingtable;
    std::vector<std::wstring> m_words;
    std::string m_hzcode;
    int         m_pos;
    int         m_num;
    float       m_listwidth;
    CGUIFont   *m_listfont;
    CCriticalSection  m_CS;

    char_callback_t m_pCharCallback;
};
