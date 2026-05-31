/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#pragma once

#include "Timer.h"
#include "WorkerThread.h"

class TimerThread : public ITimer, IWorker
{
public:
    TimerThread();
    ~TimerThread();

    void     setHandler(ITimerHandler* handler) override;

    void     start(uint32_t msec, bool repeat) override;
    void     restart() override;
    void     stop() override;

    void     setInterval(uint32_t msec) override;
    uint32_t getInterval() const override;

    void     setRepeat(bool repeat) override;
    bool     getRepeat() const override;

    bool     isRunning() const override;

private:
    ITimerHandler*  mHandler;

    bool     mRepeat;
    uint64_t mStartTime;
    int      mIntervalMs;

private:
    void     run() noexcept override;

private:
    mutable std::recursive_mutex mLock;
    WorkerThread mThread;
};

inline bool TimerThread::isRunning() const
{
    return mThread.shouldRun();
}
