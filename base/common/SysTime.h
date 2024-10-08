/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __SYSTEM_TIME_H_
#define __SYSTEM_TIME_H_

#include "Types.h"

#include <stdint.h>

typedef struct Time_s
{
    uint16_t mYear;
    uint8_t  mMonth;
    uint8_t  mDay;
    uint8_t  mHour;
    uint8_t  mMin;
    uint8_t  mSec;
    uint8_t  mMsec;
} Time_t;

class SysTime
{
public:
    /**
     * @return current tick count (milliseconds) after boot up
     */
    static uint64_t getTickCountMs();

    /**
     * @return current tick count (microseconds) after boot up
     */
    static uint64_t getTickCountUs();

    /**
     * @return current time (milliseconds)
     */
    static uint64_t getCurrentTime();

    static void getCurrentTime(Time_t* time);
};

#endif /* __SYSTEM_TIME_H_ */
