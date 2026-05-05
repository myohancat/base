/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#include "ByteRingBuffer.h"

#include <cstring>
#include <algorithm>

ByteRingBuffer::ByteRingBuffer(size_t capacity)
              : mCapacity(capacity), mSize(0), mFront(0), mRear(0)
{
    mData = new uint8_t[mCapacity];
}

ByteRingBuffer::~ByteRingBuffer()
{
    delete[] mData;
}

size_t ByteRingBuffer::write(const uint8_t* buf, size_t len)
{
    std::lock_guard<std::mutex> lock(mLock);

    size_t can_write = std::min(len, mCapacity - mSize);
    if (can_write == 0) return 0;

    size_t first_part = std::min(can_write, mCapacity - mRear);
    memcpy(&mData[mRear], buf, first_part);

    if (can_write > first_part)
    {
        memcpy(&mData[0], &buf[first_part], can_write - first_part);
    }

    mRear = (mRear + can_write) % mCapacity;
    mSize += can_write;

    return can_write;
}

size_t ByteRingBuffer::read(uint8_t* buf, size_t len)
{
    std::lock_guard<std::mutex> lock(mLock);

    size_t bytesRead = _peek(0, buf, len);
    if (bytesRead > 0)
        _drop(bytesRead);

    return bytesRead;
}

size_t ByteRingBuffer::_peek(size_t offset, uint8_t* buf, size_t len)
{
    if (offset >= mSize || len == 0)
        return 0;

    size_t can_read = std::min(len, mSize - offset);
    size_t start_idx = (mFront + offset) % mCapacity;

    size_t first_part = std::min(can_read, mCapacity - start_idx);
    if (buf)
    {
        memcpy(buf, &mData[start_idx], first_part);
        if (can_read > first_part)
        {
            memcpy(&buf[first_part], &mData[0], can_read - first_part);
        }
    }

    return can_read;
}

bool ByteRingBuffer::_drop(size_t size)
{
    if (mSize < size)
        return false;

    mFront = (mFront + size) % mCapacity;
    mSize -= size;

    return true;
}
