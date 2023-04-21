#include <unistd.h>
#include <signal.h>

#include "EventLoop.h"
#include "Log.h"
#include "Timer.h"
#include "Task.h"
#include "Mutex.h"

static void sig_handler(int signum)
{
    EventLoop::getInstance().terminate();
}

class MyTask : public Task
{
public:

protected:
    bool onPreStart()
    {
        mExitTask = false;
        return true;
    }

    void onPreStop()
    {
        mExitTask = true;
    }

    void run()
    {
        while(!mExitTask)
        {
            LOGD("Thread processed\n");
            sleep(10000);
        }
    }

protected:
    bool mExitTask = false;
    Mutex mLock;
};

class DemoTimerHandler : public ITimerHandler
{
public:
    void onTimerExpired(const ITimer* timer)
    {
        mSec ++;
        LOGD("%d Sec\n", mSec);
    }

private:
    int mSec = 0;
};

int main()
{
    LOGD("Hello\n");

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    MyTask task;
    task.start();

    Timer timer;
    DemoTimerHandler handler;
    timer.setHandler(&handler);
    timer.start(1000, true);

    while(EventLoop::getInstance().loop()) { }

    task.stop();
    timer.stop();

    return 0;
}
