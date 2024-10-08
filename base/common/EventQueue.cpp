/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "EventQueue.h"

#include "Log.h"

EventQueue::EventQueue()
          : mEOS(false),
            mHandler(NULL)
{
    MainLoop::getInstance().addFdWatcher(this);
}

EventQueue::~EventQueue()
{
    MainLoop::getInstance().removeFdWatcher(this);
}

void EventQueue::setHandler(IEventHandler* handler)
{
    Lock lock(mLock);
    mHandler = handler;
}

void EventQueue::sendEvent(int id, void* data, int dataLen)
{
    //Lock lock(mEosLock);

    if(mEOS)
        return;

    if (dataLen > MAX_DATA_LEN)
    {
        LOGE("dataLen (%d) is too big. maximum data len is %d", dataLen, MAX_DATA_LEN);
        return;
    }

    mPipe.write(&id, sizeof(id));
    mPipe.write(&dataLen, sizeof(dataLen));
    if (dataLen > 0)
        mPipe.write(data, dataLen);
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
    //Lock lock(mEosLock);

    if (mEOS)
        return -1;

    return mPipe.getFD();
}

void EventQueue::setEOS(bool eos)
{
    //Lock lock(mEosLock);
    mEOS = eos;
}

void EventQueue::flush()
{
    //Lock lock(mEosLock);
    mPipe.flush();
}

bool EventQueue::onFdReadable(UNUSED_PARAM int fd)
{
    char data[MAX_DATA_LEN];

    int id = 0;
    int len = 0;

    mPipe.read(&id, sizeof(id));
    mPipe.read(&len, sizeof(len));
    if (len > 0)
        mPipe.read(data, len);

    if (mEOS)
        return true;

    mLock.lock();
    if(mHandler)
        mHandler->onEventReceived(id, data, len);
    mLock.unlock();

    return true;
}
