/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __NETLINK_MANAGER_H_
#define __NETLINK_MANAGER_H_

#include <linux/netlink.h>

#include "MainLoop.h"
#include "Timer.h"
#include <vector>

class INetlinkListener
{
public:
    virtual ~INetlinkListener() { }

    virtual void onRouteChanged(bool isNew, const char* ifname, const char* destaddr, const char* gateway) = 0;
    virtual void onLinkChanged(bool isNew, const char* ifname) = 0;
    virtual void onAddressChanged(bool isNew, const char* ifname, const char* ifaddr) = 0;
};

class NetlinkManager : public IFdWatcher, ITimerHandler
{
public:
    static NetlinkManager& getInstance();

    int  getFD();
    bool onFdReadable(int fd);

    /* Polling to check IP */
    void onTimerExpired(const ITimer* timer);

    void addListener(INetlinkListener* observer);
    void removeListener(INetlinkListener* observer);

private:
    NetlinkManager();
    ~NetlinkManager();

    int mSock;
    Timer mTimer;

    std::vector<INetlinkListener*> mListeners;

    void parseRouteMsg(struct nlmsghdr *h);
    void parseLinkMsg(struct nlmsghdr *h);
    void parseAddressMsg(struct nlmsghdr *h);
};


#endif /* __NETLINK_MANAGER_H_ */
