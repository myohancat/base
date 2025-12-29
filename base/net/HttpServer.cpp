/**
 * Simple Http Server
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "HttpServer.h"

#include <stdlib.h>
#include <arpa/inet.h>

#include "Log.h"

#define DONE          (void*)"done"
#define MAX_BODY_SIZE (4096)

HttpServer::HttpServer(int port)
          : mPort(port)
{
}

HttpServer::~HttpServer()
{
    stop();
}


bool HttpServer::start()
{
    Lock lock(mLock);

    LOGT("start http server : port %d", mPort);
    if (mCtx != nullptr)
    {
        LOGW("alreay started.");
        return false;
    }

    mCtx = mg_start(&requestHandler, this, mPort);

    return (mCtx != nullptr);
}

void HttpServer::stop()
{
    Lock lock(mLock);

    LOGT("stop http server");
    if (mCtx == nullptr)
    {
        LOGW("alreay stopped.");
        return;
    }
    mg_stop(mCtx);
    mCtx = nullptr;
}

bool HttpServer::doGET(struct mg_connection* conn, const char* clientIp, const char* uri, const char* queryString)
{
__TRACE__
    UNUSED(conn);
    UNUSED(clientIp);
    UNUSED(uri);
    UNUSED(queryString);

    return false;
}

bool HttpServer::doPOST(struct mg_connection* conn, const char* clientIp, const char* uri, const char* body)
{
__TRACE__
    UNUSED(conn);
    UNUSED(clientIp);
    UNUSED(uri);
    UNUSED(body);

    return true;
}

bool HttpServer::doPUT(struct mg_connection* conn, const char* clientIp, const char* uri, const char* body)
{
__TRACE__
    UNUSED(conn);
    UNUSED(clientIp);
    UNUSED(uri);
    UNUSED(body);

    return false;
}

bool HttpServer::doDELETE(struct mg_connection* conn, const char* clientIp, const char* uri)
{
__TRACE__
    UNUSED(conn);
    UNUSED(clientIp);
    UNUSED(uri);

    return false;
}

void* HttpServer::requestHandler(enum mg_event event, struct mg_connection* conn, const struct mg_request_info* request_info)
{
    if (!request_info)
        return NULL;

    HttpServer* pThis = static_cast<HttpServer*>(request_info->user_data);

    const char* contentType   = NULL;
    int         contentLength = 0;

    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(request_info->remote_addr.sin_addr), clientIp, INET_ADDRSTRLEN);

    LOGT("clientIp: %s", clientIp);

    /* Process Headers */
    for (int ii = 0; ii < request_info->num_headers; ii++)
    {
        if (!request_info->http_headers[ii].name)
            continue;

        if (strncmp(request_info->http_headers[ii].name, "Content-Type", 12) == 0)
            contentType = request_info->http_headers[ii].value;
        else if (strncmp(request_info->http_headers[ii].name, "Content-Length", 14) == 0)
            contentLength = strtol(request_info->http_headers[ii].value, NULL, 10);

        LOGT("-- header %d : %s : %s", ii, request_info->http_headers[ii].name, request_info->http_headers[ii].value);
    }

    UNUSED(contentType);
    UNUSED(contentLength);

    if (request_info->uri)
        LOGT("-- uri : %s", request_info->uri);

    if (request_info->query_string)
        LOGT("-- query_string : %s", request_info->query_string);

    if (request_info->request_method) /* GET/POST/PUT/DELETE/OPTIONS... */
    {
        if (strncmp(request_info->request_method, "GET", 3) == 0)
        {
            if (pThis->doGET(conn, clientIp, request_info->uri, request_info->query_string))
                return DONE;
        }
        else if(strncmp(request_info->request_method, "POST", 4) == 0)
        {
            char body[MAX_BODY_SIZE +2] = {0x00, };
            int  bodySize = mg_read(conn, body, sizeof(body));

            if (bodySize > MAX_BODY_SIZE)
            {
                mg_send_http_error(conn, 413, "413 Request Entity Too Large", "413 Request Entity Too Large");
                return DONE;
            }

            if (pThis->doPOST(conn, clientIp, request_info->uri, body))
                return DONE;
        }
        else if(strncmp(request_info->request_method, "PUT", 3) == 0)
        {
            char body[MAX_BODY_SIZE +2] = {0x00, };
            int  bodySize = mg_read(conn, body, sizeof(body));

            if (bodySize > MAX_BODY_SIZE)
            {
                mg_send_http_error(conn, 413, "413 Request Entity Too Large", "413 Request Entity Too Large");
                return DONE;
            }

            if (pThis->doPUT(conn, clientIp, request_info->uri, body))
                return DONE;
        }
        else if(strncmp(request_info->request_method, "DELETE", 4) == 0)
        {
            /* TODO */
            if (pThis->doDELETE(conn, clientIp, request_info->uri))
                return DONE;
        }
        else
        {
            // TODO IMPLEMETNS HERE
        }
    }

    if (event == MG_NEW_REQUEST)
    {
        // TODO
    }
    // TODO IMPLEMENTS HERE

    return NULL;
}
