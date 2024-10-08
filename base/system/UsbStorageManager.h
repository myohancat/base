/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __USB_STORAGE_MANAGER_H_
#define __USB_STORAGE_MANAGER_H_

#include "Task.h"
#include "Queue.h"
#include "UsbHotplugManager.h"

#include <list>
#include <map>

class IUsbStorageListener
{
public:
    virtual ~IUsbStorageListener() { }

    virtual void onUsbStorageAdded(const char* dev, const char* mntPoint) = 0;
    virtual void onUsbStorageRemoved(const char* dev, const char* mntPoint) = 0;
};

struct UsbMessage
{
public:
    int   mId;
    void* mParam;

    UsbMessage() : mId(0), mParam(NULL) { }
    UsbMessage(int id, void* param) : mId(id), mParam(param) { }
};

class UsbStorageManager : public Task, IUsbHotplugListener
{
public:
    static UsbStorageManager& getInstance();

    bool addListener(IUsbStorageListener* listener);
    void removeListener(IUsbStorageListener* listener);

    std::list<std::string> getMountPoints();

protected:
    UsbStorageManager();
    ~UsbStorageManager();

    bool onPreStart();
    void onPreStop();

    void run();

    void onHotplugChanged(UsbHotplugEvent_e event, const char* devPath);

protected:
    Mutex mLock;
    Queue<UsbMessage, 16>  mMsgQ;
    bool  mExitTask;

    std::list<IUsbStorageListener*> mListeners;
    std::map<std::string, int> mMountPoints;
};

#endif /* __USB_STORAGE_MANAGER_H_ */
