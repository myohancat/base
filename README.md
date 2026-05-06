# My simple base code
  
이 코드는 내가 연습삼아 만들어 보는 코드입니다.
대체로 Modern C++을 공부하는데 만든 코드로 심플하면서, 복잡하지 않은,
공부용으로 만든 코드들입니다.

연습용이기에 문제점이 있을수도 있습니다.

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
