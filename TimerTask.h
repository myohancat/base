/**
 * My simple event loop source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __TIMER_TASK_H_
#define __TIMER_TASK_H_

#include "Timer.h"

#include "Task.h"
#include "Mutex.h"
#include "CondVar.h"

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

    bool     mExitTask;

    bool     mRepeat;
    uint64_t mStartTime;
    int      mIntervalMs;

private:
    void     run();
};

#endif /* __TIMER_TASK_H_ */
