#include "Log.h"

#include <signal.h>

#include "MainLoop.h"

static void _sig_handler(int signum)
{
    (void)signum;

    MainLoop::getInstance().terminate();
}

#include "Timer.h"
class TimerTest : public ITimerHandler
{
public:
    TimerTest()
    {
        mTimer.setHandler(this);
    }

    ~TimerTest()
    {
        stop();
    }

    void start()
    {
        mTimer.start(1000, true);
    }

    void stop()
    {
        mTimer.stop();
    }

protected:
    void onTimerExpired(const ITimer* timer) override
    {
        (void)timer;

        LOGD("timer expired.");
    }

private:
    Timer mTimer;
};

#include "Task.h"
class TaskTest : public Task
{
protected:
    bool onPreStart() override
    {
        mExitTask.store(false, std::memory_order_relaxed);
        return true;
    }

    void onPreStop() override
    {
        mExitTask.store(true, std::memory_order_relaxed);
    }

    void run() override
    {
        int count = 0;
        while (!mExitTask.load(std::memory_order_relaxed))
        {
            LOGD("count : %d", ++count);
            msleep(1000);
        }
    }
private:
    std::atomic<bool> mExitTask{false};
};

int main(void)
{
    signal(SIGINT, _sig_handler);

    TimerTest timer;
    timer.start();

    TaskTest  task;
    task.start();

    while (MainLoop::getInstance().loop()) {  /* NOP */ }

    task.stop();
    timer.stop();

    return 0;
}

