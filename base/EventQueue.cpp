/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "EventQueue.h"

#include "Log.h"
#include <unistd.h>

EventQueue::EventQueue()
          : mHandler(NULL)
{
    pipe(mPipe);
    MainLoop::getInstance().addFdWatcher(this);
}

EventQueue::~EventQueue()
{
    MainLoop::getInstance().removeFdWatcher(this);

    SAFE_CLOSE(mPipe[0]);
    SAFE_CLOSE(mPipe[1]);
}

void EventQueue::setHandler(IEventHandler* handler)
{
    Lock lock(mLock);
    mHandler = handler;
}

void EventQueue::sendEvent(int id, void* data, int dataLen)
{
    if(mPipe[1] < 0)
        return;

    if (dataLen > MAX_DATA_LEN)
    {
        LOGE("dataLen (%d) is too big. maximum data len is %d", dataLen, MAX_DATA_LEN);
        return;
    }

    write(mPipe[1], &id, sizeof(id));
    write(mPipe[1], &dataLen, sizeof(dataLen));
    if (dataLen > 0)
        write(mPipe[1], data, dataLen);
}

void EventQueue::sendEvent(int id, uintptr_t ptr)
{
    sendEvent(id, &ptr, sizeof(uintptr_t));
}

void EventQueue::sendEvent(int id)
{
    sendEvent(id, 0, 0);
}

int EventQueue::getFD()
{
    return mPipe[0];
}

bool EventQueue::onFdReadable(int fd)
{
    char data[MAX_DATA_LEN];

    int id = 0;
    int len = 0;

    read(fd, &id, sizeof(id));
    read(fd, &len, sizeof(len));
    if (len > 0)
        read(fd, data, len);

    mLock.lock();
    if(mHandler)
        mHandler->onEventReceived(id, data, len);
    mLock.unlock();

    return true;
}
