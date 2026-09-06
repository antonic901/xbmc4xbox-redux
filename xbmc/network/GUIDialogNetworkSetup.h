/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CGUIDialogNetworkSetup : public CGUIDialogSettingsManualBase
{
public:
  //! \brief A structure encapsulating properties of a supported protocol.
  struct Protocol
  {
    bool supportPath;      //!< Protocol has path in addition to server name
    bool supportUsername;  //!< Protocol uses logins
    bool supportPassword;  //!< Protocol supports passwords
    bool supportPort;      //!< Protocol supports port customization
    bool supportBrowsing;  //!< Protocol supports server browsing
    int defaultPort;       //!< Default port to use for protocol
    std::string type;      //!< URL type for protocol
    int label;             //!< String ID to use as label in dialog
    std::string addonId; //!< Addon identifier, leaved empty if inside Kodi
  };

  CGUIDialogNetworkSetup(void);
  virtual ~CGUIDialogNetworkSetup(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnBack(int actionID);
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  static bool ShowAndGetNetworkAddress(std::string &path);

  std::string ConstructPath() const;
  bool SetPath(const std::string &path);
  virtual bool IsConfirmed() const { return m_confirmed; }

protected:
  // implementations of ISettingCallback
  virtual void OnSettingChanged(const boost::shared_ptr<const CSetting>& setting);
  virtual void OnSettingAction(const boost::shared_ptr<const CSetting>& setting);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual bool Save() { return true; }
  virtual void SetupView();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

  void OnProtocolChange();
  void OnServerBrowse();
  void OnOK();
  virtual void OnCancel();
  void UpdateButtons();
  void Reset();

  void UpdateAvailableProtocols();

  int m_protocol; //!< Currently selected protocol
  std::vector<Protocol> m_protocols; //!< List of available protocols
  std::string m_server;
  std::string m_path;
  std::string m_username;
  std::string m_password;
  std::string m_port;

  bool m_confirmed;
};
