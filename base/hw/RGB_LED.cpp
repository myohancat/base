/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "RGB_LED.h"

#include "Log.h"

RGB_LED* RGB_LED::open(const char* name, int gpioRed, int gpioGreen, int gpioBlue, bool activeLow)
{
    GPIO* gpioR = GPIO::open(gpioRed);
    GPIO* gpioG = GPIO::open(gpioGreen);
    GPIO* gpioB = GPIO::open(gpioBlue);

    if (!gpioR || !gpioG || !gpioB)
    {
        if (!gpioR)
            LOGE("Failed open %s.. gpio red num : %d", name, gpioRed);
        else
            delete gpioR;

        if (!gpioG)
            LOGE("Failed open %s.. gpio green num : %d", name, gpioGreen);
        else
            delete gpioG;

        if (!gpioB)
            LOGE("Failed open %s.. gpio blue num : %d", name, gpioBlue);
        else
            delete gpioB;

        return NULL;
    }

    return new RGB_LED(name, gpioR, gpioG, gpioB, activeLow);
}

RGB_LED::RGB_LED(const char* name, GPIO* gpioR, GPIO* gpioG, GPIO* gpioB, bool activeLow)
   : mName(name)
{
    mGpios[RED]   = gpioR;
    mGpios[GREEN] = gpioG;
    mGpios[BLUE]  = gpioB;

    for (int ii = 0; ii < MAX_GPIO; ii++)
    {
        mGpios[ii]->setOutDir(GPIO_DIR_OUT);
        mGpios[ii]->setActiveLow(activeLow);
        mGpios[ii]->setValue(false);
    }

    mTimer.setHandler(this);
}

RGB_LED::~RGB_LED()
{
    mTimer.stop();

    turnOff();

    for (int ii = 0; ii < MAX_GPIO; ii++)
        SAFE_DELETE(mGpios[ii]);
}

void RGB_LED::setColor(bool red, bool green, bool blue)
{
    Lock lock(mLock);
    mColor[RED]   = red;
    mColor[GREEN] = green;
    mColor[BLUE]  = blue;
}

bool RGB_LED::isON()
{
    Lock lock(mLock);
    return mIsON;
}

void RGB_LED::turnOn()
{
    mTimer.stop();

    setLED(true);
}

void RGB_LED::turnOff()
{
    mTimer.stop();

    setLED(false);
}

void RGB_LED::setLED(bool isON)
{
    Lock lock(mLock);
    for (int ii = 0; ii < MAX_GPIO; ii++)
    {
        if (isON)
            mGpios[ii]->setValue(mColor[ii]);
        else
            mGpios[ii]->setValue(false);
    }
    mIsON = isON;
}

void RGB_LED::blink(int intervalMs)
{
    setLED(true);

    mTimer.start(intervalMs, true);
}

void RGB_LED::onTimerExpired(UNUSED_PARAM const ITimer* timer)
{
    if (isON())
        setLED(false);
    else
        setLED(true);
}
