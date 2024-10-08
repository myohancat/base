/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "CondVar.h"
#include "SysTime.h"

#include <errno.h>
#include <sys/time.h>

CondVar::CondVar()
{
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(&mId, &attr);
    pthread_condattr_destroy(&attr);
}

CondVar::~CondVar()
{
    pthread_cond_destroy(&mId);
}

void CondVar::wait(Mutex& mutex)
{
    pthread_cond_wait(&mId, &mutex.mId);
}

bool CondVar::wait(Mutex& mutex, int timeout)
{
    if (timeout < 0)
    {
        wait(mutex);
        return true;
    }

    int rc;
    struct timespec ts;

    uint64_t expireTime = SysTime::getTickCountMs() + timeout;

    ts.tv_sec  = expireTime / 1000;
    ts.tv_nsec = (expireTime % 1000) * 1000000L;

    rc = pthread_cond_timedwait(&mId, &mutex.mId, &ts);
    if (rc == 0)
        return true;

    if (rc == ETIMEDOUT)
        return false;

    return false;
}

void CondVar::signal()
{
    pthread_cond_signal(&mId);
}

void CondVar::broadcast()
{
    pthread_cond_broadcast(&mId);
}
