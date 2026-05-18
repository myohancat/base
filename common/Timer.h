/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#pragma once

#include <stdint.h>

class ITimer;

class ITimerHandler
{
public:
    virtual ~ITimerHandler() { };

     /*
     * Called when the timer expires.
     *
     * Return value:
     * - true  : continue the timer if repeat mode is enabled.
     * - false : stop the timer loop after this callback returns.
     *
     * Important:
     * - This callback is called from the timer task thread.
     * - Do not call start(), stop(), or destroy the timer from this callback.
     * - To stop the timer from inside the callback, return false.
     * - The timer pointer is non-owning and is valid only during this call.
     */
    virtual bool onTimerExpired(const ITimer* timer) = 0;
};

class ITimer
{
public:
    virtual ~ITimer() { }

    virtual void setHandler(ITimerHandler* handler) = 0;

    virtual void start(uint32_t msec, bool repeat) = 0;
    virtual void stop() = 0;

    virtual void     setInterval(uint32_t msec) = 0;
    virtual uint32_t getInterval() const = 0;

    virtual void setRepeat(bool repeat) = 0;
    virtual bool getRepeat() const = 0;

};

class Timer : public ITimer
{
#define    TIMER_INFINITE  ((unsigned int)-1)

public:
    Timer();
    ~Timer();

    void setHandler(ITimerHandler* handler);

    void start(uint32_t msec, bool repeat);
    void restart();
    void stop();

    void setInterval(uint32_t msec);
    void setRepeat(bool repeat);

    uint32_t     getInterval() const;
    bool         getRepeat() const;

    uint64_t     getExpiry() const;

    bool         execute();

private:
    ITimerHandler* mHandler;

    unsigned int mInterval;
    bool         mRepeat;
    uint64_t     mExpiry;
    bool         mRunning;
};

inline uint32_t Timer::getInterval() const
{
    return mInterval;
}

inline bool Timer::getRepeat() const
{
    return mRepeat;
}

inline uint64_t Timer::getExpiry() const
{
    return mExpiry;
}
