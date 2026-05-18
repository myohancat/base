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


using Lock = std::unique_lock<std::recursive_mutex>;

TimerTask::TimerTask()
          : Task("TimerTask"),
            mRepeat(false),
            mStartTime(0),
            mIntervalMs(0),
            mHandler(nullptr)
{
}

TimerTask::~TimerTask()
{
    stop();
}

void TimerTask::setHandler(ITimerHandler* handler)
{
    Lock lock(mLock);

    mHandler = handler;
}

void TimerTask::start(uint32_t msec, bool repeat)
{
    stop();

    {
        Lock lock(mLock);
        mRepeat     = repeat;
        mStartTime  = SysTime::getTickCountMs();
        mIntervalMs = msec;
    }

    if (!Task::start())
    {
        LOGE("failed to create task");
    }
}

void TimerTask::stop()
{
    Task::stop();
}

void TimerTask::setInterval(uint32_t msec)
{
    Lock lock(mLock);
    mIntervalMs = msec;
}

void TimerTask::setRepeat(bool repeat)
{
    Lock lock(mLock);
    mRepeat = repeat;
}

uint32_t TimerTask::getInterval() const
{
    Lock lock(mLock);
    return mIntervalMs;
}

bool TimerTask::getRepeat() const
{
    Lock lock(mLock);
    return mRepeat;
}

void TimerTask::run()
{
    while (shouldRun())
    {
        int timeoutMs = 0;
        {
            Lock lock(mLock);

            const int64_t currentTime = SysTime::getTickCountMs();
            const int64_t expireTime  = mStartTime + mIntervalMs;
            const int64_t remainMs    = expireTime - currentTime;

            if (remainMs <= 0)
            {
                LOGW("too short remainMs : %lld, change to 1ms", static_cast<long long>(remainMs));
                timeoutMs = 1; // avoid busy waiting.
            }
            else if (remainMs > INT32_MAX)
                timeoutMs = INT32_MAX;
            else
                timeoutMs = static_cast<int>(remainMs);
        }

        msleep(timeoutMs);

        {
            Lock lock(mLock);

            if (!shouldRun())
                break;

            bool keepGoing = mRepeat;

            if (mHandler)
                keepGoing &= mHandler->onTimerExpired(this);

            if (!keepGoing)
                break;

            mStartTime += mIntervalMs;
        }
    }
}
