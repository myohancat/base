/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#include "Task.h"

#include <unistd.h>
#include <sched.h>
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

    if (mState == TaskState::Exited)
    {
        pthread_join(mId, nullptr);
        return;
    }

    /*
     * Derived classes should call stop() in their own destructor
     * before their members are destroyed.
     * This stop() is only a last-resort cleanup.
     */
    if (mState != TaskState::Idle)
    {
        LOGE("Derived class must stop() before destoryed.");
        abort();
    }
}

void Task::setCpuAffinity(int cpuid)
{
    std::lock_guard<std::mutex> lock(mLock);

    if (cpuid != -1)
    {
        if (mState != TaskState::Idle)
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

    if (mState != TaskState::Idle && mState != TaskState::Exited)
    {
        LOGW("task %s is already running", mName.c_str());
        return false;
    }

    if (mState == TaskState::Exited)
    {
        pthread_join(mId, NULL);
        mState = TaskState::Idle;
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

    mState = TaskState::Running;

    mWakeupRequested = false;
    if (pthread_create(&mId, &attr, _task_proc_priv, this) != 0)
    {
        LOGE("pthread create failed !");
        pthread_attr_destroy(&attr);
        mState = TaskState::Idle;
        return false;
    }
    pthread_attr_destroy(&attr);

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

    onPostStart();

    return true;
}

void Task::stop()
{
    std::unique_lock<std::mutex> lock(mLock);

    if (mState == TaskState::Idle || mState == TaskState::Stopping)
        return;

    if (pthread_equal(pthread_self(), mId))
    {
        LOGE("You must not call stop() directly.");
        return;
    }

    if (mState != TaskState::Exited)
        mState = TaskState::Stopping;

    onPreStop();

    mWakeupRequested = true;
    mCvSleep.notify_all();

    mLock.unlock();
    pthread_join(mId, NULL);
    mLock.lock();

    mState = TaskState::Idle;
    onPostStop();
}

void Task::msleep(int msec)
{
    if (msec <= 0)
        return;

    {
        std::unique_lock<std::mutex> lock(mLock);

        if (pthread_equal(pthread_self(), mId))
        {
            if (mState == TaskState::Stopping)
                return;

            mCvSleep.wait_for(lock, std::chrono::milliseconds(msec), [this]() {
                return mState == TaskState::Stopping || mWakeupRequested;
            });
            mWakeupRequested = false;
            return;
        }
    }

    // not in task loop
    usleep(msec *1000);
}

void Task::wakeup()
{
    std::lock_guard<std::mutex> lock(mLock);
    mWakeupRequested = true;
    mCvSleep.notify_all();
}

void* Task::_task_proc_priv(void* param)
{
    Task* pThis = static_cast<Task*>(param);

    pThis->run();

    {
        std::lock_guard<std::mutex> lock(pThis->mLock);
        if (pThis->mState == TaskState::Running || pThis->mState == TaskState::Stopping)
            pThis->mState = TaskState::Exited;
    }
    return NULL;
}
