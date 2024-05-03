/**
 * My simple event loop source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "MsgQ.h"
#include "Log.h"

MsgQ::MsgQ() : mCapacity(DEF_QUEUE_SIZE), mEOS(false) { }
MsgQ::MsgQ(size_t capacity) : mCapacity(capacity), mEOS(false) { }
MsgQ::~MsgQ() { }

int MsgQ::send(const Msg& msg, int timeoutMs)
{
    Lock lock(mLock);

    while (mQueue.size() >= mCapacity && !mEOS)
    {
        if (timeoutMs == 0)
            return MSGQ_RET_FULL;
        else if (timeoutMs == -1) // Infinite
            mCvEmpty.wait(mLock);
        else
        {
            if (!mCvEmpty.wait(mLock, timeoutMs))
                return MSGQ_RET_TIMEOUT;
        }
    }

    if (mEOS)
        return MSGQ_RET_EOS;

    if (mQueue.size() >= mCapacity)
        return MSGQ_RET_FULL;

    mQueue.push_back(msg);
    mCvFull.signal();

    return MSGQ_RET_SUCCESS;
}

int MsgQ::recv(Msg& msg, int timeoutMs)
{
    Lock lock(mLock);

    while (mQueue.size() == 0 && !mEOS)
    {
        if (timeoutMs == 0)
            return MSGQ_RET_EMPTY;
        else if (timeoutMs == -1) // Infinite
            mCvFull.wait(mLock);
        else
        {
            if (!mCvFull.wait(mLock, timeoutMs))
                return MSGQ_RET_TIMEOUT;
        }
    }

    if (mEOS)
        return MSGQ_RET_EOS;

    if (mQueue.size() == 0)
        return MSGQ_RET_EMPTY;

    msg = mQueue.front();
    mQueue.pop_front();
    mCvEmpty.signal();

    return MSGQ_RET_SUCCESS;
}

int MsgQ::peek(Msg& msg, int timeoutMs)
{
    Lock lock(mLock);

    while (mQueue.size() == 0 && !mEOS)
    {
        if (timeoutMs == 0)
            return MSGQ_RET_EMPTY;
        else if (timeoutMs == -1) // Infinite
            mCvFull.wait(mLock);
        else
        {
            if (!mCvFull.wait(mLock, timeoutMs))
                return MSGQ_RET_TIMEOUT;
        }
    }

    if (mEOS)
        return MSGQ_RET_EOS;

    if (mQueue.size() == 0)
        return MSGQ_RET_EMPTY;

    msg = mQueue.front();

    return MSGQ_RET_SUCCESS;
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

    mCvFull.signal();
    mCvEmpty.signal();
}

void MsgQ::setEOS(bool eos)
{
    Lock lock(mLock);
    mEOS = eos;

    mCvFull.signal();
    mCvEmpty.signal();
}
