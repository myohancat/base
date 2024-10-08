/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "UART.h"

#include <stdio.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "NetUtil.h"
#include "Log.h"

void UART::setListener(IUARTListener* listener)
{
    Lock lock(mLock);
    mListener = listener;
}

UART* UART::open(const char* dev, int baudrate)
{
    int fd = ::open(dev, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        LOGE("Cannot open %s", dev);
        return NULL;
    }

    struct termios tio;
    struct termios old;
    tcgetattr(fd, &old); // save current port setting.

    //set new port settings
    switch(baudrate)
    {
        case 9600:
            tio.c_cflag = B9600;
            break;
        case 19200:
            tio.c_cflag = B19200;
            break;
        case 38400:
            tio.c_cflag = B38400;
            break;
        case 57600:
            tio.c_cflag = B57600;
            break;
        case 115200:
            tio.c_cflag = B115200;
            break;
        default:
            LOGE("baudrate %d is not supported", baudrate);
            SAFE_CLOSE(fd);
            return NULL;
    }
/*
    tio.c_cflag |= (CRTSCTS | CS8 | CLOCAL | CREAD);
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;//non-Canonical mode ... if you want Canonical mode, add ICANON;
    */
    tio.c_cflag &= ~CSIZE; 
    tio.c_cflag |= CS8;  
    tio.c_cflag &= ~CSTOPB; 
    tio.c_cflag &= ~PARENB; 
    tio.c_cflag |= (CLOCAL | CREAD); 
    tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); 

    tio.c_oflag &= ~OPOST; 
    tio.c_cc[VMIN]= 0;  // non blocking...
    tio.c_cc[VTIME]=0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &tio);

    return new UART(fd);
}

UART::UART(int fd)
    : Task("UART"), mFD(fd)
{
    start();
}

UART::~UART()
{
    mLock.lock();
    mListener = NULL;
    mLock.unlock();
    
    stop();

    SAFE_CLOSE(mFD);
}

int UART::write(const uint8_t* data, int len)
{
    if (!data)
        return -1;

    return ::write(mFD, data, len);
}

void UART::run()
{
__TRACE__
    while(!mExitTask)
    {
        int ret = NetUtil::fd_poll(mFD, POLL_REQ_IN, 1000, mPipe.getFD());
        if (ret < 0)
        {
            LOGE("failed to poll : %d", ret);
            break;
        }

        if (ret == POLL_SUCCESS)
        {
            Lock lock(mLock);

            uint8_t data[2*1024];
            int ret = ::read(mFD, &data, sizeof(data) - 1);
            data[ret] = 0;

            if (mListener)
                mListener->onDataReceived(data, ret);
            else
                fputs((const char*)data, stdout);
        }
    }
}

bool UART::onPreStart()
{
    mExitTask = false;
    return true;
}

void UART::onPreStop()
{
    mExitTask = true;
    mPipe.write("T", 1);
}

void UART::onPostStop()
{
    mExitTask = true;
    mPipe.flush();
}
