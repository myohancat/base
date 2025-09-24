/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "NetlinkManager.h"

#include <stdlib.h>
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

NetlinkManager& NetlinkManager::getInstance()
{
    static NetlinkManager _instance;

    return _instance;
}

static inline void parse_rtattr (struct rtattr **tb, int max, struct rtattr *rta, int len)
{
    while (RTA_OK (rta, len))
    {
        if (rta->rta_type <= max)
            tb[rta->rta_type] = rta;

        rta = RTA_NEXT (rta, len);
    }
}

NetlinkManager::NetlinkManager()
{
    struct sockaddr_nl addr;

    mSock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if(mSock < 0)
    {
        LOGE("socket error (%d) - %s", errno, strerror(errno));
        exit(-2);
    }

    memset(&addr, 0x00, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE | RTMGRP_LINK;
#if 0
    addr.nl_groups |= RTMGRP_IPV6_IFADDR; RTMGRP_IPV4_ROUTE;
    addr.nl_groups |= RTMGRP_IPV6_ROUTE;
#endif

    if(bind(mSock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        LOGE("bind failed (%d) - %s", errno, strerror(errno));
        exit(-2);
    }

    mTimer.setHandler(this);
    mTimer.start(3000, true);

    MainLoop::getInstance().addFdWatcher(this);
}

NetlinkManager::~NetlinkManager()
{
    mTimer.stop();

    if(mSock >= 0)
        close(mSock);

    /* TBD. IMPLEMENTS HERE */
    MainLoop::getInstance().removeFdWatcher(this);
}

int NetlinkManager::getFD()
{
    return mSock;
}

bool NetlinkManager::onFdReadable(int fd)
{
    int    ret = 0;
    char   buf[4096];
    struct nlmsghdr* hdr;
    struct sockaddr_nl nladdr;
    struct iovec iov;
    iov.iov_base = buf;
    iov.iov_len  = sizeof(buf);

    struct msghdr msg;
    msg.msg_name    = &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov     = &iov;
    msg.msg_iovlen  = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;


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

    for(hdr = (struct nlmsghdr*)buf; NLMSG_OK(hdr, (int)ret); hdr = NLMSG_NEXT(hdr, ret))
    {
        if(hdr->nlmsg_type == NLMSG_DONE)
            return true;

        switch(hdr->nlmsg_type)
        {
            case RTM_NEWROUTE:
            case RTM_DELROUTE:
                parseRouteMsg(hdr);
                break;
            case RTM_NEWLINK:
            case RTM_DELLINK:
                parseLinkMsg(hdr);
                break;
            case RTM_NEWADDR:
            case RTM_DELADDR:
                parseAddressMsg(hdr);
                break;
            default:
                LOGW("Unknown netlink nlmsg_type %d", hdr->nlmsg_type);
                break;
        }
    }

    return true;
}

void NetlinkManager::onTimerExpired(const ITimer* timer)
{
    // LOGD("---- polling to check IP ");
    (void)(timer);

    // TBD. IMPLEMENTS HERE
}

void NetlinkManager::parseRouteMsg(struct nlmsghdr *h)
{
    struct rtattr* tb[RTA_MAX + 1];
    char   ifname[IFNAMSIZ];
    char   anyaddr[16] = { 0 };

    int    index;
    void*  dest;
    void*  gate;

    struct rtmsg* rtm = (struct rtmsg*)NLMSG_DATA(h);
    int len = h->nlmsg_len - NLMSG_LENGTH(sizeof(struct rtmsg));

    if(h->nlmsg_type != RTM_NEWROUTE && h->nlmsg_type != RTM_DELROUTE)
        return;

    if(rtm->rtm_table != RT_TABLE_MAIN || rtm->rtm_type != RTN_UNICAST)
        return;

    memset(tb, 0x00, sizeof(tb));
    parse_rtattr(tb, RTA_MAX, RTM_RTA(rtm), len);

    index = 0;
    dest  = NULL;
    gate  = NULL;

    if(tb[RTA_OIF])
        index = *(int *)RTA_DATA(tb[RTA_OIF]);

    if(tb[RTA_DST])
        dest = RTA_DATA(tb[RTA_DST]);
    else
        dest = anyaddr;

    if(tb[RTA_GATEWAY])
        gate = RTA_DATA(tb[RTA_GATEWAY]);

//    if(tb[RTA_PRIORITY])
//        metric = *(int *)RTA_DATA(tb[RTA_PRIORITY]);

    if_indextoname(index, ifname);

    if(rtm->rtm_family == AF_INET || rtm->rtm_family == AF_INET6)
    {
        char destaddr[256];
        char gateway[256];

        if(gate)
        {
            inet_ntop(rtm->rtm_family, gate, gateway, 256);
        }
        if(dest)
        {
            inet_ntop(rtm->rtm_family, dest, destaddr, 256);
        }

        // Notify Netlink Event
        do
        {
            int count = mListeners.size();
            for(int ii = 0; ii < count; ii++)
                mListeners[ii]->onRouteChanged(h->nlmsg_type == RTM_NEWROUTE,
                                               ifname,
                                               dest?destaddr:NULL,
                                               gate?gateway:NULL);
        }while(0);
    }

}

void NetlinkManager::parseLinkMsg(struct nlmsghdr *h)
{
    struct rtattr* tb[IFLA_MAX + 1];
    char   ifname[IFNAMSIZ];

    struct ifinfomsg* ifi = (ifinfomsg*)NLMSG_DATA(h);
    int len = h->nlmsg_len - NLMSG_LENGTH(sizeof(struct ifinfomsg));

    if(h->nlmsg_type != RTM_NEWLINK && h->nlmsg_type != RTM_DELLINK)
        return;

    memset(tb, 0, sizeof tb);
    parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi), len);

    if(!tb[IFLA_IFNAME])
    {
        LOGE("IF_NAME is NULL ");
        return;
    }

    strncpy(ifname, (char*)RTA_DATA(tb[IFLA_IFNAME]), IFNAMSIZ);

    // Notify Netlink Event
    do
    {
        bool isNew = ((ifi->ifi_flags & IFF_RUNNING) != 0);
        int count = mListeners.size();
        for(int ii = 0; ii < count; ii++)
            mListeners[ii]->onLinkChanged(isNew, ifname);
    }while(0);
}

