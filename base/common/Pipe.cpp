
/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Pipe.h"

#include "Log.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

Pipe::Pipe()
{
    if (pipe(mFds) < 0)
    {
        LOGE("Failed to open pipe. errno : %d", errno);
        mFds[0] = -1;
        mFds[1] = -1;
    }
}

Pipe::~Pipe()
{
    SAFE_CLOSE(mFds[0]);
    SAFE_CLOSE(mFds[1]);
}

int Pipe::read(void* data, int len)
{
    if (mFds[0] == -1)
    {
        LOGE("socket is not opened.");
        return -1;
    }

    return ::read(mFds[0], data, len);
}

int Pipe::write(const void* data, int len)
{
    if (mFds[1] == -1)
    {
        LOGE("socket is not opened.");
        return -1;
    }

    return ::write(mFds[1], data, len);
}

void Pipe::flush()
{
    /* W/A Code for Flush */
    uint8_t dummy[1024];
    int value;
    while(ioctl(mFds[0], FIONREAD, &value) == 0 && value > 0)
        ::read(mFds[0], dummy, sizeof(dummy));
}

int Pipe::getFD()
{
    return mFds[0];
}
