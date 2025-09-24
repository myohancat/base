/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __QUEUE_H_
#define __QUEUE_H_

#include "Mutex.h"
#include "CondVar.h"

template<typename T, int capacity> class Queue
{
public:
    Queue() : mSize(0),
              mFront(0),
              mRear(0),
              mEOS(false)
    {
        mBuffer = new T[capacity];
        mCapacity = capacity;
    }

    virtual ~Queue()
    {
        delete[] mBuffer;
    }

    bool put(const T t, int timeoutMs = -1)
    {
        Lock lock(mLock);

        while (mSize == mCapacity && !mEOS)
        {
            if (timeoutMs == -1)
                mCvFull.wait(mLock);
            else if (timeoutMs == 0)
                return false;
            else
            {
                if (!mCvFull.wait(mLock, timeoutMs))
                    return false;
            }
        }

        if (mSize == mCapacity)
            return false;

        if (mEOS)
            return false;

        _put(t);
        mCvEmpty.signal();

        return true;
    }

    bool putForce(T t)
    {
        Lock lock(mLock);

        if (mEOS)
            return false;

        if (mSize == mCapacity)
        {
            T t1;
            _get(&t1);
            dispose(t1);
        }

        _put(t);
        mCvEmpty.signal();

        return true;
    }

    bool get(T* t, int timeoutMs = -1)
    {
        Lock lock(mLock);

        while (mSize == 0 && !mEOS)
        {
            if (timeoutMs == -1)
                mCvEmpty.wait(mLock);
            else if (timeoutMs == 0)
                return false;
            else
            {
                if (!mCvEmpty.wait(mLock, timeoutMs))
                    return false;
            }
        }

        if (mSize == 0)
            return false;

        if (mEOS)
            return false;

        _get(t);
        mCvFull.signal();

        return true;
    }

    void flush()
    {
        Lock lock(mLock);
        for (size_t ii = 0; ii < mSize; ii++)
        {
            int index = (mFront + ii) % mCapacity;
            dispose(mBuffer[index]);
        }

        mSize = 0;
        mRear = 0;
        mFront = 0;

        mCvFull.signal();
        mCvEmpty.signal();
    }

    void setEOS(bool eos)
    {
        Lock lock(mLock);
        mEOS = eos;

        mCvFull.signal();
        mCvEmpty.signal();
    }

    size_t  size();
    bool    isEOS();
    bool    isEmpty();
    bool    isFull();

protected:
    void _put(T t);
    void _get(T* t);

    /* MUST OVERRIDE to free T */
    virtual void dispose(UNUSED_PARAM T d) { }

protected:
    T*      mBuffer;

    size_t  mCapacity;
    size_t  mSize;
    size_t  mFront;
    size_t  mRear;

    bool    mEOS;

    Mutex   mLock;
    CondVar mCvFull;
    CondVar mCvEmpty;
};

template<typename T, int capacity>
inline size_t Queue<T, capacity>::size()
{
    Lock lock(mLock);

    return mSize;
}

template<typename T, int capacity>
inline bool Queue<T, capacity>::isEmpty()
{
    Lock lock(mLock);

    return mSize == 0;
}

template<typename T, int capacity>
inline bool Queue<T, capacity>::isFull()
{
    Lock lock(mLock);

    return mSize == mCapacity;
}

template<typename T, int capacity>
inline bool Queue<T, capacity>::isEOS()
{
    Lock lock(mLock);

    return mEOS;
}

template<typename T, int capacity>
inline void Queue<T, capacity>::_put(T t)
{
    mBuffer[mRear] = t;
    mRear = (mRear + 1) % mCapacity;
    mSize ++;
}

template<typename T, int capacity>
inline void Queue<T, capacity>::_get(T* t)
{
    *t = mBuffer[mFront];
    mFront = (mFront + 1) % mCapacity;
    mSize --;
}

#endif /* __QUEUE_H_ */
