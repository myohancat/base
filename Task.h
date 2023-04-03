/**
 * My simple event loop source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __TASK_H_
#define __TASK_H_

#include "Mutex.h"
#include "CondVar.h"

#include <pthread.h>
#include <string>

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
    Task();
    Task(int priority);
    Task(const std::string& name);
    Task(int priority, const std::string& name);
    virtual ~Task();

    bool start();
    void stop();

    void sleep(int msec);
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
    std::string mName;
    pthread_t   mId;

    Mutex       mMutex;
    CondVar     mCond;
    TaskState   mState;

private:
    static void* _task_proc_priv(void* param);

};

#endif /* __TASK_H_ */
