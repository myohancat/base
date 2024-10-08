/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "GPIO.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "Util.h"
#include "Log.h"

#define SYS_FS_DIR "/sys/class/gpio"

static const char* GPIO_DIR_STRs[GPIO_DIR_UNKNOWN] =
{
    "in", "out"
};

static const char* GPIO_EDGE_STRs[GPIO_EDGE_UNKNOWN] =
{
    "none", "rising", "falling", "both"
};

GPIO* GPIO::open(int num)
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

    return new GPIO(num);
}

GPIO::GPIO(int num) : mNum(num)
{
    char path[MAX_PATH_LEN];
    snprintf(path, MAX_PATH_LEN, "%s/gpio%d", SYS_FS_DIR, num);
    mPath = path;
}

GPIO_Dir_e GPIO::getOutDir()
{
    char buf[MAX_PATH_LEN];
    if (!_exist(mNum)) {
        LOGE("GPIO node does not exist.");
        return GPIO_DIR_UNKNOWN;
    }
    snprintf(buf, MAX_PATH_LEN, "%s/direction", mPath.c_str());
    int fd = ::open(buf, O_RDONLY);
    int len =0;
    if (fd < 0)
    {
        LOGE("Cannot open file %s", buf);
        return GPIO_DIR_UNKNOWN;
    }

    GPIO_Dir_e value = GPIO_DIR_UNKNOWN;
    if ((len = ::read(fd, buf, sizeof(buf))) > 0)
    {
        buf[len] = 0;
        trim(buf);
        for (int ii = 0; ii < GPIO_DIR_UNKNOWN; ii++)
        {
            if (strcmp(buf, GPIO_DIR_STRs[ii]) == 0)
            {
                value = (GPIO_Dir_e) ii;
                break;
            }
        }
    }

    ::close(fd);
    return value;
}

void GPIO::setOutDir(GPIO_Dir_e eDIR)
{
    if (!_exist(mNum)) {
        LOGE("GPIO node does not exist.");
        return;
    }

    char buf[MAX_PATH_LEN];
    snprintf(buf, MAX_PATH_LEN, "%s/direction", mPath.c_str());
    int fd = ::open(buf, O_WRONLY);
    if (fd < 0)
    {
        LOGE("Cannot open file %s", buf);
        return;
    }

    if (eDIR >= GPIO_DIR_UNKNOWN)
        return;

    ::write(fd, GPIO_DIR_STRs[eDIR], strlen (GPIO_DIR_STRs[eDIR]));
    ::close(fd);
}

void GPIO::setValue(bool value)
{
    if (!_exist(mNum)) {
        LOGE("GPIO node does not exist.");
        return;
    }

    char path[MAX_PATH_LEN];
    snprintf(path, MAX_PATH_LEN, "%s/value", mPath.c_str());
    int fd = ::open(path, O_WRONLY);
    if (fd < 0)
    {
        LOGE("Cannot open file %s", path);
        return;
    }

    ::write(fd, value?"1":"0", 1);
    ::close(fd);
}

bool GPIO::getValue()
{
    if (!_exist(mNum)) {
        LOGE("GPIO node does not exist.");
        return false;
    }

    char buf[MAX_PATH_LEN];
    int len = 0;
    snprintf(buf, MAX_PATH_LEN, "%s/value", mPath.c_str());
    int fd = ::open(buf, O_RDONLY);
    if (fd < 0)
    {
        LOGE("Cannot open file %s", buf);
        return 0;
    }

    bool value = false;
    if ((len = read(fd, buf, sizeof(buf))) > 0) {
        buf[len] = 0;
        if(strncmp(buf, "0", 1) == 0)
            value = false;
        else
            value = true;
    }
    ::close(fd);

    return value;
}

GPIO_Edge_e GPIO::getEdge()
{
    if (!_exist(mNum)) {
        LOGE("GPIO node does not exist.");
        return GPIO_EDGE_UNKNOWN;
    }

    char buf[MAX_PATH_LEN];
    int len = 0;
    snprintf(buf, MAX_PATH_LEN, "%s/edge", mPath.c_str());
    int fd = ::open(buf, O_RDONLY);
    if (fd < 0)
    {
        LOGE("Cannot open file %s", buf);
        return GPIO_EDGE_UNKNOWN;
    }

    GPIO_Edge_e value = GPIO_EDGE_UNKNOWN;
    if ((len = ::read(fd, buf, sizeof(buf))) > 0)
    {
        buf[len] = 0;
        trim(buf);
        for (int ii = 0; ii < GPIO_EDGE_UNKNOWN; ii++)
        {
            if (strcmp(buf, GPIO_EDGE_STRs[ii]) == 0)
            {
                value = (GPIO_Edge_e) ii;
                break;
            }
        }
    }
    ::close(fd);

    return value;
}

void GPIO::setEdge(GPIO_Edge_e edge)
{
    if (!_exist(mNum)) {
        LOGE("GPIO node does not exist.");
        return;
    }

    char path[MAX_PATH_LEN];
    snprintf(path, MAX_PATH_LEN, "%s/edge", mPath.c_str());
    int fd = ::open(path, O_WRONLY);
    if (fd < 0)
    {
        LOGE("Cannot open file %s", path);
        return;
    }

    if (edge >= GPIO_EDGE_UNKNOWN)
        return;

    ::write(fd, GPIO_EDGE_STRs[edge], strlen(GPIO_EDGE_STRs[edge]));
    ::close(fd);
}

bool GPIO::isActiveLow()
{
    if (!_exist(mNum)) {
        LOGE("GPIO node does not exist.");
        return false;
    }

    char buf[MAX_PATH_LEN];
    int len = 0;
    snprintf(buf, MAX_PATH_LEN, "%s/active_low", mPath.c_str());
    int fd = ::open(buf, O_RDONLY);
    if (fd < 0)
    {
        LOGE("Cannot open file %s", buf);
        return 0;
    }

    bool value = false;
    if ((len = read(fd, buf, sizeof(buf))) > 0) {
        buf[len] = 0;
        if(strcmp(buf, "0") == 0)
            value = false;
        else
            value = true;
    }
    ::close(fd);

    return value;
}

void GPIO::setActiveLow(bool activeLow)
{
    if (!_exist(mNum)) {
        LOGE("GPIO node does not exist.");
        return;
    }

    char path[MAX_PATH_LEN];
    snprintf(path, MAX_PATH_LEN, "%s/active_low", mPath.c_str());
    int fd = ::open(path, O_WRONLY);
    if (fd < 0)
    {
        LOGE("Cannot open file %s", path);
        return;
    }

    ::write(fd, activeLow?"1":"0", 1);
    ::close(fd);
}

bool GPIO::_export(int num)
{
    char buf[1024];
    int n = 0;

    int fd = 0;
    fd = ::open(SYS_FS_DIR "/export", O_WRONLY);
    if (fd < 0)
    {
        LOGE("Cannot open file : %s", SYS_FS_DIR "/export");
        return false;
    }

    n = snprintf(buf, sizeof(buf), "%d", num);

    ::write(fd, buf, n);
    ::close(fd);

    return true;
}

bool GPIO::_exist(int num)
{
    char buf[MAX_PATH_LEN];
    snprintf(buf, MAX_PATH_LEN, SYS_FS_DIR "/gpio%d", num);
    if (::access(buf, F_OK) == 0)
        return true;

    return false;
}
