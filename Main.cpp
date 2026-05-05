#include "Log.h"

#include <signal.h>

#include "MainLoop.h"

static void _sig_handler(int signum)
{
    (void)signum;

    MainLoop::getInstance().terminate();
}


int main(void)
{
    signal(SIGINT, _sig_handler);

    while (MainLoop::getInstance().loop()) {  /* NOP */ }

    return 0;
}

