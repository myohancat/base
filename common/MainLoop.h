/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#pragma once

#include "Timer.h"
#include "ObserverList.h"

#include <atomic>
#include <functional>
#include <list>
#include <mutex>

class IFdWatcher
{
public:
    virtual ~IFdWatcher() { }

    virtual int  getFD() = 0;
    virtual bool onFdReadable(int fd) = 0;
};

class MainLoop
{
public:
    static MainLoop& getInstance();

    void addFdWatcher(IFdWatcher* watcher);
    void removeFdWatcher(IFdWatcher* watcher);

    void addTimer(Timer* timer);
    void removeTimer(Timer* timer);

    void post(const std::function<void()>& func);

    bool loop();
    void wakeup();
    void terminate();

private:
    MainLoop();
    ~MainLoop();

    MainLoop(const MainLoop&) = delete;
    MainLoop& operator=(const MainLoop&) = delete;

    uint32_t runTimers();
    bool     runFunctions();

    bool     drainWakeupFd();

private:
    RawObserverList<IFdWatcher> mFdWatchers;

    std::mutex mTimerLock;
    typedef std::list<Timer*> TimerList;
    TimerList mTimers;

    std::mutex mFunctionLock;
    typedef std::list<std::function<void()> > FunctionList;
    FunctionList mFunctions;

    int mEpollFd;
    int mWakeupFd;

    std::atomic<bool> mTerminated;
};
