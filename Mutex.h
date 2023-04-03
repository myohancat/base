/**
 * My simple event loop source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __MUTEX_H_
#define __MUTEX_H_

#include "Types.h"

#include <pthread.h>

class ILockable
{
public:
    virtual ~ILockable() { }

    virtual void lock()   = 0;
    virtual void unlock() = 0;
};

class Lock
{
public:
    inline Lock(ILockable& lock) : mLock(lock)  { mLock.lock(); }
    inline Lock(ILockable* lock) : mLock(*lock) { mLock.lock(); }
    inline ~Lock() { mLock.unlock(); }

private:
    ILockable& mLock;
};

class Mutex : public ILockable
{
friend class CondVar;

public:
    Mutex(bool isRecusive = false);
    ~Mutex();

    void lock();
    void unlock();

private:
    Mutex(const Mutex&) = delete;
    Mutex& operator= (const Mutex&) = delete;

private:
    pthread_mutex_t mId;
};

class RecursiveMutex : public Mutex
{
public:
    RecursiveMutex() : Mutex(true) { }

};

#endif /* __MUTEX_H_ */
