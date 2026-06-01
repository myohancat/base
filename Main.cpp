#include "MainLoop.h"
#include "Platform.h"
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


#include "WorkerThread.h"
class App : public IWorker
{
public:
    App(MainLoop& loop) : mLoop(loop) { }
    virtual ~App() override { stop(); }

    bool start()
    {
        return mThread.start(*this);
    }

    void stop()
    {
        mThread.stop();
    }

private:
    void run() noexcept override
    {
    __TRACE__
        while (mThread.shouldRun())
        {
             mThread.msleep(100);
        }
    }

private:
    MainLoop& mLoop;
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
