/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#pragma once

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>

template<typename T, size_t capacity>
class Queue
{
    static_assert(capacity > 0, "Queue capacity must be greater than 0");

protected:
    static constexpr size_t kCapacity = capacity;

public:
    Queue()
        : mSize(0),
          mFront(0),
          mRear(0),
          mEOS(false)
    {
    }

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue& operator=(Queue&&) = delete;

    virtual ~Queue()
    {
        flush();
    }

    bool put(const T t, int timeoutMs = -1)
    {
        std::unique_lock<std::mutex> lock(mLock);

        auto condition = [this] {
            return (mSize < kCapacity) || mEOS;
        };

        if (timeoutMs == 0)
        {
            if (!condition())
                return false;
        }
        else if (timeoutMs == -1)
        {
            mCondVarFull.wait(lock, condition);
        }
        else
        {
            if (!mCondVarFull.wait_for(
                    lock,
                    std::chrono::milliseconds(timeoutMs),
                    condition))
            {
                return false;
            }
        }

        if (mEOS)
            return false;

        _put(t);
        mCondVarEmpty.notify_one();

        return true;
    }

    bool putForce(T t)
    {
        std::unique_lock<std::mutex> lock(mLock);

        if (mEOS)
            return false;

        if (mSize == kCapacity)
        {
            T oldValue;
            _get(&oldValue);
            dispose(oldValue);
        }

        _put(t);
        mCondVarEmpty.notify_one();

        return true;
    }

    bool get(T* t, int timeoutMs = -1)
    {
        if (t == nullptr)
            return false;

        std::unique_lock<std::mutex> lock(mLock);

        auto condition = [this] {
            return (mSize > 0) || mEOS;
        };

        if (timeoutMs == 0)
        {
            if (!condition())
                return false;
        }
        else if (timeoutMs == -1)
        {
            mCondVarEmpty.wait(lock, condition);
        }
        else
        {
            if (!mCondVarEmpty.wait_for(
                    lock,
                    std::chrono::milliseconds(timeoutMs),
                    condition))
            {
                return false;
            }
        }

        if (mSize == 0 && mEOS)
            return false;

        _get(t);
        mCondVarFull.notify_one();

        return true;
    }

    void flush()
    {
        std::lock_guard<std::mutex> lock(mLock);

        for (size_t ii = 0; ii < mSize; ++ii)
        {
            const size_t index = (mFront + ii) % kCapacity;
            dispose(mBuffer[index]);
        }

        mSize = 0;
        mFront = 0;
        mRear = 0;

        mCondVarFull.notify_all();
    }

    void setEOS(bool eos)
    {
        std::lock_guard<std::mutex> lock(mLock);

        mEOS = eos;

        mCondVarFull.notify_all();
        mCondVarEmpty.notify_all();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mLock);
        return mSize;
    }

    bool isEOS()
    {
        std::lock_guard<std::mutex> lock(mLock);
        return mEOS;
    }

    bool isEmpty()
    {
        std::lock_guard<std::mutex> lock(mLock);
        return mSize == 0;
    }

    bool isFull()
    {
        std::lock_guard<std::mutex> lock(mLock);
        return mSize == kCapacity;
    }

protected:
    void _put(T t)
    {
        mBuffer[mRear] = t;
        mRear = (mRear + 1) % kCapacity;
        ++mSize;
    }

    void _get(T* t)
    {
        *t = mBuffer[mFront];
        mFront = (mFront + 1) % kCapacity;
        --mSize;
    }

    /*
     * Ownership policy:
     *
     * This queue is intended for embedded-style raw pointer / C handle usage.
     *
     * - put() stores the item as-is.
     * - On successful get(), the caller becomes responsible for the item.
     * - Items discarded by putForce() or flush() are passed to dispose().
     * - _get() does not clear the internal slot after reading.
     *   Valid queue contents are controlled only by mSize, mFront, and mRear.
     *
     * For GStreamer types, override dispose() and call the matching unref API.
     *
     * Example:
     *
     *   class GstSampleQueue : public Queue<GstSample*, 8>
     *   {
     *   protected:
     *       void dispose(GstSample* sample) override
     *       {
     *           if (sample)
     *               gst_sample_unref(sample);
     *       }
     *   };
     */
    virtual void dispose(T)
    {
    }

protected:
    std::array<T, kCapacity> mBuffer;

    size_t mSize;
    size_t mFront;
    size_t mRear;

    bool mEOS;

    std::mutex mLock;
    std::condition_variable mCondVarFull;
    std::condition_variable mCondVarEmpty;
};
