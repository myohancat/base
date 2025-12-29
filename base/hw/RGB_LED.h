#ifndef __RBG_RGB_LED_H_
#define __RGB_RGB_LED_H_

#include "TimerTask.h"
#include "GPIO.h"

class RGB_LED : public ITimerHandler
{
public:
    static RGB_LED* open(const char* name, int gpioRed, int gpioGreen, int gpioBlue, bool activeLow = false);
    ~RGB_LED();

    void setColor(bool red, bool green, bool blue);

    bool isON();

    void turnOn();
    void turnOff();
    void blink(int intervalMs);

private:
    enum
    {
        RED,
        GREEN,
        BLUE,

        MAX_GPIO
    };

    RGB_LED(const char* name, GPIO* gpioR, GPIO* gpioG, GPIO* gpioB, bool activeLow);

    void setLED(bool isON);

    void onTimerExpired(const ITimer* timer) override;

private:
    std::string mName;
    Mutex       mLock;
    GPIO*       mGpios[MAX_GPIO]  = {nullptr, };
    bool        mColor[MAX_GPIO]  = {1, 1, 1};

    bool        mIsON = false;
    TimerTask   mTimer;
};

#endif /* __RGB_RGB_LED_H_ */
