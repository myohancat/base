/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <pthread.h>
#include <string>

/* Usage:
 *
 * class Foo : public Task
 * {
 * proteted:
 *     void run() override
 *     {
 *         while (shouldRun())
 *         {
 *             // TODO
 *             msleep(1000);
 *         }
 *     }
 * };
 */
typedef enum
{
    TASK_STATE_IDLE,
    TASK_STATE_RUNNING,
    TASK_STATE_STOPPING,
    TASK_STATE_EXITED,
} TaskState;

class Task
{
public:
    Task();
    Task(int priority, int cpuid = -1);
    Task(const std::string& name, int cpuid = -1);
    Task(int priority, const std::string& name, int cpuid = -1);
    virtual ~Task();

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    void setCpuAffinity(int cpuid);
    int  getCpuAffinity() const;

    bool start();
    void stop();

    void sleep(int sec);
    void msleep(int msec);

    void wakeup();

    bool shouldRun();

protected:
    virtual void run() = 0; // MUST IMPLEMENT.

    /* Important:
     * Don't call Task apis in Callback.
     */
    virtual bool onPreStart() { return true; }
    virtual void onPostStart() { }

    virtual void onPreStop() { }
    virtual void onPostStop() { }

protected:
    int         mPriority;
    int         mCpuId;
    std::string mName;
    pthread_t   mId;

private:
    std::mutex              mLock;
    std::condition_variable mCvSleep;
    TaskState               mState;
    bool                    mWakeupRequested;

private:
    static void* _task_proc_priv(void* param);
};

inline int Task::getCpuAffinity() const
{
    return mCpuId;
}

inline void Task::sleep(int sec)
{
    msleep(sec * 1000);
}

inline bool Task::shouldRun()
{
    std::lock_guard<std::mutex> lock(mLock);

    return mState == TASK_STATE_RUNNING;
}
