/**
 * Simple Http Client
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __HTTP_CLIENT_H_
#define __HTTP_CLIENT_H_

#include "Result.h"
#include "Pipe.h"
#include <functional>
#include <map>

enum class HttpMethod
{
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
};

class HttpRequest
{
public:
    HttpRequest(HttpMethod method, const std::string& url) : mMethod(method) { parseUrl(url); }
    ~HttpRequest() { }

    void setHeader(const std::string& key, const std::string& value) { mHeaders[key] = value; }
    void setBody(const std::string& body) { mBody = body; }

    HttpMethod method() const { return mMethod; }
    const std::string& host() const { return mHost; }
    int                port() const { return mPort; }
    const std::string& path() const { return mPath; }

    const std::unordered_map<std::string, std::string>& headers() const { return mHeaders; }
    const std::string& body() const { return mBody; }

private:
    bool parseUrl(const std::string& url);

private:
    HttpMethod mMethod;
    std::string mHost;
    int mPort;
    std::string mPath;
    std::unordered_map<std::string, std::string> mHeaders;
    std::string mBody;
};

class HttpResponse
{
public:
    HttpResponse(const std::string& resp);
    ~HttpResponse();

    int getStatusCode() const;
    const std::string* getHeader(const std::string& key) const;
    const std::string& getBody() const;

private:
    bool parseResponse(const std::string& str);

private:
    std::string mVersion;
    int mStatusCode = 0;
    std::string mReason;
    std::unordered_map<std::string, std::string> mHeaders;
    std::string mBody;
};

class HttpClient
{
public:
    using Callback = std::function<void(Result<HttpResponse>)>;

    HttpClient();
    ~HttpClient();

    Result<HttpResponse> request(HttpMethod method, const std::string& url, int timeout);
    Result<HttpResponse> request(HttpMethod method, const std::string& url, const std::string& body, int timeout);
    Result<HttpResponse> request(const HttpRequest& req, int timeout, int fd_int = -1);

    void requestAsync(HttpMethod method, const std::string& url, int timeout, Callback callback);
    void requestAsync(HttpMethod method, const std::string& url, const std::string& body, int timeout, Callback callback);
    void requestAsync(const HttpRequest& req, int timeout, Callback callback);

    void cancel();

private:
    Pipe mPipe;
};

#endif /* __HTTP_CLIENT_H_ */
