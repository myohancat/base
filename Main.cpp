#include <signal.h>

#include "Log.h"
#include "MainLoop.h"

static void _sig_handler(int signum)
{
    (void)signum;

    // NOP
    MainLoop::getInstance().terminate();
}

#include "TimerThread.h"
class TimerTest : public ITimerHandler
{
public:
    TimerTest()
    {
        mTimer.setHandler(this);
        mTimer2.setHandler(this);
    }

    ~TimerTest()
    {
        stop();
    }

    void start()
    {
        mTimer.start(1000, true);
        mTimer2.start(1000, true);
    }

    void stop()
    {
        mTimer.stop();
        mTimer2.stop();
    }

protected:
    bool onTimerExpired(const ITimer& timer) override
    {
        (void)timer;

        if (mTimer == timer)
        {
            LOGD("timer expired. : %d", mNum ++);

            if (mNum > 5)
            {
                mTimer.stop();
                mTimer2.stop();
            }

            return true;
        }
        else
        {
            LOGD("timer2 expired.");
            return true;
        }
    }

private:
    Timer mTimer;
    Timer mTimer2;
    int mNum = 0;
};

#include "WorkerThread.h"
class ThreadTest : public IWorker
{
public:
    ~ThreadTest() { stop(); }

    bool start() { return mThread.start(*this); }
    void stop()  { mThread.stop(); }

private:
    void run() noexcept override
    {
        int count = 0;
        while (mThread.shouldRun())
        {
            LOGD("count : %d", ++count);
            mThread.msleep(1000);
        }
    }

private:
    WorkerThread mThread;
};

int main(void)
{
    signal(SIGINT, _sig_handler);

    TimerTest timer;
    timer.start();

    ThreadTest  tread;
    tread.start();

    while (MainLoop::getInstance().loop()) {  /* NOP */ }

    tread.stop();
    timer.stop();

    return 0;
}

