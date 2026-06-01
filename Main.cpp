#include "MainLoop.h"
#include "Platform.h"
#include "WorkerThread.h"
#include "Log.h"

static bool platform_init()
{
    // TODO
    return true;
}

static void platform_deinit()
{
    // TODO
}

/**
 * Simple Example Code
 *   How to use my base code.
 *     - MainLoop - Timer, EventQ
 *     - WorkerThread
 */
class App : public IWorker, public ITimerHandler
{
public:
    explicit App(MainLoop& loop) : mLoop(loop)
                                 , mTimer(loop.createTimer())
    {
        mTimer.setHandler(this);
    }

    virtual ~App() override
    {
        stop();

        mTimer.setHandler(nullptr);
    }

    bool start()
    {
	    mTimer.start(1000, true);
        return mThread.start(*this);
    }

    void stop()
    {
        mTimer.stop();
        mThread.stop();
    }

private:
    void run() noexcept override
    {
    __TRACE__
        int n = 0;
        while (mThread.shouldRun())
        {
            LOGD("Thread : %d", n++);
            mThread.msleep(1000);
        }
    }

    bool onTimerExpired(const ITimer& timer) noexcept override
    {
        LOGD("TimerExpired.");
        return true;
    }

private:
    MainLoop& mLoop;
    Timer mTimer;
    WorkerThread mThread;
};

int main()
{
    MainLoop loop;

    Platform platform(loop); // RAII Pattern
    if (!platform.init(platform_init, platform_deinit))
       return -1;

    App app(loop);
    if (!app.start())
       return -2;

    loop.loop();

    app.stop();

    return 0;
}
