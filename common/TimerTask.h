/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#pragma once

#include "Timer.h"
#include "Task.h"

class TimerTask : public ITimer, Task
{
public:
    TimerTask();
    ~TimerTask();

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
    void     run() override;

private:
    mutable std::recursive_mutex mLock;
};

inline bool TimerTask::isRunning() const
{
    return shouldRun();
}
