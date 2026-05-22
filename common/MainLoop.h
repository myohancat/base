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
    virtual ~IFdWatcher() = default;

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

    enum class LoopCommand : uint8_t
    {
        Wakeup    = 'S',
        Terminate = 'T'
    };

    void sendCommand(LoopCommand command);
    bool drainCommandPipe(bool& terminated);

    void insertTimerLocked(Timer* timer);
    Timer* takeExpiredTimerLocked(uint64_t now);

    uint32_t getNextTimerTimeoutLocked(uint64_t now) const;
    uint32_t getNextTimerTimeout();

private:
    static constexpr uint32_t WaitTimeMs = 10 * 1000;

    using TimerList = std::list<Timer*>;
    using FunctionList = std::list<std::function<void()>>;

    RawObserverList<IFdWatcher, 30> mFdWatchers;

    std::mutex mTimerLock;
    TimerList mTimers;

    std::mutex mFunctionLock;
    FunctionList mFunctions;

    int mEpollFd;
    int mPipe[2];

    std::atomic<bool> mTerminated;
};
