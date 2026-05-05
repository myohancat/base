/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#include "SysTime.h"
#include <chrono>
#include <ctime>

using namespace std;
using namespace std::chrono;

uint64_t SysTime::getTickCountMs()
{
    auto now = steady_clock::now();
    return duration_cast<milliseconds>(now.time_since_epoch()).count();
}

uint64_t SysTime::getTickCountUs()
{
    auto now = steady_clock::now();
    return duration_cast<microseconds>(now.time_since_epoch()).count();
}

uint64_t SysTime::getCurrentTime()
{
    auto now = system_clock::now();
    return duration_cast<milliseconds>(now.time_since_epoch()).count();
}

void SysTime::getCurrentTime(Time_t* time)
{
    if (!time) return;

    auto now = system_clock::now();
    time_t rawTime = system_clock::to_time_t(now);

    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    struct tm timeInfo;

    localtime_r(&rawTime, &timeInfo);

    time->mYear  = timeInfo.tm_year + 1900;
    time->mMonth = timeInfo.tm_mon + 1;
    time->mDay   = timeInfo.tm_mday;
    time->mHour  = timeInfo.tm_hour;
    time->mMin   = timeInfo.tm_min;
    time->mSec   = timeInfo.tm_sec;
    time->mMsec  = static_cast<unsigned int>(ms.count());
}
