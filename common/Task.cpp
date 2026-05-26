/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#include "Task.h"

#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <cstring>
#include "Log.h"

Task::Task()
     : mPriority(0),
       mCpuId(-1),
       mName("Task"),
       mId{},
       mState(TaskState::Idle),
       mWakeupRequested(false)
{
}

Task::Task(int priority, int cpuid)
     : mPriority(priority),
       mCpuId(cpuid),
       mName("Task"),
       mId{},
       mState(TaskState::Idle),
       mWakeupRequested(false)
{
}

Task::Task(const std::string& name, int cpuid)
     : mPriority(0),
       mCpuId(cpuid),
       mName(name),
       mId{},
       mState(TaskState::Idle),
       mWakeupRequested(false)
{
}

Task::Task(int priority, const std::string& name, int cpuid)
     : mPriority(priority),
       mCpuId(cpuid),
       mName(name),
       mId{},
       mState(TaskState::Idle),
       mWakeupRequested(false)
{
}

Task::~Task()
{
    std::lock_guard<std::mutex> lock(mLock);

    TaskState state = mState.load();

    if (state == TaskState::Exited)
    {
        pthread_join(mId, nullptr);
        mState.store(TaskState::Idle);
        return;
    }

    /*
     * Derived classes should call stop() in their own destructor
     * before their members are destroyed.
     * This stop() is only a last-resort cleanup.
     */
    if (state != TaskState::Idle)
    {
        LOGE("[%s] Derived class must call stop() before destroyed. State: %d",
             mName.c_str(),
             static_cast<int>(state));
        abort();
    }
}

void Task::setCpuAffinity(int cpuid)
{
    std::lock_guard<std::mutex> lock(mLock);

    if (cpuid != -1)
    {
        if (mState.load() != TaskState::Idle)
        {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(cpuid, &cpuset);
            pthread_setaffinity_np(mId, sizeof(cpu_set_t), &cpuset);
        }
    }

    mCpuId = cpuid;
}

bool Task::start()
{
    std::unique_lock<std::mutex> lock(mLock);

    TaskState state = mState.load();
    if (state == TaskState::Running || state == TaskState::Stopping)
    {
        LOGW("task %s is already running", mName.c_str());
        return false;
    }

    if (state == TaskState::Exited)
    {
        pthread_join(mId, NULL);
        mState.store(TaskState::Idle);
    }

    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0)
    {
        LOGW("[%s] Cannot pthread_attr_init.", mName.c_str());
        return false;
    }

    if (mPriority > 0)
    {
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        struct sched_param params;
        params.sched_priority = mPriority;
        if (pthread_attr_setschedparam(&attr, &params) != 0)
            LOGW("Cannot pthread_attr_setschedparam.");
    }

    if (!onPreStart())
    {
        LOGE("[%s] onPreStart() failed !", mName.c_str());
        pthread_attr_destroy(&attr);
        return false;
    }

    {
        std::lock_guard<std::mutex> sleepLock(mSleepLock);
        mWakeupRequested = false;
    }

    mState.store(TaskState::Running);
    int ret = pthread_create(&mId, &attr, _task_proc_priv, this);
    pthread_attr_destroy(&attr);

    if (ret != 0)
    {
        LOGE("[%s] pthread_create() failed: %s", mName.c_str(), std::strerror(ret));
        mState.store(TaskState::Idle);
        return false;
    }

    // CPU Affinity
    if (mCpuId != -1)
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(mCpuId, &cpuset);
        if (pthread_setaffinity_np(mId, sizeof(cpu_set_t), &cpuset) != 0)
            LOGW("Cannot pthread_setaffinity_np.");
    }

    // For Debugging (GDB)
    char nameBuf[16];
    strncpy(nameBuf, mName.c_str(), 15);
    nameBuf[15] = '\0';
    pthread_setname_np(mId, nameBuf);

    return true;
}

void Task::stop()
{
    TaskState current = mState.load();

    if (current == TaskState::Stopping &&
        pthread_equal(pthread_self(), mId))
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mLock);

    TaskState state = mState.load();

    if (state == TaskState::Idle)
        return;

    if (state == TaskState::Exited)
    {
        pthread_join(mId, nullptr);
        mState.store(TaskState::Idle);
        return;
    }

    if (state == TaskState::Running)
    {
        mState.store(TaskState::Stopping);
    }

    onPreStop();

    {
        std::lock_guard<std::mutex> sleepLock(mSleepLock);
        mWakeupRequested = true;
    }
    mCvSleep.notify_all();

    if (pthread_equal(pthread_self(), mId) != 0)
        return;

    pthread_join(mId, nullptr);

    mState.store(TaskState::Idle);
}

void Task::msleep(int msec)
{
    if (msec <= 0)
        return;

    {
        std::unique_lock<std::mutex> lock(mSleepLock);

        TaskState state = mState.load();
        if ((state == TaskState::Running || state == TaskState::Stopping) &&
             pthread_equal(pthread_self(), mId) != 0)
        {
            if (state == TaskState::Stopping)
                return;

            mCvSleep.wait_for(lock, std::chrono::milliseconds(msec), [this]() {
                return mState.load() == TaskState::Stopping || mWakeupRequested;
            });
            mWakeupRequested = false;
            return;
        }
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

void Task::wakeup()
{
    {
        std::lock_guard<std::mutex> lock(mSleepLock);
        mWakeupRequested = true;
    }
    mCvSleep.notify_all();
}

void* Task::_task_proc_priv(void* param)
{
    Task* pThis = static_cast<Task*>(param);

    pThis->onPostStart();

    pThis->run();

    pThis->onPostStop();

    TaskState state = pThis->mState.load();

    if (state == TaskState::Running || state == TaskState::Stopping)
    {
        pThis->mState.store(TaskState::Exited);
    }

    return NULL;
}
