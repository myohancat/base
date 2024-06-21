#include "MainLoop.h"
#include "Log.h"

#include <signal.h>

static void _signal_handler(int signo)
{
    UNUSED(signo);

    MainLoop::getInstance().terminate();
}

int main()
{
__TRACE__
    signal(SIGINT,  _signal_handler);
    signal(SIGQUIT, _signal_handler);
    signal(SIGTERM, _signal_handler);

    /* TODO */

    LOGE("ERROR");
    LOGW("WARNNIG");
    LOGI("INFO");
    LOGD("DEBUG");
    LOGT("TRACE");
    PRINT("PRINT");
    CHECK("CHECK");

    while(MainLoop::getInstance().loop()) { }   

    /* TODO */

    return 0;
}
