/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#include "MainLoop.h"

#include "SysTime.h"
#include "Log.h"

#include <algorithm>
#include <chrono>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>

static bool timer_sort(const Timer* first, const Timer* second)
{
    return first->getExpiry() < second->getExpiry();
}

MainLoop& MainLoop::getInstance()
{
    static MainLoop instance;

    return instance;
}

MainLoop::MainLoop()
        : mEpollFd(-1)
        , mWakeupFd(-1)
        , mTerminated(false)
{
    mEpollFd = epoll_create1(EPOLL_CLOEXEC);
    if (mEpollFd < 0)
    {
        LOGE("cannot create epoll fd! errno=%d", errno);
        return;
    }

    mWakeupFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (mWakeupFd < 0)
    {
        LOGE("cannot create eventfd! errno=%d", errno);
        return;
    }

    struct epoll_event event;
    memset(&event, 0, sizeof(event));

    event.events = EPOLLIN;
    event.data.ptr = NULL; // NULL means wakeup fd.

    if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mWakeupFd, &event) < 0)
    {
        LOGE("epoll_ctl add wakeup fd failed! errno=%d", errno);
    }
}

MainLoop::~MainLoop()
{
    if (mWakeupFd >= 0)
    {
        close(mWakeupFd);
        mWakeupFd = -1;
    }

    if (mEpollFd >= 0)
    {
        close(mEpollFd);
        mEpollFd = -1;
    }
}

void MainLoop::addFdWatcher(IFdWatcher* watcher)
{
    if (!watcher)
        return;

    if (mEpollFd < 0)
    {
        LOGE("epoll fd is invalid!");
        return;
    }

    const int fd = watcher->getFD();
    if (fd < 0)
    {
        LOGE("invalid fd watcher. fd=%d", fd);
        return;
    }

    struct epoll_event event;
    memset(&event, 0, sizeof(event));

    event.events = EPOLLIN;
    event.data.ptr = watcher;

    if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        if (errno == EEXIST)
            LOGE("fd watcher already exists in epoll. fd=%d", fd);
        else
            LOGE("epoll_ctl ADD failed. fd=%d errno=%d", fd, errno);

        return;
    }

    mFdWatchers.addObserver(watcher);
}

void MainLoop::removeFdWatcher(IFdWatcher* watcher)
{
    if (!watcher)
        return;

    const int fd = watcher->getFD();

    if (mEpollFd >= 0 && fd >= 0)
    {
        if (epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, NULL) < 0)
        {
            if (errno != ENOENT && errno != EBADF)
            {
                LOGE("epoll_ctl DEL failed. fd=%d errno=%d", fd, errno);
            }
        }
    }

    mFdWatchers.removeObserver(watcher);
}

void MainLoop::addTimer(Timer* timer)
{
    if (!timer)
        return;

    {
        std::lock_guard<std::mutex> lock(mTimerLock);

        TimerList::iterator it = std::find(mTimers.begin(), mTimers.end(), timer);
        if (it != mTimers.end())
        {
            LOGE("timer is alreay exsit !!");
            return;
        }

        mTimers.push_back(timer);
        mTimers.sort(timer_sort);
    }

    wakeup();
}

void MainLoop::removeTimer(Timer* timer)
{
    if (!timer)
        return;

    {
        std::lock_guard<std::mutex> lock(mTimerLock);

        for (TimerList::iterator it = mTimers.begin(); it != mTimers.end(); ++it)
        {
            if (*it == timer)
            {
                mTimers.erase(it);
                break;
            }
        }
    }

    wakeup();
}

#define WAIT_TIME (10 * 1000)

