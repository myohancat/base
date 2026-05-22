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
#include <string>
#include <cstdint>

#include <pthread.h>

/* Usage:
 *
 * class Foo : public Task
 * {
 * protected:
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

    bool shouldRun() const;

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
    enum class TaskState : std::uint8_t
    {
        Idle,
        Running,
        Stopping,
        Exited
    };

    std::mutex              mLock;
    std::condition_variable mCvSleep;
    std::atomic<TaskState>  mState;
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

inline bool Task::shouldRun() const
{
    return mState.load() == TaskState::Running;
}
