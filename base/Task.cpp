/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Task.h"

#include <unistd.h>

#include "Log.h"

Task::Task()
     : mPriority(0),
       mName("Task"),
       mId(-1),
       mState(TASK_STATE_INIT)
{
}

Task::Task(int priority)
     : mPriority(priority),
       mName("Task"),
       mId(-1),
       mState(TASK_STATE_INIT)
{
}

Task::Task(const std::string& name)
     : mPriority(0),
       mName(name),
       mId(-1),
       mState(TASK_STATE_INIT)
{
}

Task::Task(int priority, const std::string& name)
     : mPriority(priority),
       mName(name),
       mId(-1),
       mState(TASK_STATE_INIT)
{
}

Task::~Task()
{
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
        LOGE("onPreStart() failed !");
        return false;
    }

    pthread_attr_init(&attr);

    // Setting Priority
    if (mPriority > 0)
    {
        pthread_attr_setinheritsched (&attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
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

void Task::sleep(int msec)
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
