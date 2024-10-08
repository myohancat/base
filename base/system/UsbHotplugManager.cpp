/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "UsbHotplugManager.h"

#include <stdlib.h>
#include <linux/netlink.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "Log.h"


UsbHotplugManager& UsbHotplugManager::getInstance()
{
    static UsbHotplugManager _instance;

    return _instance;
}

UsbHotplugManager::UsbHotplugManager()
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

UsbHotplugManager::~UsbHotplugManager()
{
    MainLoop::getInstance().removeFdWatcher(this);

    if(mSock >= 0)
        close(mSock);
}

int UsbHotplugManager::getFD()
{
    return mSock;
}

bool UsbHotplugManager::onFdReadable(int fd)
{
    UsbHotplugEvent_e eEvent;

    int    ret = 0;
    char   buf[4096];
    char*  action;
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
    //LOGT("==> %s", buf);
    action = buf;
    devpath = strchr(buf, '@');
    if(devpath == NULL)
        return true;

    *devpath = 0;
    devpath++;

    if (strncmp(devpath, "/devices", 8))
        return true;

    if(strcmp(action, "add") == 0)
        eEvent = USB_HOTPLUG_EVENT_ADD;
    else if(strcmp(action, "change") == 0)
        eEvent = USB_HOTPLUG_EVENT_CHANGE;
    else if(strcmp(action, "remove") == 0)
        eEvent = USB_HOTPLUG_EVENT_REMOVE;
    else
        eEvent = USB_HOTPLUG_EVENT_UNKNOWN;

    notifyHotplugEvent(eEvent, devpath);

    return true;
}

void UsbHotplugManager::notifyHotplugEvent(UsbHotplugEvent_e eEvent, const char* devPath)
{
    int count = mListeners.size();
    int ii;

    for(ii = 0; ii < count; ii++)
    {
        mListeners[ii]->onHotplugChanged(eEvent, devPath);
    }
}

void UsbHotplugManager::addListener(IUsbHotplugListener* listener)
{
    mListeners.push_back(listener);
}

void UsbHotplugManager::removeListener(IUsbHotplugListener* listener)
{
    int count = mListeners.size();
    int ii;

    for(ii = 0; ii < count; ii++)
    {
        if(mListeners[ii] == listener)
            break;
    }

    if(ii < count)
        mListeners.erase(mListeners.begin() + ii);
}

