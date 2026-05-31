/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#include "WorkerThread.h"

#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <ctime>

#include "Log.h"

WorkerThread::WorkerThread(const std::string& name, int priority, int cpuid)
    : mWorker(nullptr),
      mPriority(priority),
      mCpuId(cpuid),
      mName(name),
      mId{},
      mState(ThreadState::Idle),
      mWakeupRequested(false)
{
}

WorkerThread::~WorkerThread()
{
    ThreadState state = mState.load();

    if (state == ThreadState::Idle)
        return;

    if (pthread_equal(pthread_self(), mId) != 0)
    {
        LOGE("[%s] WorkerThread destroyed from its own worker thread. State: %d",
             mName.c_str(),
             static_cast<int>(state));
        std::abort();
    }

    stop();
}

void WorkerThread::setCpuAffinity(int cpuid)
{
    std::lock_guard<std::mutex> lifecycleLock(mLifecycleLock);

    mCpuId = cpuid;

    if (cpuid == -1)
        return;

    ThreadState state = mState.load();

    if (state != ThreadState::Running && state != ThreadState::Stopping)
        return;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuid, &cpuset);

    int ret = pthread_setaffinity_np(mId, sizeof(cpu_set_t), &cpuset);
    if (ret != 0)
    {
        LOGW("[%s] Cannot pthread_setaffinity_np: %s", mName.c_str(), std::strerror(ret));
    }
}

bool WorkerThread::start(IWorker& worker)
{
    std::lock_guard<std::mutex> lifecycleLock(mLifecycleLock);

    ThreadState state = mState.load();

    if (state == ThreadState::Exited)
    {
        pthread_join(mId, nullptr);

        std::lock_guard<std::mutex> lock(mLock);
        mWorker = nullptr;
        mState.store(ThreadState::Idle);
        state = ThreadState::Idle;
    }

    if (state != ThreadState::Idle)
    {
        LOGW("[%s] task is already running or stopping. State: %d", mName.c_str(), static_cast<int>(state));
        return false;
    }

    pthread_attr_t attr;
    int ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        LOGW("[%s] Cannot pthread_attr_init: %s",
             mName.c_str(),
             std::strerror(ret));
        return false;
    }

    if (mPriority > 0)
    {
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (ret != 0)
        {
            LOGW("[%s] Cannot pthread_attr_setinheritsched: %s",
                 mName.c_str(),
                 std::strerror(ret));
        }

        ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        if (ret != 0)
        {
            LOGW("[%s] Cannot pthread_attr_setschedpolicy: %s",
                 mName.c_str(),
                 std::strerror(ret));
        }

        sched_param params;
        params.sched_priority = mPriority;

        ret = pthread_attr_setschedparam(&attr, &params);
        if (ret != 0)
        {
            LOGW("[%s] Cannot pthread_attr_setschedparam: %s",
                 mName.c_str(),
                 std::strerror(ret));
        }
    }

    if (!worker.onPreStart())
    {
        LOGE("[%s] onPreStart() failed", mName.c_str());
        pthread_attr_destroy(&attr);
        return false;
    }

    {
        std::lock_guard<std::mutex> sleepLock(mSleepLock);
        mWakeupRequested = false;
    }

    {
        std::lock_guard<std::mutex> lock(mLock);
        mWorker = &worker;
        mState.store(ThreadState::Running);
    }

    ret = pthread_create(&mId, &attr, _task_proc_priv, this);
    pthread_attr_destroy(&attr);

    if (ret != 0)
    {
        LOGE("[%s] pthread_create() failed: %s", mName.c_str(), std::strerror(ret));

        std::lock_guard<std::mutex> lock(mLock);
        mWorker = nullptr;
        mState.store(ThreadState::Idle);
        return false;
    }

    if (mCpuId != -1)
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(mCpuId, &cpuset);

        ret = pthread_setaffinity_np(mId, sizeof(cpu_set_t), &cpuset);
        if (ret != 0)
        {
            LOGW("[%s] Cannot pthread_setaffinity_np: %s",
                 mName.c_str(),
                 std::strerror(ret));
        }
    }

    char nameBuf[16];
    std::strncpy(nameBuf, mName.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';

    ret = pthread_setname_np(mId, nameBuf);
    if (ret != 0)
    {
        LOGW("[%s] Cannot pthread_setname_np: %s", mName.c_str(), std::strerror(ret));
    }

    return true;
}

void WorkerThread::stop()
{
    ThreadState state = mState.load();

    if (state == ThreadState::Idle)
        return;

    /* Self stop in run(), without joining */
    if ((state == ThreadState::Running || state == ThreadState::Stopping) &&
        pthread_equal(pthread_self(), mId) != 0)
    {
        ThreadState expected = ThreadState::Running;

        if (mState.compare_exchange_strong(expected, ThreadState::Stopping))
        {
            IWorker* worker = mWorker;
            if (worker != nullptr)
                worker->onPreStop();
        }

        wakeup();
        return;
    }

    std::lock_guard<std::mutex> lifecycleLock(mLifecycleLock);

    state = mState.load();

    if (state == ThreadState::Idle)
        return;

    bool callPreStop = false;

    if (state == ThreadState::Running)
    {
        mState.store(ThreadState::Stopping);
        if (mWorker)
            mWorker->onPreStop();
    }

    wakeup();

    pthread_join(mId, nullptr);

    {
        std::lock_guard<std::mutex> lock(mLock);
        mWorker = nullptr;
        mState.store(ThreadState::Idle);
    }
}

void WorkerThread::msleep(int msec)
{
    if (msec <= 0)
        return;

    ThreadState state = mState.load();

    if ((state == ThreadState::Running || state == ThreadState::Stopping) &&
        pthread_equal(pthread_self(), mId) != 0)
    {
        std::unique_lock<std::mutex> lock(mSleepLock);

        if (mState.load() == ThreadState::Stopping)
            return;

        mCvSleep.wait_for(lock, std::chrono::milliseconds(msec), [this]() {
            return mState.load() == ThreadState::Stopping || mWakeupRequested;
        });

        mWakeupRequested = false;
        return;
    }

    timespec req;
    req.tv_sec = msec / 1000;
    req.tv_nsec = static_cast<long>(msec % 1000) * 1000000L;

    while (nanosleep(&req, &req) != 0)
    {
        if (errno != EINTR)
            break;
    }
}

void WorkerThread::wakeup()
{
    {
        std::lock_guard<std::mutex> lock(mSleepLock);
        mWakeupRequested = true;
    }

    mCvSleep.notify_all();
}

void* WorkerThread::_task_proc_priv(void* param)
{
    WorkerThread* pThis = static_cast<WorkerThread*>(param);

    IWorker* worker = pThis->mWorker;

    if (worker != nullptr)
    {
        worker->onPostStart();
        worker->run();
        worker->onPostStop();
    }

    ThreadState state = pThis->mState.load();

    if (state == ThreadState::Running ||
        state == ThreadState::Stopping)
    {
        pThis->mState.store(ThreadState::Exited);
    }

    return nullptr;
}
