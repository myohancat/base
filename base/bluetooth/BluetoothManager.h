/**
 * Simple Bluetooth Manager
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Copyright (c) 2022, MedithinQ. All rights reserved.
 */
#ifndef __BLUETOOTH_MANAGER_H_
#define __BLUETOOTH_MANAGER_H_

#include "EventQueue.h"
#include "Task.h"

#include <string>
#include <list>
#include <vector>

#include <glib.h>
#include "gdbus/gdbus.h"

typedef enum
{
    BT_TYPE_PHONE,
    BT_TYPE_MODEM,
    BT_TYPE_AUDIO,
    BT_TYPE_VIDEO,
    BT_TYPE_PHOTO,
    BT_TYPE_INPUT,
    BT_TYPE_PRINTER,

    BT_TYPE_UNKNOWN
}BluetoothType_e;

class BluetoothDevice
{
public:
    BluetoothDevice()
            : mAddress(""),
              mName(""),
              mConnected(false),
              mPaired(false),
              mType(BT_TYPE_UNKNOWN) { }

    BluetoothDevice(BluetoothDevice* dev)
            : mAddress(dev->mAddress),
              mName(dev->mName),
              mConnected(dev->mConnected),
              mPaired(dev->mPaired),
              mType(dev->mType) { }

    BluetoothDevice(const std::string& address, const std::string& name, bool isConnected, bool isPaired, BluetoothType_e eType)
                    : mAddress(address),
                      mName(name),
                      mConnected(isConnected),
                      mPaired(isPaired),
                      mType(eType) { }

    ~BluetoothDevice() { }

    void setDevice(const std::string& address, const std::string& name, bool isConnected, bool isPaired, BluetoothType_e eType)
    {
        mAddress = address;
        mName = name;
        mConnected = isConnected;
        mPaired = isPaired;
        mType   = eType;
    }

    void setDevice(BluetoothDevice* dev)
    {
        mAddress = dev->mAddress;
        mName = dev->mName;
        mConnected = dev->mConnected;
        mPaired = dev->mPaired;
        mType   = dev->mType;
    }

    std::string& getAddress() { return mAddress; }
    std::string& getName() { return mName; };

    bool isConnected() { return mConnected; }
    bool isPaired() { return mPaired; }

    BluetoothType_e getType() { return mType; }

private:
    std::string     mAddress;
    std::string     mName;
    bool            mConnected;
    bool            mPaired;
    BluetoothType_e mType;
};

typedef enum
{
    BT_EVENT_ADD_DEVICE,
    BT_EVENT_REMOVE_DEVICE,
    BT_EVENT_CHANGE_DEVICE,
    BT_EVENT_CONNECT_DEVICE,
    BT_EVENT_DISCONNECT_DEVICE,
    BT_EVENT_PAIR_DEVICE,
    BT_EVENT_ERROR,

    MAX_BLUETOOTH_EVENT
}BluetoothEvent_e;

typedef enum
{
    BT_ERROR_SCAN_FAILED,
    BT_ERROR_PAIRING_FAILED,
    BT_ERROR_CONNECT_FAILED,
    BT_ERROR_DISCONNECT_FAILED,

    MAX_BT_ERROR
}BluetoothError_e;

class IBluetoothListener
{
public:
    virtual void onDeviceAdded(BluetoothDevice* device) = 0;
    virtual void onDeviceRemoved(BluetoothDevice* device) = 0;
    virtual void onDeviceChanged(BluetoothDevice* device) = 0;
    virtual void onDeviceConnected(BluetoothDevice* device) = 0;
    virtual void onDeviceDisconnected(BluetoothDevice* device) = 0;
    virtual void onDevicePaired(BluetoothDevice* device) = 0;
    virtual void onErrorOccured(BluetoothError_e eError, const char* errmsg) = 0;
};

struct adapter {
    GDBusProxy *proxy;
    GDBusProxy *ad_proxy;
    GList *devices;
};

class BluetoothManager : public Task, IEventHandler
{
public:
    static BluetoothManager& getInstance();

    bool enable();
    void disable();

    void scan(bool enable);
    void discoverable(bool enable);
    void pair(const char* device);
    void trust(const char* device);
    void connect(const char* device);
    void disconnect(const char* device, bool remove);
    void remove(const char* device);
    void removeDeviceAll();
    void removeDeviceAllWithoutPaired();

