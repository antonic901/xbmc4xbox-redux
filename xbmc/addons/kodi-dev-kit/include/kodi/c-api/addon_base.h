/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDON_BASE_H
#define C_API_ADDON_BASE_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
  //============================================================================
  /// @ingroup cpp_kodi_addon_addonbase_Defs
  /// @defgroup cpp_kodi_addon_addonbase_Defs_ADDON_STATUS enum ADDON_STATUS
  /// @brief <b>Return value of functions in @ref cpp_kodi_addon_addonbase "kodi::addon::CAddonBase"
  /// and associated classes</b>\n
  /// With this Kodi can do any follow-up work or add-on e.g. declare it as defective.
  ///
  ///@{
  typedef enum ADDON_STATUS
  {
    /// @brief For everything OK and no error
    ADDON_STATUS_OK,

    /// @brief A needed connection was lost
    ADDON_STATUS_LOST_CONNECTION,

    /// @brief Addon needs a restart inside Kodi
    ADDON_STATUS_NEED_RESTART,

    /// @brief Necessary settings are not yet set
    ADDON_STATUS_NEED_SETTINGS,

    /// @brief Unknown and incomprehensible error
    ADDON_STATUS_UNKNOWN,

    /// @brief Permanent failure, like failing to resolve methods
    ADDON_STATUS_PERMANENT_FAILURE,

    /* internal used return error if function becomes not used from child on
    * addon */
    ADDON_STATUS_NOT_IMPLEMENTED
  } ADDON_STATUS;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  // TODO: remove this when new C-API is introduced
  ///@{
  typedef struct
  {
    int           type;
    char*         id;
    char*         label;
    int           current;
    char**        entry;
    unsigned int  entry_elements;
  } ADDON_StructSetting;
  ///@}
  //----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDON_BASE_H */
