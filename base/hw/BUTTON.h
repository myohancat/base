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

class IButtonListener
{
public:
    virtual ~IButtonListener() { }

    virtual void onButtonPressed(const void* btn)  = 0;
    virtual void onButtonRepeated(const void* btn) = 0;
    virtual void onButtonReleased(const void* btn) = 0;
};

class BUTTON : ITimerHandler, IIRQListener
{
public:
    static BUTTON* open(const char* name, int gpioNum, bool activeLow = false);
    static BUTTON* open(const char* name, int gpioNum, const std::string ioname, bool activeLow = false);
    ~BUTTON();

    void enable();
    void disable();

    void setListener(IButtonListener* listener);

    const char* getName() const;

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

inline const char* BUTTON::getName() const
{
    return mName.c_str();
}

#endif /* __GPIO_BUTTON_H_ */
