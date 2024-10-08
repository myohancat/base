/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __UART_H_
#define __UART_H_

#include "Mutex.h"
#include "Task.h"
#include "GPIO.h"
#include "Pipe.h"

class UART;

class IUARTListener
{
public:
    virtual ~IUARTListener() { };

    virtual void onDataReceived(uint8_t* data, int len) = 0;
};

class UART : public Task
{
public:
    static UART* open(const char* dev, int baudrate = 115200);
    virtual ~UART();

    void setListener(IUARTListener* listener);

    int write(const uint8_t* data, int len);

private:
    UART(int fd);

    /* Hide this function */
    void start() { Task::start(); }
    void stop()  { Task::stop(); }

    void run();

    bool onPreStart();
    void onPreStop();
    void onPostStop();

private:
    int  mFD   = -1;
    Pipe mPipe;

    bool mExitTask = false;

    Mutex mLock;
    IUARTListener* mListener = NULL;
};


#endif //__GPIO_H_
