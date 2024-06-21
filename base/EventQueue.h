/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __EVENT_QUEUE_H_
#define __EVENT_QUEUE_H_

#include "MainLoop.h"

#define MAX_DATA_LEN   (4096)

class IEventHandler
{
public:
    virtual ~IEventHandler() { }

    virtual void onEventReceived(int id, void* data, int dataLen) = 0;
};

class EventQueue : public IFdWatcher
{
public:
    EventQueue();
    ~EventQueue();

    void setHandler(IEventHandler* handler);

    void sendEvent(int id);
    void sendEvent(int id, uintptr_t ptr);
    void sendEvent(int id, void* data, int dataLen);

protected:
    int  getFD();
    bool onFdReadable(int fd);

private:
    Mutex mLock;
    int   mPipe[2];
    IEventHandler* mHandler;
};

#endif /* __EVENT_QUEUE_H_ */
