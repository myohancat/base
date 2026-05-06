/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
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

    if (!Task::start())
    {
        LOGE("failed to create task");
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
    while (shouldRun())
    {
        int64_t currentTime = SysTime::getTickCountMs();
        int64_t expireTime  = mStartTime + mIntervalMs;
        int timeoutMs = (int)(expireTime - currentTime);

        if (timeoutMs < 0)
            timeoutMs = 1; // Escape busy waiting

        msleep(timeoutMs);

        if (!shouldRun())  // Cancel
            break;

        if (mHandler)
            mHandler->onTimerExpired(this);

        if (mRepeat == false)
            break;

        mStartTime += mIntervalMs;
    }
}
