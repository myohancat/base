/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __TASK_H_
#define __TASK_H_

#include "Mutex.h"
#include "CondVar.h"

#include <pthread.h>
#include <string>
#include <functional>

typedef enum
{
    TASK_STATE_INIT,
    TASK_STATE_RUNNING,
    TASK_STATE_STOPPING,
    TASK_STATE_STOPPED,
} TaskState;

class Task
{
public:
    static void asyncOnce(const std::function<void()> &func);

    Task();
    Task(int priority, int cpuid = -1);
    Task(const std::string& name, int cpuid = -1);
    Task(int priority, const std::string& name, int cpuid = -1);
    virtual ~Task();

    void setCpuAffinity(int cpuid);
    int  getCpuAffinity() const;

    bool start();
    void stop();

    void sleep(int sec);
    void msleep(int msec);

    void wakeup();

    TaskState state();

protected:
    virtual void run() = 0; // MUST IMPLEMENT.

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
    Mutex       mMutex;
    CondVar     mCond;
    TaskState   mState;

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

#endif /* __TASK_H_ */
