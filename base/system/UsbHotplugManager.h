/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __USB_HOTPLUG_MANAGER_H_
#define __USB_HOTPLUG_MANAGER_H_

#include "MainLoop.h"
#include <vector>

typedef enum
{
    USB_HOTPLUG_EVENT_ADD,
    USB_HOTPLUG_EVENT_CHANGE,
    USB_HOTPLUG_EVENT_REMOVE,

    USB_HOTPLUG_EVENT_UNKNOWN /* DON'T MODIFY THIS */

} UsbHotplugEvent_e;

class IUsbHotplugListener
{
public:
    virtual ~IUsbHotplugListener() { }

    virtual void onHotplugChanged(UsbHotplugEvent_e event, const char* devPath) = 0;
};


class UsbHotplugManager : public IFdWatcher
{
public:
    static UsbHotplugManager& getInstance();

    int  getFD();
    bool onFdReadable(int fd);

    void addListener(IUsbHotplugListener* listener);
    void removeListener(IUsbHotplugListener* listener);

private:
    UsbHotplugManager();
    ~UsbHotplugManager();

    int mSock;

    std::vector<IUsbHotplugListener*> mListeners;

    void notifyHotplugEvent(UsbHotplugEvent_e eEvent, const char* devPath);

};

#endif /* __USB_HOTPLUG_MANAGER_H_ */
