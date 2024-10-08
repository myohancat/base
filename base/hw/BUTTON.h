/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __GPIO_BUTTON_H_
#define __GPIO_BUTTON_H_

#include "TimerTask.h"
#include "IRQ.h"

class BUTTON;

class IButtonListener
{
public:
    virtual ~IButtonListener() { }

    virtual void onButtonPressed(const BUTTON* btn)  = 0;
    virtual void onButtonRepeated(const BUTTON* btn) = 0;
    virtual void onButtonReleased(const BUTTON* btn) = 0;
};

class BUTTON : public ITimerHandler, IIRQListener
{
public:
    static BUTTON* open(const char* name, int gpioNum, bool activeLow = false);
    ~BUTTON();

    void enable();
    void disable();

    void setListener(IButtonListener* listener);

private:
    BUTTON(const char* name, IRQ* irq);

    void onTimerExpired(const ITimer* timer);
    void onInterrupted(IRQ* irq, char value);

private:
    std::string mName;
    IRQ*        mIRQ;
    TimerTask   mRepeatTimer;

    Mutex            mLock;
    IButtonListener* mListener;
};

#endif /* __GPIO_BUTTON_H_ */
