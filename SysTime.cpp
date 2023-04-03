/**
 * My simple event loop source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "SysTime.h"

#include <time.h>
#include <sys/time.h>

uint64_t SysTime::getTickCountMs()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    return (uint64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

uint64_t SysTime::getTickCountUs()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    return (uint64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
}

uint64_t SysTime::getCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void SysTime::getCurrentTime(Time_t* time)
{
    timeval tv;
    gettimeofday(&tv, 0);

    time_t curtime = tv.tv_sec;
    tm *t = localtime(&curtime);

    time->mYear  = t->tm_year + 1900;
    time->mMonth = t->tm_mon + 1;
    time->mDay   = t->tm_mday;
    time->mHour  = t->tm_hour;
    time->mMin   = t->tm_min;
    time->mSec   = t->tm_sec;
    time->mMsec  = tv.tv_usec/1000;
}

