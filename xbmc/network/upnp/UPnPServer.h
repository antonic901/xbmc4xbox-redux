#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "PltMediaConnect.h"
#include "FileItem.h"

class PLT_MediaObject;
class PLT_HttpRequestContext;

namespace UPNP
{

class CUPnPServer : public PLT_MediaConnect
{
public:
    CUPnPServer(const char* friendly_name, const char* uuid = NULL, int port = 0) :
        PLT_MediaConnect("", friendly_name, false, uuid, port) {
        // hack: override path to make sure it's empty
        // urls will contain full paths to local files
        m_Path = "";  
    }

    // PLT_MediaServer methods
    virtual NPT_Result OnBrowseMetadata(PLT_ActionReference&          action,
                                        const char*                   object_id,
                                        const char*                   filter,
                                        NPT_UInt32                    starting_index,
                                        NPT_UInt32                    requested_count,
                                        const NPT_List<NPT_String>&   sort_criteria,
                                        const PLT_HttpRequestContext& context);
    virtual NPT_Result OnBrowseDirectChildren(PLT_ActionReference&          action,
                                              const char*                   object_id,
                                              const char*                   filter,
                                              NPT_UInt32                    starting_index,
                                              NPT_UInt32                    requested_count,
                                              const NPT_List<NPT_String>&   sort_criteria, 
                                              const PLT_HttpRequestContext& context);
    virtual NPT_Result OnSearchContainer(PLT_ActionReference&          action,
                                         const char*                   container_id,
                                         const char*                   search_criteria,
                                         const char*                   filter,
                                         NPT_UInt32                    starting_index,
                                         NPT_UInt32                    requested_count,
                                         const NPT_List<NPT_String>&   sort_criteria, 
                                         const PLT_HttpRequestContext& context);

    // PLT_FileMediaServer methods
    virtual NPT_Result ServeFile(NPT_HttpRequest&              request,
                                 const NPT_HttpRequestContext& context,
                                 NPT_HttpResponse&             response,
                                 const NPT_String&             file_path);

    NPT_String BuildSafeResourceUri(const char*        host,
                                    const char*        file_path);

    void AddSafeResourceUri(PLT_MediaObject* object, NPT_List<NPT_IpAddress> ips, const char* file_path, const NPT_String& info)
    {
        PLT_MediaItemResource res;
        for(NPT_List<NPT_IpAddress>::Iterator ip = ips.GetFirstItem(); ip; ++ip) {
            res.m_ProtocolInfo = PLT_ProtocolInfo(info);
            res.m_Uri          = BuildSafeResourceUri((*ip).ToString(), file_path);
            object->m_Resources.Add(res);
        }
    }


private:
    PLT_MediaObject* Build(CFileItemPtr                  item,
                           bool                          with_count,
                           const PLT_HttpRequestContext& context,
                           const char*                   parent_id = NULL);
    NPT_Result       BuildResponse(PLT_ActionReference&          action,
                                   CFileItemList&                items,
                                   const char*                   filter,
                                   NPT_UInt32                    starting_index,
                                   NPT_UInt32                    requested_count,
                                   const NPT_List<NPT_String>&   sort_criteria,
                                   const PLT_HttpRequestContext& context,
                                   const char*                   parent_id /* = NULL */);

    // class methods
    static NPT_String GetParentFolder(NPT_String file_path) {
        int index = file_path.ReverseFind("\\");
        if (index == -1) return "";

        return file_path.Left(index);
    }

    NPT_Mutex                       m_FileMutex;
    NPT_Map<NPT_String, NPT_String> m_FileMap;

public:
    // class members
    static NPT_UInt32 m_MaxReturnedItems;
};

} /* namespace UPNP */
