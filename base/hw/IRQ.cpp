/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "IRQ.h"

#include <stdio.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "Log.h"

void IRQ::setListener(IIRQListener* listener)
{
    mListener = listener;
}

IRQ* IRQ::open(int num, GPIO_Edge_e egde, bool activeLow)
{
    if (!_exist(num))
    {
        _export(num);
        if (!_exist(num))
        {
            LOGE("Cannot open gpio node : num %d", num);
            return NULL;
        }
    }

    return new IRQ(num, egde, activeLow);
}

IRQ::IRQ(int num, GPIO_Edge_e egde, bool activeLow)
    : GPIO(num),
      mEdge(egde),
      mActiveLow(activeLow)
{
    char name[32];
    sprintf(name, "IRQ%d", num);
    mName = name;

    setOutDir(GPIO_DIR_IN);
    setEdge(GPIO_EDGE_NONE);
    setActiveLow(activeLow);

    start();
}

IRQ::~IRQ()
{
    stop();
}

void IRQ::disable(bool isAsyncCall)
{
    if (isAsyncCall)
        mPipe.write("D", 1);
    else
        setEdge(GPIO_EDGE_NONE);
}

void IRQ::enable(bool isAsyncCall)
{
    if (isAsyncCall)
        mPipe.write("E", 1);
    else
        setEdge(mEdge);
}

#define POLL_TIMEOUT   (1000*1000)
void IRQ::run()
{
    int ret = 0;
    struct pollfd fds[2];
    nfds_t nfds = 2;

    fds[0].fd = mPipe.getFD();
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    fds[1].fd = mFD;
    fds[1].events = POLLPRI | POLLERR;
    fds[1].revents = 0;

    char dummy;
    ::read(mFD, &dummy, 1);

    while(!mExitTask)
    {
        ret = ::poll(fds, nfds, POLL_TIMEOUT);
        if (ret == 0)
        {
            //LOG_WARN("poll timeout - %d msec. GIVE UP.", POLL_TIMEOUT);
            continue;
        }

        if (ret < 0)
        {
            LOGE("poll failed. ret=%d, errno=%d.", ret, errno);
            break;
        }

        if (fds[0].revents & POLLIN)
        {
            char data;
            mPipe.read(&data, 1);

            switch(data)
            {
                case 'D':
                    setEdge(GPIO_EDGE_NONE);
                    LOGT("IRQ %d disabled.", getNumber());
                    break;
                case 'E':
                    setEdge(mEdge);
                    LOGT("IRQ %d enabled.", getNumber());
                    break;
                case 'T':
                    LOGI("Termiate IRQ Proc");
                    continue;
            }
        }

        if (fds[1].revents & POLLPRI)
        {

            //LOGT("[[[[ INTERRUPTED ]]]] IRQ %d", getNumber());
            char value;
            ::lseek(mFD, 0, SEEK_SET);
            ::read(mFD, &value, 1);

            if (mListener)
                mListener->onInterrupted(this, value == '1');
        }

#if 0
        if ( fds[0].revents & POLLRDHUP )
            LOGE("POLLRDHUP.");
        if ( fds[0].revents & POLLERR )
            LOGE("POLLERR.");
        if ( fds[0].revents & POLLHUP )
            LOGE("POLLHUP.");
        if ( fds[0].revents & POLLNVAL )
            LOGE("POLLNVAL.");
#endif
    }
}

bool IRQ::onPreStart()
{
    char path[MAX_PATH_LEN];
    mExitTask = false;
    snprintf(path, MAX_PATH_LEN, "%s/value", getPath().c_str());
    mFD = ::open(path, O_RDONLY);
    if (mFD == -1)
        return false;

    return true;
}

void IRQ::onPreStop()
{
    mExitTask = true;
    mPipe.write("T", 1);
}

void IRQ::onPostStop()
{
    mExitTask = true;
    mPipe.flush();
    if (mFD != -1)
    {
        ::close(mFD);
        mFD = -1;
    }
}
