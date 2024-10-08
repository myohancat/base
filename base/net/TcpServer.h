/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __TCP_SERVER_H_
#define __TCP_SERVER_H_

#include "MainLoop.h"
#include "Mutex.h"
#include "Task.h"
#include "Pipe.h"

class TcpSession;
class TcpServer;

class ITcpSessionListener
{
public:
    virtual ~ITcpSessionListener() { }

    virtual void onTcpSessionEstablished(TcpSession* session) = 0;
    virtual void onTcpSessionRemoved(TcpSession* session)     = 0;
    virtual void onTcpSessionReadable(TcpSession* session)    = 0;
};

class TcpSession : public Task
{
public:
    int getFD();
    const std::string& getAddress();

    void* getParam();
    void  setParam(void* param);

protected:
    friend class TcpServer;
    TcpSession(TcpServer* server, int sock, const char* address);
    ~TcpSession();

    bool onPreStart();
    void onPreStop();
    void run();

protected:
    TcpServer*  mServer = NULL;

    std::string mAddress;
    int  mSock = -1;

    bool mExitTask = false;
    Pipe mPipe;

    Mutex mLock;
    void* mParam = NULL;
};

class TcpServer : public IFdWatcher
{
public:
    TcpServer();
    ~TcpServer();

    bool start(int port);
    void stop();

    void setSessionListener(ITcpSessionListener* listener);

protected:
    int getFD();
    bool onFdReadable(int fd);

    friend class TcpSession;
    void notifyTcpSessionEstablished(TcpSession* session);
    void notifyTcpSessionRemoved(TcpSession* session);
    void notifyTcpSessionReadable(TcpSession* session);

    void clearSession();

private:
    Mutex mLock;
    int   mSock = -1;

    ITcpSessionListener* mListener = NULL;
    std::list<TcpSession*> mSessions;
};

#endif /* __TCP_SERVER_H_ */
