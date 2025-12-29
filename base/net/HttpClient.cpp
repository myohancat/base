/**
 * Simple Http Client
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "HttpClient.h"

#include "Task.h"
#include "NetUtil.h"
#include "Util.h"
#include "Log.h"

bool HttpRequest::parseUrl(const std::string& url)
{
    char str[4096];
    const char* p = url.c_str();

    if (strncmp(p, "http://", 7))
        return false;

    p += 7;

    char* slash = strchr((char*)p, '/');
    if (slash)
    {
        strncpy(str, p, slash - p);
        str[slash - p] = '\0';
        mPath = std::string(slash);
    }
    else
    {
        strcpy(str, p);
        mPath = "/";
    }

    char* colon = strchr(str, ':');
    if (colon)
    {
        mPort = strtol(colon + 1, NULL, 10);
        *colon = 0;
    }
    else
        mPort = 80;

    mHost = str;
    return true;
}

HttpResponse::HttpResponse(const std::string& resp)
{
    parseResponse(resp);
}

HttpResponse::~HttpResponse()
{
}

static const char* to_undercase(char* str)
{
    char* p = str;
    while(*p) { *p = toupper(*p); p++; }

    return str;
}

bool HttpResponse::parseResponse(const std::string& str)
{
    char header[4096];
    const char* raw = str.data();
    char* p = strstr((char*)raw, "\r\n\r\n"); // Header End
    if (p)
    {
        strncpy(header, raw, p - raw);
        header[p - raw] = 0;
        mBody = std::string(p + 4);
    }
    else
    {
        mBody = str;
        return false;
    }

    char* saveptr;
    int idx = 0;
    for (char* tok = strtok_r(header, "\r\n", &saveptr); tok; tok = strtok_r(NULL, "\r\n", &saveptr))
    {
        if (idx == 0) // status line
        {
            char version[16];
            int status;
            char reason[64];

            sscanf(tok, "%15s %d %63[^\r\n]", version, &status, reason);
            mVersion    = version;
            mStatusCode = status;
            mReason     = reason;
        }
        else
        {
            char* colon = strchr(tok, ':');
            if (colon)
            {
                *colon = 0;

                std::string key = to_undercase(trim(tok));
                std::string value = trim(colon + 1);

                mHeaders[key] = value;
            }
        }
    }

    return true;
}

int HttpResponse::getStatusCode() const
{
    return mStatusCode;
}

const std::string* HttpResponse::getHeader(const std::string& key) const
{
    UNUSED(key);
    return NULL;
}

const std::string& HttpResponse::getBody() const
{
    return mBody;
}


HttpClient::HttpClient()
{
}

HttpClient::~HttpClient()
{
}

Result<HttpResponse> HttpClient::request(HttpMethod method, const std::string& url, int timeout)
{
    HttpRequest req(method, url);

    return request(req, timeout);
}

Result<HttpResponse> HttpClient::request(HttpMethod method, const std::string& url, const std::string& body, int timeout)
{
    HttpRequest req(method, url);
    req.setBody(body);

    return request(req, timeout);
}

static const char* method_to_str(HttpMethod m)
{
    switch (m)
    {
        case HttpMethod::HTTP_GET:    return "GET";
        case HttpMethod::HTTP_POST:   return "POST";
        case HttpMethod::HTTP_PUT:    return "PUT";
        case HttpMethod::HTTP_DELETE: return "DELETE";
    }
    return "GET";
}

Result<HttpResponse> HttpClient::request(const HttpRequest& req, int timeout, int fd_int)
{
    char buf[4096];
    int sock = NetUtil::socket(SOCK_TYPE_TCP);
    if (sock < 0)
    {
        return Err<HttpResponse>(-2, "socket failed");
    }

    if (NetUtil::connect(sock, req.host().c_str(), req.port(), timeout, fd_int) != NET_ERR_SUCCESS) // TODO
    {
        SAFE_CLOSE(sock);
        return Err<HttpResponse>(-2, "connect failed");
    }

    const std::string method = method_to_str(req.method());
    std::string path = req.path().empty() ? "/" :  req.path();

    std::string body = req.body();
    std::unordered_map<std::string,std::string> headers = req.headers();

    if (!headers.count("Host"))         headers["Host"]         = req.host() + (req.port() != 80 ? (":" + std::to_string(req.port())) : "");
    if (!headers.count("Connection"))   headers["Connection"]   = "close";
    if (!headers.count("User-Agent"))   headers["User-Agent"]   = "Medithinq(linux)/1.0";
    if (!headers.count("Accept"))       headers["Accept"]       = "*/*";
    if (!body.empty())
    {
        if (!headers.count("Content-Type")) headers["Content-Type"] = "application/json";
        headers["Content-Length"] = std::to_string(body.size());
    }

    std::string reqLine = method + " " + path + " HTTP/1.1\r\n";
    std::string headerBlock;
    headerBlock.reserve(256);
    for (auto& kv : headers) {
        headerBlock += kv.first; headerBlock += ": "; headerBlock += kv.second; headerBlock += "\r\n";
    }

    std::string requestBytes = reqLine + headerBlock + "\r\n" + body;
    int ret = NetUtil::send(sock, requestBytes.data(), requestBytes.size(), timeout, fd_int); // TODO
    if (ret < 0)
    {
        SAFE_CLOSE(sock);
        return Err<HttpResponse>(ret, "send failed");
    }

    ret = NetUtil::recv(sock, buf, sizeof(buf), timeout, fd_int); // TODO
    if (ret < 0)
    {
        SAFE_CLOSE(sock);
        return Err<HttpResponse>(ret, "recv failed");
    }
    buf[ret] = 0;

//    LOGT("resp Data :\n%s\n", buf);
    SAFE_CLOSE(sock);

    return Ok(HttpResponse(buf));
}

void HttpClient::requestAsync(HttpMethod method, const std::string& url, int timeout, Callback callback)
{
    HttpRequest req(method, url);

    Task::asyncOnce([this, req, timeout, callback] {
        Result<HttpResponse> resp = request(req, timeout, mPipe.getFD());
        callback(resp);
    });
}

void HttpClient::requestAsync(HttpMethod method, const std::string& url, const std::string& body, int timeout, Callback callback)
{
    HttpRequest req(method, url);
    req.setBody(body);

    Task::asyncOnce([this, req, timeout, callback] {
        Result<HttpResponse> resp = request(req, timeout, mPipe.getFD());
        callback(resp);
    });
}

void HttpClient::requestAsync(const HttpRequest& req, int timeout, Callback callback)
{
    Task::asyncOnce([this, req, timeout, callback] {
        Result<HttpResponse> resp = request(req, timeout, mPipe.getFD());
        callback(resp);
    });
}

void HttpClient::cancel()
{
    mPipe.write("T", 1);
}
