/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Task.h"

#include <unistd.h>
#include <sched.h>
#include "Log.h"

#include "MainLoop.h"

class AsyncOnceTask : public Task
{
public:
    AsyncOnceTask() : Task("AsyncOnceTask")
    {
    }

    ~AsyncOnceTask()
    {
        stop();
    }

    void execute(const std::function<void()> &func)
    {
        mFunc = func;
        start();
    }

protected:
    void run()
    {
        mFunc();

        AsyncOnceTask* ptr = this;
        MainLoop::getInstance().post([ptr] {
            delete ptr;
        });
    }

    std::function<void()> mFunc;
};

void Task::asyncOnce(const std::function<void()> &func)
{
    AsyncOnceTask* task = new AsyncOnceTask();
    task->execute(func);
}

Task::Task()
     : mPriority(0),
       mCpuId(-1),
       mName("Task"),
       mId(-1),
       mState(TASK_STATE_INIT)
{
}

Task::Task(int priority, int cpuid)
     : mPriority(priority),
       mCpuId(cpuid),
       mName("Task"),
       mId(-1),
       mState(TASK_STATE_INIT)
{
}

Task::Task(const std::string& name, int cpuid)
     : mPriority(0),
       mCpuId(cpuid),
       mName(name),
       mId(-1),
       mState(TASK_STATE_INIT)
{
}

Task::Task(int priority, const std::string& name, int cpuid)
     : mPriority(priority),
       mCpuId(cpuid),
       mName(name),
       mId(-1),
       mState(TASK_STATE_INIT)
{
}

Task::~Task()
{
    // stop();
}

void Task::setCpuAffinity(int cpuid)
{
    Lock lock(mMutex);

    if (mId != (pthread_t)-1)
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpuid, &cpuset);
        pthread_setaffinity_np(mId, sizeof(cpu_set_t), &cpuset);
    }

    mCpuId = cpuid;
}

bool Task::start()
{
    Lock lock(mMutex);

    pthread_attr_t attr;

    if (mState != TASK_STATE_INIT && mState != TASK_STATE_STOPPED)
    {
        LOGW("task %s is already running", mName.c_str());
        return false;
    }

    if (mState == TASK_STATE_STOPPED)
    {
        mMutex.unlock();
        stop();
        mMutex.lock();
    }

    if (!onPreStart())
    {
        LOGE("[%s] onPreStart() failed !", mName.c_str());
        return false;
    }

    pthread_attr_init(&attr);

    // Setting Priority
    if (mPriority > 0)
    {
        pthread_attr_setinheritsched (&attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        struct sched_param params;
        params.sched_priority = mPriority;
        pthread_attr_setschedparam(&attr, &params);
    }

    if (pthread_create (&mId, &attr, _task_proc_priv, this) != 0)
    {
        LOGE("pthread create failed !");
        pthread_attr_destroy(&attr);
        return false;
    }

    if (mCpuId != -1)
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(mCpuId, &cpuset);
        pthread_setaffinity_np(mId, sizeof(cpu_set_t), &cpuset);
    }

    pthread_attr_destroy(&attr);

    // Setting Thread Name
    {
        char name[16];
        strncpy(name, mName.c_str(), 15);
        name[15] = 0;
        pthread_setname_np(mId, name);
    }

    onPostStart();

    mState = TASK_STATE_RUNNING;

    return true;
}

void Task::stop()
{
    Lock lock(mMutex);

    if (mState != TASK_STATE_RUNNING && mState != TASK_STATE_STOPPED)
        return;

    onPreStop();

    mState = TASK_STATE_STOPPING;
    wakeup();

    if (mId != (pthread_t)-1)
    {
        mMutex.unlock();
        pthread_join(mId, NULL);
        mMutex.lock();
        mId = -1;
    }

    mState = TASK_STATE_INIT;
    onPostStop();
}

void Task::msleep(int msec)
{
    Lock lock(mMutex);

    if (mState == TASK_STATE_STOPPING)
        return;

    if (mId == pthread_self())
        mCond.wait(mMutex, msec);
    else
        usleep(msec * 1000);
}

TaskState Task::state()
{
    return mState;
}

void Task::wakeup()
{
    mCond.signal();
}

void* Task::_task_proc_priv(void* param)
{
    Task* pThis = static_cast<Task*>(param);

    pThis->run();

    pThis->mMutex.lock();
    pThis->mState = TASK_STATE_STOPPED;
    pThis->mMutex.unlock();

    return NULL;
}