    void getDeviceList(std::vector<BluetoothDevice*>& list);
    void getDeviceListWithoutPaired(std::vector<BluetoothDevice*>& list);
    void getPairedDeviceList(std::vector<BluetoothDevice*>& list);

    void addBluetoothListener(IBluetoothListener* listener);
    void removeBluetoothListener(IBluetoothListener* listener);

private:
    BluetoothManager();
    ~BluetoothManager();

    // Hide
    bool start() { return Task::start(); }
    void stop()  { Task::stop(); }

    // Implement Task
    bool onPreStart();
    void onPreStop();
    void onPostStop();

    void run();

    void onEventReceived(int id, void* data, int dataLen);

    int  getDeviceFromProxy(GDBusProxy* proxy, BluetoothDevice* device);

    void sendEventFromProxy(int eventId, GDBusProxy* proxy);
    void sendEventError(BluetoothError_e errNum, const char* errmsg);

    typedef std::list<IBluetoothListener*> BtListenerList;
    BtListenerList mBtListeners;

private:
    EventQueue mEvtQ;

private:
    // CLI
    static void cmd_bt_scan(void* session, int argc, char** argv, void* param);
    static void cmd_bt_discoverable(void* session, int argc, char** argv, void* param);
    static void cmd_bt_pair(void* session, int argc, char** argv, void* param);
    static void cmd_bt_trust(void* session, int argc, char** argv, void* param);

private:
    ///////////// porting from bluetoothctl //////////////
    GMainLoop*      main_loop = nullptr;
    DBusConnection* dbus_conn = nullptr;

    GDBusProxy*     agent_manager = nullptr;
    char*           auto_register_agent = nullptr;

    struct adapter* default_ctrl = nullptr;
    GDBusProxy*     default_dev = nullptr;
    GDBusProxy*     default_attr = nullptr;
    GList*          ctrl_list = nullptr;

    static void _generic_callback(const DBusError *error, void *user_data);
    static void _start_discovery_reply(DBusMessage *message, void *user_data);
    static void _pair_reply(DBusMessage *message, void *user_data);
    static void _connect_reply(DBusMessage *message, void *user_data);
    static void _disconn_reply(DBusMessage *message, void *user_data);
    static void _remove_device_reply(DBusMessage *message, void *user_data);
    static void _remove_device_setup(DBusMessageIter *iter, void *user_data);

    static void _connect_handler(DBusConnection *connection, void *user_data);
    static void _disconnect_handler(DBusConnection *connection, void *user_data);
    static void _message_handler(DBusConnection *connection, DBusMessage *message, void *user_data);

    static void _proxy_added(GDBusProxy *proxy, void *user_data);
    static void _proxy_removed(GDBusProxy *proxy, void *user_data);
    static void _property_changed(GDBusProxy *proxy, const char *name, DBusMessageIter *iter, void *user_data);

    void proxy_added(GDBusProxy *proxy, void *user_data);
    void proxy_removed(GDBusProxy *proxy, void *user_data);
    void property_changed(GDBusProxy *proxy, const char *name, DBusMessageIter *iter, void *user_data);

    void print_adapter(GDBusProxy *proxy, const char *description);
    void print_device(GDBusProxy *proxy, const char *description);

    gboolean device_is_child(GDBusProxy *device, GDBusProxy *master);
    gboolean service_is_child(GDBusProxy *service);

    struct adapter* adapter_new(GDBusProxy *proxy);
    struct adapter* find_parent(GDBusProxy *device);
    struct adapter* find_ctrl(GList *source, const char *path);
    struct adapter* find_ctrl_by_address(GList *source, const char *address);

    GDBusProxy*  find_device(const char* device);
    GDBusProxy*  find_proxy_by_address(GList *source, const char *address);

    gboolean check_default_ctrl(void);
    void     set_default_device(GDBusProxy *proxy, const char *attribute);
    void     set_default_attribute(GDBusProxy *proxy);

    void     remove_device(GDBusProxy *proxy);

    void     device_added(GDBusProxy *proxy);
    void     adapter_added(GDBusProxy *proxy);
    void     ad_manager_added(GDBusProxy *proxy);

    void     device_removed(GDBusProxy *proxy);
    void     adapter_removed(GDBusProxy *proxy);
};

#endif /* __BLUETOOTH_MANAGER_H_ */
