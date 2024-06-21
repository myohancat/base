/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __EVENT_LOOP_H_
#define __EVENT_LOOP_H_

#include "Types.h"

#include "Mutex.h"
#include "Timer.h"

#include <list>
#include <functional>

class IFdWatcher
{
public:
    virtual ~IFdWatcher() { };

    virtual int  getFD() = 0;
    virtual bool onFdReadable(int fd) = 0;
};

class EventLoop
{
public:
    static EventLoop& getInstance();

    void addFdWatcher(IFdWatcher* watcher);
    void removeFdWatcher(IFdWatcher* watcher);

    void addTimer(Timer* timer);
    void removeTimer(Timer* timer);

    void post(const std::function<void()> &func);

    bool loop();
    void wakeup();
    void terminate();

private:
    EventLoop();
    ~EventLoop();

    uint32_t runTimers();
    bool     runFunctions();

    RecursiveMutex mFdWatcherLock;
    typedef std::list<IFdWatcher*> FdWatcherList;
    FdWatcherList mFdWatchers;

    RecursiveMutex mTimerLock;
    typedef std::list<Timer*> TimerList;
    TimerList mTimers;

    RecursiveMutex mFunctionLock;
    typedef std::list<std::function<void()>> FunctionList;
    FunctionList mFunctions;

    int mPipe[2];
};

#endif /* __EVENT_LOOP_H_ */
