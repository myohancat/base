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
    "/devices/platform/display-subsystem/drm/card0"
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
    addr.nl_pid    = getpid();
    addr.nl_groups = 1;

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

     SAFE_CLOSE(mSock);
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

        if (strncmp(de->d_name, "card0-HDMI-A", 12) != 0 && strncmp(de->d_name, "card1-HDMI-A", 12) != 0)
            continue;

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
    sprintf(path, "%s/%s/status", syspath, subpath);

    bool connected = true;
    FILE* fp = fopen(path, "r");
    if (!fp)
        return connected;

    if (fgets(line, sizeof(line), fp))
    {
        LOGD("%s : %s", path, trim(line));
        if (strcmp(line, "disconnected") == 0)
            connected = false;

    }
    fclose(fp);

    return connected;
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

void DisplayHotplugManager::addListener(IDisplayHotplugListener* listener)
{
    Lock lock(mListenerLock);

    if(!listener)
        return;

    ListenerList::iterator it = std::find(mListeners.begin(), mListeners.end(), listener);
    if(it != mListeners.end())
    {
        LOGW("Listener is alreay exsit !!");
        return;
    }

    mListeners.push_back(listener);
}

void DisplayHotplugManager::removeListener(IDisplayHotplugListener* listener)
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

