/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>

class ByteRingBuffer
{
public:
    ByteRingBuffer(size_t capacity);
    ~ByteRingBuffer();

    ByteRingBuffer(const ByteRingBuffer&) = delete;
    ByteRingBuffer& operator=(const ByteRingBuffer&) = delete;

    size_t write(const uint8_t* buf, size_t len);
    size_t read(uint8_t* buf, size_t len);

    bool read8(uint8_t* val);
    bool read16(uint16_t* val);
    bool read32(uint32_t* val);

    bool peek8(size_t offset, uint8_t* val);
    bool peek16(size_t offset, uint16_t* val);
    bool peek32(size_t offset, uint32_t* val);

    size_t capacity() const;
    size_t size() const;
    size_t available() const;

private:
    size_t _peek(size_t offset, uint8_t* data, size_t len);
    bool   _drop(size_t size);

    template <typename T>
    bool _peekType(size_t offset, T* val, bool bigEndian)
    {
        uint8_t temp[sizeof(T)];
        if (_peek(offset, temp, sizeof(T)) != sizeof(T))
            return false;

        if (val)
        {
            *val = 0;
            for (size_t i = 0; i < sizeof(T); ++i)
            {
                if (bigEndian)
                    *val |= (static_cast<T>(temp[i]) << (8 * (sizeof(T) - 1 - i)));
                else
                    *val |= (static_cast<T>(temp[i]) << (8 * i));
            }
        }
        return true;
    }

    template <typename T>
    bool _readType(T* val, bool bigEndian)
    {
        if (_peekType(0, val, bigEndian))
        {
            _drop(sizeof(T));
            return true;
        }
        return false;
    }

private:
    mutable std::mutex mLock;

    uint8_t* mData;
    size_t   mCapacity;
    size_t   mSize;
    size_t   mFront;
    size_t   mRear;
};

inline size_t ByteRingBuffer::capacity() const
{
    std::lock_guard<std::mutex> lock(mLock);
    return mCapacity;
}

inline size_t ByteRingBuffer::size() const
{
    std::lock_guard<std::mutex> lock(mLock);
    return mSize;
}

inline size_t ByteRingBuffer::available() const
{
    std::lock_guard<std::mutex> lock(mLock);
    return mCapacity - mSize;
}

inline bool ByteRingBuffer::read8(uint8_t* val)
{
    std::lock_guard<std::mutex> lock(mLock);
    return _readType<uint8_t>(val, false);
}

inline bool ByteRingBuffer::read16(uint16_t* val)
{
    std::lock_guard<std::mutex> lock(mLock);
    return _readType<uint16_t>(val, false);
}

inline bool ByteRingBuffer::read32(uint32_t* val)
{
    std::lock_guard<std::mutex> lock(mLock);
    return _readType<uint32_t>(val, false);
}

inline bool ByteRingBuffer::peek8(size_t offset, uint8_t* val)
{
    std::lock_guard<std::mutex> lock(mLock);
    return _peekType<uint8_t>(offset, val, false);
}

inline bool ByteRingBuffer::peek16(size_t offset, uint16_t* val)
{
    std::lock_guard<std::mutex> lock(mLock);
    return _peekType<uint16_t>(offset, val, false);
}

inline bool ByteRingBuffer::peek32(size_t offset, uint32_t* val)
{
    std::lock_guard<std::mutex> lock(mLock);
    return _peekType<uint32_t>(offset, val, false);
}
