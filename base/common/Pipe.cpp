#include "Pipe.h"

#include "Log.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "NetUtil.h"

Pipe::Pipe()
{
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, mFds) < 0)
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
    char buf[1024];
    NetUtil::socket_set_blocking(mFds[0], false);
    while(::read(mFds[0], buf, sizeof(buf)) > 0) { } // Flush Data
    NetUtil::socket_set_blocking(mFds[0], true);
}

int Pipe::getFD()
{
    return mFds[0];
}
