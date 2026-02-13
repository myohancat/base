/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "MsgQ.h"
#include "Log.h"

MsgQ::MsgQ() : mCapacity(DEF_QUEUE_SIZE), mEOS(false) { }
MsgQ::MsgQ(size_t capacity) : mCapacity(capacity), mEOS(false) { }
MsgQ::~MsgQ() { }

bool MsgQ::put(const Msg& msg, int timeoutMs)
{
    Lock lock(mLock);

    while (mQueue.size() >= mCapacity && !mEOS)
    {
        if (timeoutMs == 0)
            return false;
        else if (timeoutMs == -1) // Infinite
            mCvEmpty.wait(mLock);
        else
        {
            if (!mCvEmpty.wait(mLock, timeoutMs))
                return false;
        }
    }

    if (mQueue.size() >= mCapacity)
        return false;

    mQueue.push_back(msg);
    mCvFull.signal();

    return true;
}

bool MsgQ::get(Msg* msg, int timeoutMs)
{
    Lock lock(mLock);

    while (mQueue.size() == 0 && !mEOS)
    {
        if (timeoutMs == 0)
            return false;
        else if (timeoutMs == -1) // Infinite
            mCvFull.wait(mLock);
        else
        {
            if (!mCvFull.wait(mLock, timeoutMs))
                return false;
        }
    }

    if (mEOS)
        return false;

    if (mQueue.size() == 0)
        return false;

    *msg = mQueue.front();
    mQueue.pop_front();
    mCvEmpty.signal();

    return true;
}

bool MsgQ::peek(Msg* msg, int timeoutMs)
{
    Lock lock(mLock);

    while (mQueue.size() == 0 && !mEOS)
    {
        if (timeoutMs == 0)
            return false;
        else if (timeoutMs == -1) // Infinite
            mCvFull.wait(mLock);
        else
        {
            if (!mCvFull.wait(mLock, timeoutMs))
                return false;
        }
    }

    if (mEOS)
        return false;

    if (mQueue.size() == 0)
        return false;

    *msg = mQueue.front();

    return true;
}

void MsgQ::remove(int what)
{
    Lock lock(mLock);

    std::deque<Msg>::iterator it = mQueue.begin();

    while(it != mQueue.end())
    {
        if ((*it).what == what)
            it = mQueue.erase(it);
        else
            it++;
    }
    mCvFull.signal();
    mCvEmpty.signal();
}

void MsgQ::flush()
{
    Lock lock(mLock);

    mQueue.clear();

    mCvFull.broadcast();
    mCvEmpty.broadcast();
}

void MsgQ::setEOS(bool eos)
{
    Lock lock(mLock);
    mEOS = eos;

    mCvFull.broadcast();
    mCvEmpty.broadcast();
}
