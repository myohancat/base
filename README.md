# My base code
  
My base code to help developments.
  - MainLoop
  - Timer
  - Task
  - Mutex
  - CondVar
  - NetUtil
  - ProcessUtil

```c
#include "MainLoop.h"

#include <signal.h>

static void _sig_handler(int signum)
{
    (void)signum;

    printf("SIGINT - terminate program\n");
    MainLoop::getInstance().terminate();
}

int main(void)
{
    signal(SIGINT, _sig_handler);

    // TODO. IMPLEMENTS HERE

    while(MainLoop::getInstance().loop()) { }

    // TODO. IMPLEMENTS HERE

    return 0;
}

```
