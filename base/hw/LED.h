/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __LED_H_
#define __LED_H_

#include "TimerTask.h"
#include "GPIO.h"

class LED : public ITimerHandler
{
public:
    static LED* open(const char* name, int gpioNum, bool activeLow = false);
    ~LED();

    void turnOn();
    void turnOff();
    void blink(int intervalMs);

private:
    LED(const char* name, GPIO* gpio, bool activeLow);

    void onTimerExpired(const ITimer* timer);

private:
    std::string mName;
    GPIO*       mGPIO;
    TimerTask   mTimer;
};

#endif /* __LED_H_ */