void NetlinkManager::parseAddressMsg(struct nlmsghdr *h)
{
    struct rtattr* tb[IFA_MAX + 1];
    char   ifname[IFNAMSIZ];
    char   ifaddr[256];

    struct ifaddrmsg* ifa = (struct ifaddrmsg*) NLMSG_DATA(h);
    int len = h->nlmsg_len - NLMSG_LENGTH(sizeof (struct ifaddrmsg));

    if(ifa->ifa_family != AF_INET && ifa->ifa_family != AF_INET6)
        return;

    if(h->nlmsg_type != RTM_NEWADDR && h->nlmsg_type != RTM_DELADDR)
        return;

    memset(tb, 0x00, sizeof(tb));
    parse_rtattr(tb, IFA_MAX, IFA_RTA(ifa), len);


#if 0
    if(tb[IFA_LOCAL])
        LOGD("   IFA_LOCAL     %s", inet_ntop(ifa->ifa_family, RTA_DATA(tb[IFA_LOCAL]), ifaddr, 256));
    if(tb[IFA_ADDRESS])
        LOGD("   IFA_ADDRESS   %s", inet_ntop(ifa->ifa_family, RTA_DATA(tb[IFA_ADDRESS]), ifaddr, 256));
    if(tb[IFA_BROADCAST])
        LOGD("   IFA_BROADCAST %s", inet_ntop(ifa->ifa_family, RTA_DATA(tb[IFA_BROADCAST]), ifaddr, 256));
#endif

    if(!tb[IFA_LOCAL])
        tb[IFA_LOCAL] = tb[IFA_ADDRESS];

    if(tb[IFLA_IFNAME])
        strncpy(ifname, (char*)RTA_DATA(tb[IFLA_IFNAME]), IFNAMSIZ);
    else
        if_indextoname(ifa->ifa_index, ifname);

    inet_ntop(ifa->ifa_family, RTA_DATA(tb[IFA_LOCAL]), ifaddr, 256);

    // Notify Netlink Event
    do
    {
        int count = mListeners.size();
        for(int ii = 0; ii < count; ii++)
            mListeners[ii]->onAddressChanged(h->nlmsg_type == RTM_NEWADDR, ifname, ifaddr);
    }while(0);
}

void NetlinkManager::addListener(INetlinkListener* observer)
{
    mListeners.push_back(observer);
}

void NetlinkManager::removeListener(INetlinkListener* observer)
{
    int count = mListeners.size();
    int ii;

    for(ii = 0; ii < count; ii++)
    {
        if(mListeners[ii] == observer)
            break;
    }

    if(ii < count)
        mListeners.erase(mListeners.begin() + ii);
}

