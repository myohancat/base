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
#include <climits>
#include <chrono>

#include <pthread.h>

/*
 * Usage:
 *
 * class Foo : public IWorker
 * {
 * public:
 *     bool start() { return mThread.start(*this); }
 *     void stop()  { mThread.stop(); }
 *
 * protected:
 *     void run() noexcept override
 *     {
 *         while (mThread.shouldRun())
 *         {
 *             // TODO
 *             mThread.msleep(1000);
 *         }
 *     }
 *
 * private:
 *     // If relying on WorkerThread destructor auto-stop,
 *     // resources used by run() must be declared before WorkerThread.
 *     Buffer mBuffer;   // resources
 *     WorkerThread mThread; // worker thread
 * };
 */

class IWorker
{
public:
    virtual ~IWorker() { }

    virtual void run() noexcept = 0;

    /*
     * Callback thread context:
     *
     * onPreStart()  : called by the thread that calls start(), before pthread_create().
     * onPostStart() : called by the worker thread, before run().
     * onPreStop()   : called by the thread that requests stop().
     * onPostStop()  : called by the worker thread, after run().
     */
    virtual bool onPreStart()  { return true; }
    virtual void onPostStart() { }
    virtual void onPreStop()   { }
    virtual void onPostStop()  { }
};

class WorkerThread
{
public:
    WorkerThread(const std::string& name = "Worker",
                 int priority = -1, /* default : linux priority */
                 int cpuid = -1);

    ~WorkerThread();

    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;

    void setCpuAffinity(int cpuid);
    int  getCpuAffinity() const;

    bool start(IWorker& worker);
    void stop();

    void sleep(int sec);
    void msleep(int msec);

    void wakeup();

    bool shouldRun() const;

private:
    enum class ThreadState : std::uint8_t
    {
        Idle,
        Running,
        Stopping,
        Exited
    };

private:
    static void* _task_proc_priv(void* param);

private:
    IWorker*    mWorker;
    int         mPriority;
    int         mCpuId;
    std::string mName;
    pthread_t   mId;

    std::mutex               mLifecycleLock;
    std::mutex               mLock;
    std::atomic<ThreadState> mState;

    bool                     mWakeupRequested;
    mutable std::mutex       mSleepLock;
    std::condition_variable  mCvSleep;
};

inline int WorkerThread::getCpuAffinity() const
{
    return mCpuId;
}

inline void WorkerThread::sleep(int sec)
{
    if (sec <= 0)
        return;

    if (sec > INT_MAX / 1000)
        sec = INT_MAX / 1000;

    msleep(sec * 1000);
}

inline bool WorkerThread::shouldRun() const
{
    return mState.load() == ThreadState::Running;
}
