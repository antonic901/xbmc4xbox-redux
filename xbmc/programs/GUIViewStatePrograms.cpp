/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "programs/GUIViewStatePrograms.h"
#include "filesystem/ProgramDatabaseDirectory.h"
#include "filesystem/PluginDirectory.h"
#include "GUIBaseContainer.h"
#include "programs/ProgramDatabase.h"
#include "FileItem.h"
#include "ViewState.h"
#include "settings/Settings.h"
#include "filesystem/Directory.h"
#include "Util.h"
#include "LocalizeStrings.h"

using namespace XFILE;
using namespace PROGRAMDATABASEDIRECTORY;

CStdString CGUIViewStateWindowProgram::GetLockType()
{
  return "programs";
}

CStdString CGUIViewStateWindowProgram::GetExtensions()
{
  return g_settings.m_programExtensions;
}

VECSOURCES& CGUIViewStateWindowProgram::GetSources()
{
  return CGUIViewState::GetSources();
}

CGUIViewStateWindowProgramFiles::CGUIViewStateWindowProgramFiles(const CFileItemList& items) : CGUIViewStateWindowProgram(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SortByLabel, 551, LABEL_MASKS()); // Preformated
    AddSortMethod(SortByDriveType, 564, LABEL_MASKS()); // Preformated
    SetSortMethod(SortByLabel);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SortOrderAscending);
  }
  else
  {
    AddSortMethod(SortByLabel, 551, LABEL_MASKS("%L", "%I", "%L", ""),  // Label, Size | Label, empty
      g_guiSettings.GetBool("filelists.ignorethewhensorting") ? SortAttributeIgnoreArticle : SortAttributeNone);
    AddSortMethod(SortBySize, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Label, Size | Label, Size
    AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Label, Date | Label, Date
    AddSortMethod(SortByFile, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Label, Size | Label, empty

    SetSortMethod(g_settings.m_viewStateProgramFiles.m_sortDescription);
    SetViewAsControl(g_settings.m_viewStateProgramFiles.m_viewMode);
    SetSortOrder(g_settings.m_viewStateProgramFiles.m_sortDescription.sortOrder);
  }
  LoadViewState(items.GetPath(), WINDOW_PROGRAM_FILES);
}

void CGUIViewStateWindowProgramFiles::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAM_FILES, &g_settings.m_viewStateProgramFiles);
}

VECSOURCES& CGUIViewStateWindowProgramFiles::GetSources()
{
  AddOrReplace(g_settings.m_programSources, CGUIViewStateWindowProgram::GetSources());
  return g_settings.m_programSources; 
}

CGUIViewStateWindowProgramNav::CGUIViewStateWindowProgramNav(const CFileItemList& items) : CGUIViewStateWindowProgram(items)
{
  SortAttribute sortAttributes = SortAttributeNone;
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    sortAttributes = SortAttributeIgnoreArticle;

  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Label, empty
    SetSortMethod(SortByNone);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SortOrderNone);
  }
  else if (items.IsProgramDb())
  {
    NODE_TYPE NodeType=CProgramDatabaseDirectory::GetDirectoryChildType(items.GetPath());
    CQueryParams params;
    CProgramDatabaseDirectory::GetQueryParams(items.GetPath(),params);

    switch (NodeType)
    {
    case NODE_TYPE_GAMES_OVERVIEW:
    case NODE_TYPE_APPLICATIONS_OVERVIEW:
    case NODE_TYPE_OVERVIEW:
      {
        AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Label, empty

        SetSortMethod(SortByNone);

        SetViewAsControl(DEFAULT_VIEW_LIST);

        SetSortOrder(SortOrderNone);        
      }
      break;
    case NODE_TYPE_TITLE_GAMES:
    case NODE_TYPE_TITLE_APPLICATIONS:
      {
        AddSortMethod(SortBySortTitle, sortAttributes, 556, LABEL_MASKS("%T", "%R", "%T", "%R"));  // Title, Rating | Title, Rating
        AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y", "%T", "%Y"));  // Title, Year | Title, Year
        AddSortMethod(SortByRating, 563, LABEL_MASKS("%T", "%R", "%T", "%R"));  // Title, Rating | Title, Rating
        AddSortMethod(SortByDateAdded, 570, LABEL_MASKS("%T", "%a", "%T", "%a"));  // Title, DateAdded | Title, DateAdded

        SetSortMethod(g_settings.m_viewStateProgramNavTitles.m_sortDescription);

        SetViewAsControl(g_settings.m_viewStateProgramNavTitles.m_viewMode);

        SetSortOrder(g_settings.m_viewStateProgramNavTitles.m_sortDescription.sortOrder);
      }
      break;
    default:
      break;
    }
  }
  else
  {
    AddSortMethod(SortByLabel, sortAttributes, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Label, Size | Label, empty
    AddSortMethod(SortBySize, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Label, Size | Label, Size
    AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Label, Date | Label, Date
    AddSortMethod(SortByFile, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Label, Size | Label, empty

    SetSortMethod(g_settings.m_viewStateProgramFiles.m_sortDescription);
    SetViewAsControl(g_settings.m_viewStateProgramFiles.m_viewMode);
    SetSortOrder(g_settings.m_viewStateProgramFiles.m_sortDescription.sortOrder); 
  }
  LoadViewState(items.GetPath(), WINDOW_PROGRAM_NAV);
}

void CGUIViewStateWindowProgramNav::SaveViewState()
{
  NODE_TYPE NodeType = CProgramDatabaseDirectory::GetDirectoryChildType(m_items.GetPath());
  if (m_items.IsProgramDb())
  {
    NODE_TYPE NodeType = CProgramDatabaseDirectory::GetDirectoryChildType(m_items.GetPath());
    CQueryParams params;
    CProgramDatabaseDirectory::GetQueryParams(m_items.GetPath(),params);
    switch (NodeType)
    {
    case NODE_TYPE_TITLE_GAMES:
      SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAM_NAV, &g_settings.m_viewStateProgramNavTitles);
      break;
    default:
      SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAM_NAV);
      break;
    }
  }
  else
  {
    SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAM_NAV, &g_settings.m_viewStateProgramFiles); 
  }
}

VECSOURCES& CGUIViewStateWindowProgramNav::GetSources()
{
  //  Setup shares we want to have
  m_sources.clear();
  CFileItemList items;
  if (g_settings.m_bMyProgramNavFlatten)
    CDirectory::GetDirectory("library://program_flat/", items, "");
  else
    CDirectory::GetDirectory("library://program/", items, "");
  for (int i=0; i<items.Size(); ++i)
  {
    CFileItemPtr item=items[i];
    CMediaSource share;
    share.strName=item->GetLabel();
    share.strPath = item->GetPath();
    share.m_strThumbnailImage= item->GetIconImage();
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    m_sources.push_back(share);
  }

  return CGUIViewStateWindowProgram::GetSources();
}
