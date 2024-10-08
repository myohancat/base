/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "LED.h"

#include "Log.h"

LED* LED::open(const char* name, int gpioNum, bool activeLow)
{
    GPIO* gpio = GPIO::open(gpioNum);
    if (!gpio)
    {
        LOGE("Failed open %s.. gpio num : %d", name, gpioNum);
        return NULL;
    }

    return new LED(name, gpio, activeLow);
}

LED::LED(const char* name, GPIO* gpio, bool activeLow)
   : mName(name),
     mGPIO(gpio)
{
    mGPIO->setOutDir(GPIO_DIR_OUT);
    mGPIO->setActiveLow(activeLow);
    mTimer.setHandler(this);
}

LED::~LED()
{
    mTimer.stop();
    SAFE_DELETE(mGPIO);
}

void LED::turnOn()
{
    mTimer.stop();
    mGPIO->setValue(true);
}

void LED::turnOff()
{
    mTimer.stop();
    mGPIO->setValue(false);
}

void LED::blink(int intervalMs)
{
    mTimer.start(intervalMs, true);
}

void LED::onTimerExpired(const ITimer* timer)
{
    UNUSED(timer);
    if (mGPIO->getValue())
        mGPIO->setValue(false);
    else
        mGPIO->setValue(true);
}
