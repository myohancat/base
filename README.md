# My simple base code
  
이 코드는 내가 연습삼아 만들어 보는 코드입니다.
대체로 Modern C++을 공부하는데 만든 코드로 심플하면서, 복잡하지 않은,
공부용으로 만든 코드들입니다.

연습용이기에 문제점이 있을수도 있습니다.

```c
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
class App : public IWorker, public ITimerHandler, public IEventHandler
{
public:
    explicit App(MainLoop& loop) : mLoop(loop)
                                 , mTimer(loop.createTimer())
                                 , mEvtQ(loop.createEventQ())
    {
        mTimer.setHandler(this);
	mEvtQ.setHandler(this);
    }

    virtual ~App() override
    {
        stop();

        mTimer.setHandler(nullptr);
        mEvtQ.setHandler(nullptr);
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
             mEvtQ.sendEvent(n++);
             mThread.msleep(1000);
        }
    }

    bool onTimerExpired(const ITimer& timer) noexcept override
    {
        LOGD("TimerExpired.");
        return true;
    }

    void onEventReceived(int id, void* data, int dataLen) noexcept override
    {
        LOGD("Event : %d", id);
    }

private:
    MainLoop& mLoop;
    Timer mTimer;
    EventQ mEvtQ;
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

```
