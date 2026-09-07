#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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

#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "threads/CriticalSection.h"

#include <map>
#include <string>

#include <microhttpd.h>

class CWebServer : public JSONRPC::ITransportLayer
{
public:
    CWebServer();

    bool Start(int port, const std::string &username, const std::string &password);
    bool Stop();
    bool IsStarted();
    void SetCredentials(const std::string &username, const std::string &password);

    virtual bool PrepareDownload(const char *path, CVariant &details, std::string &protocol);
    virtual bool Download(const char *path, CVariant &result);
    virtual int GetCapabilities();
private:
    enum HTTPMethod
    {
        UNKNOWN,
        POST,
        GET,
        HEAD
    };
    struct MHD_Daemon* StartMHD(unsigned int flags, int port);
    static int AskForAuthentication(struct MHD_Connection *connection);
    static bool IsAuthenticated(CWebServer *server, struct MHD_Connection *connection);

    static ssize_t ContentReaderCallback(void *cls, uint64_t pos, char *buf, size_t max);

    static int JSONRPC(CWebServer *server, void **con_cls, struct MHD_Connection *connection, const char *upload_data, size_t *upload_data_size);
    static enum MHD_Result AnswerToConnection(void *cls, struct MHD_Connection *connection,
                                                const char *url, const char *method,
                                                const char *version, const char *upload_data,
                                                size_t *upload_data_size, void **con_cls);
    static void ContentReaderFreeCallback(void *cls);
    static HTTPMethod GetMethod(const char *method);
    static int CreateRedirect(struct MHD_Connection *connection, const std::string &strURL);
    static int CreateFileDownloadResponse(struct MHD_Connection *connection, const std::string &strURL, HTTPMethod methodType);
    static int CreateErrorResponse(struct MHD_Connection *connection, int responseType, HTTPMethod method);
    static int CreateMemoryDownloadResponse(struct MHD_Connection *connection, void *data, size_t size);

    static enum MHD_Result FillArgumentMap(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
    static void StringToBase64(const char *input, std::string &output);

    static const char *CreateMimeTypeFromExtension(const char *ext);

    struct MHD_Daemon *m_daemon;
    bool m_running, m_needcredentials;
    std::string m_Credentials64Encoded;
    CCriticalSection m_critSection;

    class CHTTPClient : public JSONRPC::IClient
    {
    public:
        virtual int  GetPermissionFlags();
        virtual int  GetAnnouncementFlags();
        virtual bool SetAnnouncementFlags(int flags);
    };
};
