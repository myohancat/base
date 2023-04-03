/**
 * My simple event loop source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __TIMER_H_
#define __TIMER_H_

#include "Types.h"

class ITimer;

class ITimerHandler
{
public:
    virtual ~ITimerHandler() { };

    virtual void onTimerExpired(const ITimer* timer) = 0;
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

#endif /* __TIMER_H_ */
