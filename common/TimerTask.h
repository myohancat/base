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

    void     setHandler(ITimerHandler* handler);

    void     start(uint32_t msec, bool repeat);
    void     stop();

    void     setInterval(uint32_t msec);
    uint32_t getInterval() const;

    void     setRepeat(bool repeat);
    bool     getRepeat() const;

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
