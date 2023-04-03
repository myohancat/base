/**
 * My simple event loop source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "TimerTask.h"

#include <errno.h>
#include "SysTime.h"
#include "Log.h"


TimerTask::TimerTask()
          : Task("TimerTask")
{
    mExitTask   = false;
    mRepeat     = false;
    mIntervalMs = 0;
    mHandler    = NULL;
}

TimerTask::~TimerTask()
{
    stop();
}

void TimerTask::setHandler(ITimerHandler* handler)
{
    mHandler = handler;
}

void TimerTask::start(uint32_t msec, bool repeat)
{
    stop();

    mRepeat     = repeat;
    mStartTime  = SysTime::getTickCountMs();
    mIntervalMs = msec;
    mExitTask   = false;

    if(!Task::start())
    {
        LOGE("failed to create task\n");
    }
}

void TimerTask::stop()
{
    mExitTask = true;
    Task::stop();
}

void TimerTask::setInterval(uint32_t msec)
{
    mIntervalMs = msec;
}

void TimerTask::setRepeat(bool repeat)
{
    mRepeat = repeat;
}

uint32_t TimerTask::getInterval() const
{
    return mIntervalMs;
}

bool TimerTask::getRepeat() const
{
    return mRepeat;
}

void TimerTask::run()
{
    while(!mExitTask)
    {
        uint64_t expireTime = mStartTime + mIntervalMs;
        uint64_t timeoutMs  = expireTime - SysTime::getTickCountMs();

        sleep(timeoutMs);

        if (state() == TASK_STATE_STOPPING)
            continue;

        if (mHandler)
            mHandler->onTimerExpired(this);

        if(mRepeat == false)
            break;

        mStartTime = expireTime;
    }
}
