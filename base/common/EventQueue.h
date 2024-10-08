/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __EVENT_QUEUE_H_
#define __EVENT_QUEUE_H_

#include "MainLoop.h"
#include "Pipe.h"

#define MAX_DATA_LEN   (4096)

class IEventHandler
{
public:
    virtual ~IEventHandler() { }

    virtual void onEventReceived(int id, void* data, int dataLen) = 0;
};

#define UINTPTR_TO_PTR(data)    (void*)(*(uintptr_t*)data)
class EventQueue : public IFdWatcher
{
public:
    EventQueue();
    ~EventQueue();

    void setHandler(IEventHandler* handler);

    void sendEvent(int id);

    void sendEvent(int id, uintptr_t ptr);
    void sendEvent(int id, void* data, int dataLen);

    void setEOS(bool eos);
    void flush();

protected:
    int  getFD();
    bool onFdReadable(int fd);

private:
    Mutex mEosLock;
    bool  mEOS;
    Pipe  mPipe;

    Mutex mLock;
    IEventHandler* mHandler;
};

#endif /* __EVENT_QUEUE_H_ */
