/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "MainLoop.h"

#include "SysTime.h"
#include "Log.h"

#include <algorithm>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <inttypes.h>

static bool timer_sort(const Timer* first, const Timer* second)
{
    return first->getExpiry() < second->getExpiry();
}

MainLoop& MainLoop::getInstance()
{
    static MainLoop instance;

    return instance;
}

void MainLoop::addFdWatcher(IFdWatcher* watcher)
{
    Lock lock(mFdWatcherLock);

    if(!watcher)
        return;

    FdWatcherList::iterator it = std::find(mFdWatchers.begin(), mFdWatchers.end(), watcher);
    if(watcher == *it)
    {
        LOGE("FdWatcher is alreay exsit !!");
        return;
    }

    mFdWatchers.push_back(watcher);
}

void MainLoop::removeFdWatcher(IFdWatcher* watcher)
{
    Lock lock(mFdWatcherLock);

    if(!watcher)
        return;

    for(FdWatcherList::iterator it = mFdWatchers.begin(); it != mFdWatchers.end(); it++)
    {
        if(watcher == *it)
        {
            mFdWatchers.erase(it);
            return;
        }
    }
}

void MainLoop::addTimer(Timer* timer)
{
    Lock lock(mTimerLock);

    if(!timer)
        return;

    TimerList::iterator it = std::find(mTimers.begin(), mTimers.end(), timer);
    if(timer == *it)
    {
        LOGE("timer is alreay exsit !!");
        return;
    }

    mTimers.push_back(timer);
    mTimers.sort(timer_sort);
}

void MainLoop::removeTimer(Timer* timer)
{
    Lock lock(mTimerLock);

    if(!timer)
        return;

    for(TimerList::iterator it = mTimers.begin(); it != mTimers.end(); it++)
    {
        if(timer == *it)
        {
            mTimers.erase(it);
            mTimers.sort(timer_sort);
            return;
        }
    }
}

#define WAIT_TIME (10* 1000)

uint32_t MainLoop::runTimers()
{
    Lock lock(mTimerLock);

    int32_t timeout = WAIT_TIME;

    if(mTimers.size())
    {
        uint64_t now    = SysTime::getTickCountMs();
        uint64_t expiry = mTimers.front()->getExpiry();
        if(expiry <= now)
        {
            Timer* timer = mTimers.front();
            if(!mTimers.front()->execute())
            {
                if(timer == mTimers.front())
                    mTimers.pop_front();
            }

            mTimers.sort(timer_sort);
            if(mTimers.size())
                timeout = mTimers.front()->getExpiry() - now;
        }
        else
            timeout = expiry - now;
    }

    if(timeout < 0)
        timeout = 0;
    else if(timeout > WAIT_TIME)
        timeout = WAIT_TIME;

    return timeout;
}

#define _MAX(x, y)   ((x>y)?x:y)
bool MainLoop::loop()
{
    int      nCnt = 0;
    int      nLastFd = -1;
    uint64_t timeToWait;
    struct   timeval sWait;

    fd_set sReadFds;
    FD_ZERO(&sReadFds);

    nLastFd = mPipe[0];
    FD_SET(mPipe[0], &sReadFds);

    for(FdWatcherList::iterator it = mFdWatchers.begin(); it != mFdWatchers.end(); it++)
    {
        nLastFd = _MAX((*it)->getFD(), nLastFd);
        FD_SET((*it)->getFD(), &sReadFds);
    }

    if(nLastFd < 0)
        return false;

    nLastFd++;

    while(runFunctions()) { }; // Execute Post Functions

    while((timeToWait = runTimers()) == 0) { } // Execute Timers

    sWait.tv_sec  = timeToWait / 1000;
    sWait.tv_usec = (timeToWait % 1000) * 1000;

    nCnt = select(nLastFd, &sReadFds, NULL, NULL, &sWait);

    if(nCnt == 0)
    {
        /* TIMEOUT OCCURED */
        return true;
    }

    if(nCnt == -1)
    {
        if(errno == EINTR)
        {
            /* INTERRUPT RECIEVED */
            return true;
        }
        LOGE("select error oucced!!! errno=%d", errno);
        return false;
    }

    if(FD_ISSET(mPipe[0], &sReadFds))
    {
        char cCmd;
        if(read(mPipe[0], &cCmd, 1) < 0)
        {
            LOGE("pipe read failed !");
            return true;
        }

        switch(cCmd)
        {
            case 'T':
                LOGI(">> terminated recieved.");
                return false;
            case 'S':
                return true;
            default:
                LOGE("Unkown Thread Command Inputed '%c'!!!", cCmd);
                return true;
        }
    }

    mFdWatcherLock.lock();
    for(FdWatcherList::iterator it = mFdWatchers.begin(); it != mFdWatchers.end(); it++)
    {
        if(FD_ISSET((*it)->getFD(), &sReadFds))
        {
            (*it)->onFdReadable((*it)->getFD());
        }
    }
    mFdWatcherLock.unlock();

    return true;
}

void MainLoop::post(const std::function<void()> &func)
{
    Lock lock(mFunctionLock);

    mFunctions.push_back(func);
    wakeup();
}

bool MainLoop::runFunctions()
{
    Lock lock(mFunctionLock);

    for(FunctionList::iterator it = mFunctions.begin(); it != mFunctions.end(); it++)
    {
        (*it)();
        mFunctions.erase(it);
        return true;
    }

    return false;
}

void MainLoop::wakeup()
{
    if(mPipe[1] >= 0)
        write(mPipe[1], "S", 1);
}

void MainLoop::terminate()
{
    if(mPipe[1] >= 0)
        write(mPipe[1], "T", 1);
}

MainLoop::MainLoop()
{
    /* for stop command */
    if(pipe(mPipe) < 0)
    {
        LOGE("cannot create pipe !");
    }
}

MainLoop::~MainLoop()
{
    SAFE_CLOSE(mPipe[0]);
    SAFE_CLOSE(mPipe[1]);
}