uint32_t MainLoop::runTimers()
{
    Timer* expiredTimer = NULL;
    int32_t timeout = WAIT_TIME;

    {
        std::lock_guard<std::mutex> lock(mTimerLock);

        if (!mTimers.empty())
        {
            const uint64_t now = SysTime::getTickCountMs();
            const uint64_t expiry = mTimers.front()->getExpiry();

            if (expiry <= now)
            {
                expiredTimer = mTimers.front();
                timeout = 0;
            }
            else
            {
                const uint64_t diff = expiry - now;
                timeout = diff > static_cast<uint64_t>(WAIT_TIME)
                    ? WAIT_TIME
                    : static_cast<int32_t>(diff);
            }
        }
    }

    if (expiredTimer)
    {
        const bool keepTimer = expiredTimer->execute();

        {
            std::lock_guard<std::mutex> lock(mTimerLock);

            if (!keepTimer)
            {
                if (!mTimers.empty() && expiredTimer == mTimers.front())
                {
                    mTimers.pop_front();
                }
                else
                {
                    mTimers.remove(expiredTimer);
                }
            }

            mTimers.sort(timer_sort);

            if (!mTimers.empty())
            {
                const uint64_t now = SysTime::getTickCountMs();
                const uint64_t expiry = mTimers.front()->getExpiry();

                if (expiry <= now)
                {
                    timeout = 0;
                }
                else
                {
                    const uint64_t diff = expiry - now;
                    timeout = diff > static_cast<uint64_t>(WAIT_TIME)
                        ? WAIT_TIME
                        : static_cast<int32_t>(diff);
                }
            }
            else
            {
                timeout = WAIT_TIME;
            }
        }
    }

    if (timeout < 0)
        timeout = 0;
    else if (timeout > WAIT_TIME)
        timeout = WAIT_TIME;

    return static_cast<uint32_t>(timeout);
}

bool MainLoop::loop()
{
    if (mEpollFd < 0 || mWakeupFd < 0)
        return false;

    while (runFunctions()) { } // Execute posted functions.

    uint32_t timeToWait = 0;
    while ((timeToWait = runTimers()) == 0) { } // Execute expired timers.

    static const int MAX_EVENTS = 32;
    struct epoll_event events[MAX_EVENTS];

    int eventCount = epoll_wait(
        mEpollFd,
        events,
        MAX_EVENTS,
        static_cast<int>(timeToWait)
    );

    if (eventCount == 0) // TIMEOUT OCCURED
        return true;

    if (eventCount < 0)
    {
        if (errno == EINTR)
            return true; // INTERRUPT RECEIVED

        LOGE("epoll_wait error occured!!! errno=%d", errno);
        return false;
    }

    for (int i = 0; i < eventCount; ++i)
    {
        if (events[i].data.ptr == NULL)
        {
            if (!drainWakeupFd())
                return true;

            if (mTerminated.load())
            {
                LOGI(">> terminated recieved.");
                return false;
            }

            continue;
        }

        IFdWatcher* watcher = static_cast<IFdWatcher*>(events[i].data.ptr);
        if (!watcher)
            continue;

        const int fd = watcher->getFD();
        if (fd < 0)
            continue;

        if (events[i].events & (EPOLLERR | EPOLLHUP))
        {
            LOGE("epoll fd error. fd=%d events=%u", fd, events[i].events);
        }

        if (events[i].events & EPOLLIN)
        {
            watcher->onFdReadable(fd);
        }
    }

    return true;
}

void MainLoop::post(const std::function<void()>& func)
{
    {
        std::lock_guard<std::mutex> lock(mFunctionLock);
        mFunctions.push_back(func);
    }

    wakeup();
}

bool MainLoop::runFunctions()
{
    std::function<void()> func = NULL;

    {
        std::lock_guard<std::mutex> lock(mFunctionLock);

        if (!mFunctions.empty())
        {
            func = mFunctions.front();
            mFunctions.pop_front();
        }
    }

    if (func)
    {
        func();
        return true;
    }

    return false;
}

void MainLoop::wakeup()
{
    if (mWakeupFd < 0)
        return;

    uint64_t value = 1;

    while (true)
    {
        ssize_t ret = write(mWakeupFd, &value, sizeof(value));
        if (ret == static_cast<ssize_t>(sizeof(value)))
            return;

        if (ret < 0 && errno == EINTR)
            continue;

        if (ret < 0 && errno == EAGAIN)
            return;

        if (ret < 0)
            LOGE("eventfd write failed! errno=%d", errno);

        return;
    }
}

void MainLoop::terminate()
{
    mTerminated.store(true);
    wakeup();
}

bool MainLoop::drainWakeupFd()
{
    if (mWakeupFd < 0)
        return false;

    while (true)
    {
        uint64_t value = 0;
        ssize_t ret = read(mWakeupFd, &value, sizeof(value));

        if (ret == static_cast<ssize_t>(sizeof(value)))
            continue;

        if (ret < 0 && errno == EINTR)
            continue;

        if (ret < 0 && errno == EAGAIN)
            return true;

        if (ret < 0)
        {
            LOGE("eventfd read failed! errno=%d", errno);
            return false;
        }

        return true;
    }
}
