/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __IRQ_H_
#define __IRQ_H_

#include "Task.h"
#include "GPIO.h"
#include "Pipe.h"

class IRQ;

class IIRQListener
{
public:
    virtual ~IIRQListener() { };

    virtual void onInterrupted(IRQ* irq, bool value) = 0;
};

class IRQ : public GPIO, Task
{
public:
    static IRQ* open(int num, GPIO_Edge_e edge = GPIO_EDGE_FALLING, bool activeLow = false);
    static IRQ* open(int num, const std::string ioname, GPIO_Edge_e edge = GPIO_EDGE_FALLING, bool activeLow = false);
    virtual ~IRQ();

    void disable(bool isAsyncCall = true);
    void enable(bool isAsyncCall = true);

    void setListener(IIRQListener* listener);

private:
    IRQ(int num, GPIO_Edge_e edge = GPIO_EDGE_FALLING, bool activeLow = false);
    IRQ(int num, std::string ioname, GPIO_Edge_e edge = GPIO_EDGE_FALLING, bool activeLow = false);

    void run();

    bool onPreStart();
    void onPreStop();
    void onPostStop();

private:
    int  mFD   = -1;

    Pipe mPipe;
    bool mExitTask = false;

    IIRQListener* mListener = NULL;

    GPIO_Edge_e   mEdge;
    bool          mActiveLow;
};


#endif //__IRQ_H_
