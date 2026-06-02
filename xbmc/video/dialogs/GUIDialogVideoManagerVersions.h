/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/dialogs/GUIDialogVideoManager.h"
#include "video/VideoManagerTypes.h" // VideoDbContentType, VideoAssetType, MediaRole

#include <memory>
#include <string>

class CFileItem;

class CGUIDialogVideoManagerVersions : public CGUIDialogVideoManager
{
public:
  CGUIDialogVideoManagerVersions();
  virtual ~CGUIDialogVideoManagerVersions() {}

  virtual void SetVideoAsset(const boost::shared_ptr<CFileItem>& item);

  static bool ProcessVideoVersion(VideoDbContentType::Type itemType, int dbId);
  /*!
   * \brief Open the Manage Versions dialog for a video
   * \param item video to manage
   * \return true: the video or another item was modified, a containing list should be refreshed.
   * false: no changes
   */
  static bool ManageVideoVersions(const boost::shared_ptr<CFileItem>& item);

protected:
  virtual bool OnMessage(CGUIMessage& message);

  virtual VideoAssetType::Type GetVideoAssetType();
  virtual int GetHeadingId() { return 40024; } // Versions:

  virtual void Clear();
  virtual void Refresh();
  virtual void UpdateButtons();
  virtual void Remove();

private:
  void SetDefaultVideoVersion(const CFileItem& version);
  /*!
   * \brief Prompt the user to select a file / movie to add as version
   * \return true if a version was added, false otherwise.
   */
  bool AddVideoVersion();
  void SetDefault();
  void UpdateDefaultVideoVersionSelection();

  /*!
   * \brief Ask the user to choose an item from the list of items, the version type of the item,
   * and perform the version conversion according to the role parameter.
   * \param[in] items The items for the user to choose from
   * \param[in] itemType Type of the item being chosen
   * \param[in] mediaType ?
   * \param[in] dbId id of the video being added if role is NewVersion, id of the video being added
   * to if role is Parent
   * \param[in] videoDb Database connection
   * \param[in] role NewVersion: dbId will be converted to a version of the movie chosen by
   * the user from the whole libray.
   * Parent: dbId will have another movie chosen by the user from the whole library as a new version.
   *
   * \return True: success, a version was created and attached, false otherwise.
   */
  static bool ChooseVideoAndConvertToVideoVersion(CFileItemList& items,
                                                  VideoDbContentType::Type itemType,
                                                  const std::string& mediaType,
                                                  int dbId,
                                                  CVideoDatabase& videoDb,
                                                  MediaRole::Type role);
  /*!
   * \brief Use a file picker to select a file to add as a new version of a movie.
   * \return True when a version was added, false otherwise
   */
  bool AddVideoVersionFilePicker();

  /*!
   * \brief Populates a list of movies of the library that are similar to the item provided as
   * parameter.
   * \param[in] item The reference item
   * \param[out] list List to populate
   * \param[in] videoDb Database connection
   * \return True for success, false otherwise
   */
  static bool GetSimilarMovies(const boost::shared_ptr<CFileItem>& item,
                               CFileItemList& list,
                               CVideoDatabase& videoDb);

  /*!
   * \brief Convert the movie into a version
   * \param itemMovie Movie to convert
   * \return True for success, false otherwse
   */
  bool AddSimilarMovieAsVersion(const boost::shared_ptr<CFileItem>& itemMovie);

  /*!
   * \brief Populates a list with all movies of the libray, excluding the item provided as parameter.
   * \param[in] item The item that will be excluded from the list
   * \param[out] list List to populate
   * \param[in] videoDb Database connection
   * \return True for success, false otherwise.
   */
  static bool GetAllOtherMovies(const boost::shared_ptr<CFileItem>& item,
                                CFileItemList& list,
                                CVideoDatabase& videoDb);

  /*!
   * \brief Shared post processing of lists after extraction and before display
   * \param[in,out] list the list of movies
   * \param[in] dbId item to remove from the list
   * \return True for success, false otherwise.
   */
  static bool PostProcessList(CFileItemList& list, int dbId);

  boost::shared_ptr<CFileItem> m_defaultVideoVersion;
};
