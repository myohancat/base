/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "BUTTON.h"

#include "Log.h"

BUTTON* BUTTON::open(const char* name, int gpioNum, bool activeLow)
{
    IRQ* irq = IRQ::open(gpioNum, GPIO_EDGE_BOTH, activeLow);
    if (!irq)
    {
        LOGE("Failed open %s.. irq num : %d", name, gpioNum);
        return NULL;
    }

    return new BUTTON(name, irq);
}

BUTTON* BUTTON::open(const char* name, int gpioNum, const std::string ioname, bool activeLow)
{
    IRQ* irq = IRQ::open(gpioNum, ioname, GPIO_EDGE_BOTH, activeLow);
    if (!irq)
    {
        LOGE("Failed open %s.. irq ioname : %s (%d)", name, ioname.c_str(), gpioNum);
        return NULL;
    }

    return new BUTTON(name, irq);
}

BUTTON::BUTTON(const char* name, IRQ* irq)
   : mName(name),
     mIRQ(irq)
{
    mRepeatTimer.setHandler(this);
    mIRQ->setListener(this);
}

BUTTON::~BUTTON()
{
    mRepeatTimer.stop();
    SAFE_DELETE(mIRQ);
}

void BUTTON::enable()
{
    mRepeatTimer.stop();
    mIRQ->enable();
}

void BUTTON::disable()
{
    mRepeatTimer.stop();
    mIRQ->disable();
}

void BUTTON::setListener(IButtonListener* listener)
{
    Lock lock(mLock);
    mListener = listener;
}

void BUTTON::onInterrupted(IRQ* irq, char value)
{
    UNUSED(irq);
    bool isPressed = (value == 1);
    if (isPressed)
    {
        Lock lock(mLock);
        mRepeatTimer.start(500, true);
        if (mListener)
            mListener->onButtonPressed(this);
    }
    else
    {
        Lock lock(mLock);
        mRepeatTimer.stop();
        if (mListener)
            mListener->onButtonReleased(this);
    }
}

void BUTTON::onTimerExpired(const ITimer* timer)
{
    UNUSED(timer);
    Lock lock(mLock);
    mRepeatTimer.setInterval(100);
    if (mListener)
        mListener->onButtonRepeated(this);
}
