/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __DP_HOTPLUG_MANAGER_H_
#define __DP_HOTPLUG_MANAGER_H_

#include "MainLoop.h"
#include <vector>

class IDiplayHotplugListener
{
public:
    virtual ~IDiplayHotplugListener() { }

    virtual void onDisplayPlugged() = 0;
    virtual void onDisplayRemoved() = 0;
};

class DisplayHotplugManager : public IFdWatcher
{
public:
    static DisplayHotplugManager& getInstance();

    bool isPlugged();

    void addListener(IDiplayHotplugListener* listener);
    void removeListener(IDiplayHotplugListener* listener);

protected:
    int  getFD();
    bool onFdReadable(int fd);

    bool isPlugged(const char* devpath);
    bool isPlugged(const char* syspath, const char* subpath);

private:
    DisplayHotplugManager();
    ~DisplayHotplugManager();

    void notifyHotplugEvent(const char* devpath);

    int   mSock;

    Mutex mListenerLock;
    typedef std::list<IDiplayHotplugListener*> ListenerList;
    ListenerList mListeners;
};

#endif /* __DP_HOTPLUG_MANAGER_H_ */
