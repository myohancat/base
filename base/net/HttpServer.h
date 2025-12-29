/**
 * Simple Http Server
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __HTTP_SERVER_H_
#define __HTTP_SERVER_H_

#include "Types.h"
#include "Mutex.h"
#include "mongoose.h"

class HttpServer
{
public:
    HttpServer(int port);
    virtual ~HttpServer();

    bool start();
    void stop();

protected:
    virtual bool doGET(struct mg_connection* conn, const char* clientIp, const char* uri, const char* queryString);
    virtual bool doPOST(struct mg_connection* conn, const char* clientIp, const char* uri, const char* body);
    virtual bool doPUT(struct mg_connection* conn, const char* clientIp, const char* uri, const char* body);
    virtual bool doDELETE(struct mg_connection* conn, const char* clientIp, const char* uri);

private:
    static void* requestHandler(enum mg_event event, struct mg_connection* conn, const struct mg_request_info* request_info);

    const char* getClientIpAddress(struct mg_connection* conn);

private:
    Mutex  mLock;
    int    mPort = 0;
    struct mg_context* mCtx = nullptr;
};

#endif //__HTTP_SERVER_H_
