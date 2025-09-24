/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "DisplayHotplugManager.h"

#include <stdlib.h>
#include <string.h>
#include <linux/netlink.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <algorithm>

#include "Util.h"
#include "Log.h"

static const char* gDevicePaths[] =
{
#if defined(CONFIG_SOC_RK3588)
    "/devices/platform/fde50000.dp/extcon",
    "/devices/platform/fde80000.hdmi/extcon",
#elif defined(CONFIG_SOC_IMX8MQ)
    // TODO
#elif defined(CONFIG_SOC_XAVIER)
    // TODO
#elif defined(CONFIG_SOC_RPI5)
    "/devices/platform/axi/axi:gpu/drm/card0",
    "/devices/platform/axi/axi:gpu/drm/card1",
#else
#error === UNSUPPORTED CHIPSET ===
#endif
};

DisplayHotplugManager& DisplayHotplugManager::getInstance()
{
    static DisplayHotplugManager _instance;

    return _instance;
}

DisplayHotplugManager::DisplayHotplugManager()
{
    struct sockaddr_nl addr;

    mSock = socket(PF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
    if(mSock < 0)
    {
        LOGE("socket error (%d) - %s", errno, strerror(errno));
        exit(-2);
    }

    memset(&addr, 0x00, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_LINK;

    if(bind(mSock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        LOGE("bind failed (%d) - %s", errno, strerror(errno));
        exit(-2);
    }

    MainLoop::getInstance().addFdWatcher(this);
}

DisplayHotplugManager::~DisplayHotplugManager()
{
    MainLoop::getInstance().removeFdWatcher(this);

    if(mSock >= 0)
        close(mSock);
}

int DisplayHotplugManager::getFD()
{
    return mSock;
}

bool DisplayHotplugManager::onFdReadable(int fd)
{
    int    ret = 0;
    char   buf[4096];
    char*  devpath;
    struct sockaddr_nl nladdr;
    struct iovec iov =
    {
        .iov_base = buf,
        .iov_len  = sizeof(buf),
    };

    struct msghdr msg =
    {
        .msg_name    = &nladdr,
        .msg_namelen = sizeof(nladdr),
        .msg_iov     = &iov,
        .msg_iovlen  = 1,
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_flags = 0,
    };

    ret = recvmsg(fd, &msg, 0);
    if(ret < 0)
        return true;

    if(ret == 0)
    {
        LOGE("EOF on netlink");
        return false;
    }

    if(msg.msg_namelen != sizeof(nladdr))
    {
        LOGE("Wrong address length");
        return true;
    }

    if(iov.iov_len < ((size_t)ret) || (msg.msg_flags & MSG_TRUNC))
    {
        LOGE("Malformatted or truncated message, skipping");
        return true;
    }

    buf[ret] = 0;
    devpath = strchr(buf, '@');
    if(devpath == NULL)
        return true;

    //LOGD("%s", devpath);

    *devpath = 0;
    devpath++;

    for (size_t ii = 0; ii < NELEM(gDevicePaths); ii++)
    {
        if (strncmp(devpath, gDevicePaths[ii], strlen(gDevicePaths[ii])) == 0)
        {
            notifyHotplugEvent(gDevicePaths[ii]);
            return true;
        }
    }

    return true;
}

#include <sys/types.h>
#include <dirent.h>
bool DisplayHotplugManager::isPlugged()
{
__TRACE__
    for (int ii = 0; ii < NELEM(gDevicePaths); ii++)
    {
        if (isPlugged(gDevicePaths[ii]))
            return true;
    }

    return false;
}

bool DisplayHotplugManager::isPlugged(const char* devpath)
{
    char path[1024];
    snprintf(path, sizeof(path), "/sys%s", devpath);

    bool plugged = false;

    DIR* dir = opendir(path);
    if (!dir)
        return false;

    struct dirent* de;
    while((de = readdir(dir)))
    {
        if(de->d_name[0] == '.')
        {
            if(de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0'))
                    continue;
        }

#if defined(CONFIG_SOC_RPI5)
        if (strncmp(de->d_name, "card0-HDMI-A", 12) != 0 && strncmp(de->d_name, "card1-HDMI-A", 12) != 0)
            continue;
#endif

        plugged |= isPlugged(path, de->d_name);
        if (plugged)
            break;
    }
    closedir(dir);

    LOGD("plugged : %d", plugged);
    return plugged;
}

bool DisplayHotplugManager::isPlugged(const char* syspath, const char* subpath)
{
    char path[2048];
    char line[1024];
#if defined(CONFIG_SOC_RK3588)
    sprintf(path, "%s/%s/state", syspath, subpath);
#else
    sprintf(path, "%s/%s/status", syspath, subpath);
#endif

    FILE* fp = fopen(path, "r");
    if (!fp)
        return false;

    if (fgets(line, sizeof(line), fp))
    {
        LOGD("%s", trim(line));
#if defined(CONFIG_SOC_RK3588)
        char* p = strchr(line, '=');
        if (!p)
            p = line;
        else
            p++;

        if (atoi(p) == 1)
            return true;

#elif defined(CONFIG_SOC_RPI5)
        if (strncmp(line, "connected", 9) == 0)
            return true;
#endif
    }
    fclose(fp);

    return false;
}

void DisplayHotplugManager::notifyHotplugEvent(const char* devpath)
{
    bool plugged= isPlugged(devpath);

    mListenerLock.lock();
    for(ListenerList::iterator it = mListeners.begin(); it != mListeners.end(); it++)
    {
        if (plugged)
            (*it)->onDisplayPlugged();
        else
            (*it)->onDisplayRemoved();
    }
    mListenerLock.unlock();
}

void DisplayHotplugManager::addListener(IDiplayHotplugListener* listener)
{
    Lock lock(mListenerLock);

    if(!listener)
        return;

    ListenerList::iterator it = std::find(mListeners.begin(), mListeners.end(), listener);
    if(listener == *it)
    {
        LOGW("Listener is alreay exsit !!");
        return;
    }

    mListeners.push_back(listener);
}

void DisplayHotplugManager::removeListener(IDiplayHotplugListener* listener)
{
    Lock lock(mListenerLock);

    if(!listener)
        return;

    for(ListenerList::iterator it = mListeners.begin(); it != mListeners.end(); it++)
    {
        if(listener == *it)
        {
            mListeners.erase(it);
            return;
        }
    }
}

