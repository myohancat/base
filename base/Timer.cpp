/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Timer.h"

#include "MainLoop.h"
#include "SysTime.h"

Timer::Timer()
      : mHandler(NULL),
        mInterval(TIMER_INFINITE),
        mRepeat(false),
        mRunning(false)
{
}

Timer::~Timer()
{
    stop();
}

void Timer::setHandler(ITimerHandler* handler)
{
    mHandler = handler;
}

void Timer::start(uint32_t msec, bool repeat)
{
    if(mRunning)
        return;

    mInterval = msec;
    mRepeat   = repeat;

    if(mInterval != TIMER_INFINITE)
        mExpiry = SysTime::getTickCountMs() + mInterval;
    else
        mExpiry = (uint64_t)(-1);

    mRunning = true;
    MainLoop::getInstance().addTimer(this);
    MainLoop::getInstance().wakeup();
}

void Timer::restart()
{
    if(mRunning)
        stop();

    start(mInterval, mRepeat);    
}

void Timer::stop()
{
    mRunning = false;
    MainLoop::getInstance().removeTimer(this);
    MainLoop::getInstance().wakeup();
}

void Timer::setInterval(uint32_t msec)
{
    mInterval = msec;
}

void Timer::setRepeat(bool repeat)
{
    mRepeat = repeat;
}

bool Timer::execute()
{
#if 0 /* TODO */
    uint64_t expiry = SysTime::getTickCount() + mInterval;
#endif
    if(mHandler != NULL)
        mHandler->onTimerExpired(this);
    
    if(mRepeat)
        mExpiry = mExpiry + mInterval;
    else
        mRunning = false;

    return (mRepeat == true);
}
