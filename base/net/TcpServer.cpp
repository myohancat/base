/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "TcpServer.h"

#include <errno.h>
#include <algorithm>

#include "NetUtil.h"
#include "Log.h"

TcpSession::TcpSession(TcpServer* server, int sock, const char* address)
           : Task("TcpSession"),
             mServer(server),
             mAddress(address),
             mSock(sock)
{
}

TcpSession::~TcpSession()
{
    mServer = NULL;
    stop();

    if (mSock >= 0 )
        close(mSock);
}

bool TcpSession::onPreStart()
{
    mExitTask = false;

    mPipe.flush();
    return true;
}

void TcpSession::onPreStop()
{
    mExitTask = true;
    mPipe.write("T", 1);
}

void TcpSession::run()
{
    mLock.lock();
    if (mServer)
        mServer->notifyTcpSessionEstablished(this);
    mLock.unlock();

    while(!mExitTask)
    {
        int ret = NetUtil::fd_poll(mSock, POLL_REQ_IN, 1000, mPipe.getFD());
        if (ret < 0)
            break;

        if (ret == POLL_SUCCESS)
        {
            mLock.lock();
            if (mServer)
                mServer->notifyTcpSessionReadable(this);
            mLock.unlock();
        }
    }

    mLock.lock();
    if (mServer)
        mServer->notifyTcpSessionRemoved(this);
    mLock.unlock();
}

const std::string& TcpSession::getAddress()
{
    return mAddress;
}

int TcpSession::getFD()
{
    return mSock;
}

void* TcpSession::getParam()
{
    return mParam;
}

void TcpSession::setParam(void* param)
{
    mParam = param;
}

TcpServer::TcpServer()
{
}

TcpServer::~TcpServer()
{
    stop();
}

bool TcpServer::start(int port)
{
    mSock = NetUtil::socket(SOCK_TYPE_TCP);

    NetUtil::socket_set_reuseaddr(mSock);
    if(NetUtil::bind(mSock, NULL, port) < 0 )
    {
        SAFE_CLOSE(mSock);
        return false;
    }

    if(NetUtil::listen(mSock, 2) == -1 )
    {
        SAFE_CLOSE(mSock);
        return false;
    }

    MainLoop::getInstance().addFdWatcher(this);

    LOGI("Start TCP SERVER SUCCESS !!!!!");

    return true;

}

void TcpServer::stop()
{
    MainLoop::getInstance().removeFdWatcher(this);

    clearSession();
    SAFE_CLOSE(mSock);

    LOGI("Stop TCP SERVER SUCCESS !!!!!");
}

void TcpServer::setSessionListener(ITcpSessionListener* listener)
{
    Lock lock(mLock);
    mListener = listener;
}

int TcpServer::getFD()
{
    return mSock;
}

void TcpServer::clearSession()
{
    Lock lock(mLock);

    for(std::list<TcpSession*>::iterator it = mSessions.begin(); it != mSessions.end(); it++)
    {
        delete *it;
    }

    mSessions.clear();
}

void TcpServer::notifyTcpSessionEstablished(TcpSession* session)
{
    Lock lock(mLock);

    std::list<TcpSession*>::iterator it = std::find(mSessions.begin(), mSessions.end(), session);
    if(it != mSessions.end())
    {
        LOGE("Session %s is alreay exsit !!");
        return;
    }
    LOGD("TCP Session added : %s", session->getAddress().c_str());
    mSessions.push_back(session);

    if (mListener)
        mListener->onTcpSessionEstablished(session);
}

void TcpServer::notifyTcpSessionRemoved(TcpSession* session)
{
    Lock lock(mLock);

    for(std::list<TcpSession*>::iterator it = mSessions.begin(); it != mSessions.end(); it++)
    {
        if(session == *it)
        {
            mSessions.erase(it);
            LOGD("TCP Session Removed : %s", session->getAddress().c_str());
            delete session;

            if (mListener)
                mListener->onTcpSessionRemoved(session);
            return;
        }
    }
}

void TcpServer::notifyTcpSessionReadable(TcpSession* session)
{
    Lock lock(mLock);
    if (mListener)
        mListener->onTcpSessionReadable(session);
}

bool TcpServer::onFdReadable(int fd)
{
    char szIP[1024];

    int clntSock = NetUtil::accept(fd, szIP, sizeof(szIP));
    if(clntSock < 0)
        return true;

    LOGI("Connected Client (%s)!", szIP);

    TcpSession* session = new TcpSession(this, clntSock, szIP);
    session->start();

    return true;
}
