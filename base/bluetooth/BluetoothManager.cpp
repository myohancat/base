/**
 * Simple Bluetooth Manager
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Copyright (c) 2022, MedithinQ. All rights reserved.
 */
#include "BluetoothManager.h"

#include "ProcessUtil.h"

#include <algorithm>
#include <wordexp.h>
#include <unistd.h>

#include "gdbus/gdbus.h"

#ifdef CONFIG_SUPPORT_CLI
#include "CLI.h"
#endif

extern "C"
{
    #include "agent.h"
    //#include "gatt.h"
    //#include "advertising.h"
}

#include "Log.h"

#define COLORED_NEW    ANSI_COLOR_GREEN "NEW" ANSI_COLOR_RESET
#define COLORED_CHG    ANSI_COLOR_YELLOW "CHG" ANSI_COLOR_RESET
#define COLORED_DEL    ANSI_COLOR_RED "DEL" ANSI_COLOR_RESET

struct DbusParam
{
public:
    BluetoothManager* mThis;
    int        mArg;
    void*      mData;

    DbusParam(BluetoothManager* pThis, void* data) : mThis(pThis), mData(data) { }
    DbusParam(BluetoothManager* pThis, int arg, void* data) : mThis(pThis), mArg(arg), mData(data) { }
};

struct BtEvent
{
public:
    int   mEventId;
    void* mData;

    BtEvent(int eventId, void* data) : mEventId(eventId), mData(data) { }
};

struct BtError
{
public:
    BluetoothError_e  mNum;
    std::string mMsg;

    BtError(BluetoothError_e err, const char* errMsg) : mNum(err), mMsg(errMsg) { }
};

BluetoothManager& BluetoothManager::getInstance()
{
    static BluetoothManager _obj;

    return _obj;
}

void BluetoothManager::addBluetoothListener(IBluetoothListener* listener)
{
    if(!listener)
        return;

    BtListenerList::iterator it = std::find(mBtListeners.begin(), mBtListeners.end(), listener);
    if(it != mBtListeners.end())
        return;

    mBtListeners.push_back(listener);
}

void BluetoothManager::removeBluetoothListener(IBluetoothListener* listener)
{
    if(!listener)
        return;

    for(BtListenerList::iterator it = mBtListeners.begin(); it != mBtListeners.end(); it++)
    {
        if(listener == *it)
        {
            mBtListeners.erase(it);
            return;
        }
    }
}

void BluetoothManager::onEventReceived(int eventId, void* data, int dataLen)
{
    UNUSED(dataLen);
    void* p = UINTPTR_TO_PTR(data);

    if(eventId == BT_EVENT_ADD_DEVICE)
    {
        BluetoothDevice* device = (BluetoothDevice*)p;
        for(BtListenerList::iterator it = mBtListeners.begin(); it != mBtListeners.end(); it++)
        {
            (*it)->onDeviceAdded(device);
        }
        if (device)
            delete device;
    }
    else if(eventId == BT_EVENT_REMOVE_DEVICE)
    {
        BluetoothDevice* device = (BluetoothDevice*)p;
        for(BtListenerList::iterator it = mBtListeners.begin(); it != mBtListeners.end(); it++)
        {
            (*it)->onDeviceRemoved(device);
        }
        if (device)
            delete device;
    }
    else if(eventId == BT_EVENT_CHANGE_DEVICE)
    {
        BluetoothDevice* device = (BluetoothDevice*)p;
        for(BtListenerList::iterator it = mBtListeners.begin(); it != mBtListeners.end(); it++)
        {
            (*it)->onDeviceChanged(device);
        }
        if (device)
            delete device;
    }
    else if(eventId == BT_EVENT_PAIR_DEVICE)
    {
        BluetoothDevice* device = (BluetoothDevice*)p;
        for(BtListenerList::iterator it = mBtListeners.begin(); it != mBtListeners.end(); it++)
        {
            (*it)->onDevicePaired(device);
        }

        trust(device->getAddress().c_str());
        if (device)
            delete device;
    }
    else if(eventId == BT_EVENT_CONNECT_DEVICE)
    {
        BluetoothDevice* device = (BluetoothDevice*)p;
        LOGI("device connected");
        for(BtListenerList::iterator it = mBtListeners.begin(); it != mBtListeners.end(); it++)
        {
            (*it)->onDeviceConnected(device);
        }
        if (device)
            delete device;
    }
    else if(eventId == BT_EVENT_DISCONNECT_DEVICE)
    {
        BluetoothDevice* device = (BluetoothDevice*)p;
        LOGI("device disconnected");
        for(BtListenerList::iterator it = mBtListeners.begin(); it != mBtListeners.end(); it++)
        {
            (*it)->onDeviceDisconnected(device);
        }
    }
    else if(eventId == BT_EVENT_ERROR)
    {
        BtError* err = (BtError*)p;
        for(BtListenerList::iterator it = mBtListeners.begin(); it != mBtListeners.end(); it++)
        {
            (*it)->onErrorOccured(err->mNum, err->mMsg.c_str());
        }
        if (err)
            delete err;
    }
}

void BluetoothManager::_start_discovery_reply(DBusMessage *message, void *user_data)
{
    DbusParam* param = (DbusParam*)user_data;
    BluetoothManager* pThis = param->mThis;
    void* data       = param->mData;
    delete param;

    dbus_bool_t enable = GPOINTER_TO_UINT(data);
    DBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE)
    {
        LOGE("Failed to %s discovery: %s", enable == TRUE ? "start" : "stop", error.name);
        pThis->sendEventError(BT_ERROR_SCAN_FAILED, error.name);
        dbus_error_free(&error);
        return;
    }

    LOGI("Discovery %s", enable == TRUE ? "started" : "stopped");
}

static bool is_exist_bluetooth_interface(const char* intf)
{
    char path[1024];
    sprintf(path, "/sys/class/bluetooth/%s", intf);
    if (::access(path, F_OK) == 0)
        return true;

    return false;
}

bool BluetoothManager::enable()
{
__TRACE__
    if (!is_exist_bluetooth_interface("hci0"))
        return false;

    ProcessUtil::system("hciconfig hci0 up");

#ifdef CONFIG_SUPPORT_CLI
    CLI::getInstance().addCommand("bt_scan", cmd_bt_scan, this, "bluetooth scan on/off [0: off | 1: on]");
    CLI::getInstance().addCommand("bt_discoverable", cmd_bt_discoverable, this, "bluetooth discoverable on/off [0: off |1: on]");
    CLI::getInstance().addCommand("bt_pair", cmd_bt_pair, this, "bluetooth pair [device]");
    CLI::getInstance().addCommand("bt_trust", cmd_bt_trust, this, "bluetooth trust [device]");
#endif

    return start();
}

void BluetoothManager::disable()
{
__TRACE__
    stop();
    ProcessUtil::system("hciconfig hci0 down");
}

void BluetoothManager::scan(bool enable)
{
    const char *method;

    LOGI("----- REQUEST SCAN : %s", enable?"ON":"OFF");

    if(check_default_ctrl() == FALSE)
    {
        LOGE("there is no default ctrl !");
        return;
    }

    if (enable == TRUE)
        method = "StartDiscovery";
    else
        method = "StopDiscovery";

    if(g_dbus_proxy_method_call(default_ctrl->proxy, method, NULL, _start_discovery_reply, new DbusParam(this, GUINT_TO_POINTER(enable)), NULL) == FALSE)
    {
        LOGE("Failed to %s discovery", enable == TRUE ? "start" : "stop");
        return;
    }
}

void BluetoothManager::discoverable(bool enable)
{
    dbus_bool_t discoverable = (dbus_bool_t)enable;
    char *str;

    if (check_default_ctrl() == FALSE)
        return;

    str = g_strdup_printf("discoverable %s", discoverable == TRUE ? "on" : "off");

    if(g_dbus_proxy_set_property_basic(default_ctrl->proxy, "Discoverable",
                    DBUS_TYPE_BOOLEAN, &discoverable,
                    _generic_callback, str, g_free) == TRUE)
        return;

    g_free(str);
    return;

}

void BluetoothManager::_generic_callback(const DBusError *error, void *user_data)
{
    char *str = (char*)user_data;

    if (dbus_error_is_set(error))
        LOGE("Failed to set %s: %s", str, error->name);
    else
        LOGI("Changing %s succeeded", str);
}

void BluetoothManager::_pair_reply(DBusMessage *message, void *user_data)
{
    DbusParam* param = (DbusParam*)user_data;
    BluetoothManager* pThis = param->mThis;
    void* data       = param->mData;
    delete param;

    GDBusProxy *proxy = (GDBusProxy *)data;
    DBusError error;

    dbus_error_init(&error);

    if(dbus_set_error_from_message(&error, message) == TRUE)
    {
        LOGE("Failed to pair: %s", error.name);
        pThis->sendEventError(BT_ERROR_PAIRING_FAILED, error.name);
        dbus_error_free(&error);
        return;
    }

    pThis->sendEventFromProxy(BT_EVENT_PAIR_DEVICE, proxy);
    LOGI("Pairing successful");
}

void BluetoothManager::pair(const char* device)
{
    GDBusProxy *proxy;

    LOGI("----- REQUEST PAIRING : %s", device);

    proxy = find_device(device);
    if(!proxy)
        return;

    if(g_dbus_proxy_method_call(proxy, "Pair", NULL, _pair_reply, new DbusParam(this, proxy), NULL) == FALSE)
    {
        LOGE("Failed to pair");
        return;
    }

    LOGI("Attempting to pair with %s", device);
}

void BluetoothManager::trust(const char* device)
{
    GDBusProxy *proxy;

    LOGI("----- SET TRUST : %s", device);

    proxy = find_device(device);
    if(!proxy)
        return;

    char *str;
    dbus_bool_t trusted = TRUE;

    str = g_strdup_printf("%s trust", device);

    if(g_dbus_proxy_set_property_basic(proxy, "Trusted", DBUS_TYPE_BOOLEAN, &trusted, _generic_callback, str, g_free) == TRUE)
        return;

    g_free(str);
}

void BluetoothManager::_connect_reply(DBusMessage *message, void *user_data)
{
    DbusParam* param = (DbusParam*)user_data;
    BluetoothManager* pThis = param->mThis;
    void* data       = param->mData;
    delete param;

    GDBusProxy *proxy = (GDBusProxy *)data;
    DBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE) {
        LOGE("Failed to connect: %s", error.name);
        pThis->sendEventError(BT_ERROR_CONNECT_FAILED, error.name);
        dbus_error_free(&error);
        return;
    }

    LOGI("Connection successful");

    pThis->sendEventFromProxy(BT_EVENT_CONNECT_DEVICE, proxy);
    pThis->set_default_device(proxy, NULL);
}

void BluetoothManager::connect(const char* device)
{
    GDBusProxy *proxy;

    LOGI("----- REQUEST CONNECT : %s", device);

    if(!device || !strlen(device))
    {
        LOGE("Missing device address argument");
        return;
    }

    if(check_default_ctrl() == FALSE)
        return;

    proxy = find_proxy_by_address(default_ctrl->devices, device);
    if (!proxy)
    {
        LOGE("Device %s not available", device);
        return;
    }

    if(g_dbus_proxy_method_call(proxy, "Connect", NULL, _connect_reply, new DbusParam(this, proxy), NULL) == FALSE)
    {
        LOGE("Failed to connect");
        return;
    }

    LOGI("Attempting to connect to %s", device);
}

void BluetoothManager::_disconn_reply(DBusMessage *message, void *user_data)
{
    DbusParam* param = (DbusParam*)user_data;
    BluetoothManager* pThis = param->mThis;
    void* data       = param->mData;
    bool remove      = param->mArg;
    delete param;

    GDBusProxy *proxy = (GDBusProxy *)data;
    DBusError error;

    dbus_error_init(&error);

    if(dbus_set_error_from_message(&error, message) == TRUE)
    {
        LOGE("Failed to disconnect: %s", error.name);
        pThis->sendEventError(BT_ERROR_DISCONNECT_FAILED, error.name);
        dbus_error_free(&error);
        return;
    }

    LOGI("Successful disconnected");

    if (remove)
    {
        LOGI("try remove");
        pThis->remove_device(proxy);
    }

    if (proxy != pThis->default_dev)
        return;

    pThis->set_default_device(NULL, NULL);
}

void BluetoothManager::disconnect(const char* device, bool remove)
{
    GDBusProxy *proxy;

    LOGI("----- REQUEST DISCONNECT : %s", device);

    proxy = find_device(device);
    if(!proxy)
        return;

    if(g_dbus_proxy_method_call(proxy, "Disconnect", NULL, _disconn_reply, new DbusParam(this, remove, proxy), NULL) == FALSE)
    {
        LOGE("Failed to disconnect");
        return;
    }
    if(strlen(device) == 0)
    {
        DBusMessageIter iter;

        if (g_dbus_proxy_get_property(proxy, "Address", &iter) == TRUE)
            dbus_message_iter_get_basic(&iter, &device);
    }
    LOGI("Attempting to disconnect from %s", device);
}

void BluetoothManager::_remove_device_reply(DBusMessage *message, void *user_data)
{
    UNUSED(user_data);

    DBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE) {
        LOGE("Failed to remove device: %s", error.name);
        dbus_error_free(&error);
        return;
    }

    LOGI("Device has been removed");
}

void BluetoothManager::_remove_device_setup(DBusMessageIter *iter, void *user_data)
{
    const char *path = (const char*)user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

void BluetoothManager::remove_device(GDBusProxy *proxy)
{
    char *path;

    path = g_strdup(g_dbus_proxy_get_path(proxy));

    if (!default_ctrl)
        return;

    if (g_dbus_proxy_method_call(default_ctrl->proxy, "RemoveDevice",
                        _remove_device_setup,
                        _remove_device_reply,
                        path, g_free) == FALSE) {
        LOGE("Failed to remove device");
        g_free(path);
    }
}

void BluetoothManager::remove(const char* device)
{
    GDBusProxy *proxy;

    LOGI("----- REQUEST REMOVE : %s", device);

    if(check_default_ctrl() == FALSE)
        return;

    if(!device)
    {
        LOGE("device is null !");
    }
    proxy = find_proxy_by_address(default_ctrl->devices, device);
    if(!proxy)
    {
        LOGE("Device %s is not avaliable", device);
        return;
    }

    remove_device(proxy);
}

void BluetoothManager::removeDeviceAll()
{
    GList *list;

    if(check_default_ctrl() == FALSE)
        return;

    for(list = default_ctrl->devices; list; list = g_list_next(list))
    {
        BluetoothDevice device;
        GDBusProxy *proxy = (GDBusProxy *)list->data;
        if(!getDeviceFromProxy(proxy, &device))
        {
            remove_device(proxy);
        }
    }
}

void BluetoothManager::removeDeviceAllWithoutPaired()
{
    GList *list;

    if(check_default_ctrl() == FALSE)
        return;

    for(list = default_ctrl->devices; list; list = g_list_next(list))
    {
        BluetoothDevice device;
        GDBusProxy *proxy = (GDBusProxy *)list->data;
        if(!getDeviceFromProxy(proxy, &device) && !device.isPaired())
        {
            remove_device(proxy);
        }
    }
}

int  BluetoothManager::getDeviceFromProxy(GDBusProxy* proxy, BluetoothDevice* device)
{
    DBusMessageIter iter;
    const char *address, *name, *icon;
    dbus_bool_t paired, connected;

    if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
        return -1;

    dbus_message_iter_get_basic(&iter, &address);

    if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
        dbus_message_iter_get_basic(&iter, &name);
    else
        name = "<unknown>";

    if (g_dbus_proxy_get_property(proxy, "Connected", &iter) == TRUE)
        dbus_message_iter_get_basic(&iter, &connected);
    else
        connected = FALSE;

    if (g_dbus_proxy_get_property(proxy, "Paired", &iter) == TRUE)
        dbus_message_iter_get_basic(&iter, &paired);
    else
        paired = FALSE;

    BluetoothType_e eType = BT_TYPE_UNKNOWN;
    if (g_dbus_proxy_get_property(proxy, "Icon", &iter) == TRUE)
    {
        dbus_message_iter_get_basic(&iter, &icon);
        if(!strcmp(icon, "phone"))
            eType = BT_TYPE_PHONE;
        else if(!strcmp(icon, "modem"))
            eType = BT_TYPE_MODEM;
        else if(!strcmp(icon, "audio-card"))
            eType = BT_TYPE_AUDIO;
        else if(!strcmp(icon, "camera-video"))
            eType = BT_TYPE_VIDEO;
        else if(!strcmp(icon, "camera-photo"))
            eType = BT_TYPE_PHOTO;
        else if(!strcmp(icon, "input-gaming"))
            eType = BT_TYPE_INPUT;
        else if(!strcmp(icon, "input-keyboard"))
            eType = BT_TYPE_INPUT;
        else if(!strcmp(icon, "input-tablet"))
            eType = BT_TYPE_INPUT;
        else if(!strcmp(icon, "input-mouse"))
            eType = BT_TYPE_INPUT;
        else if(!strcmp(icon, "printer"))
            eType = BT_TYPE_PRINTER;
    }
    else if (g_dbus_proxy_get_property(proxy, "Appearance", &iter) == TRUE)
    {
        dbus_uint16_t value16;
        dbus_message_iter_get_basic(&iter, &value16);
        if ((value16 >= 0x03C0 && value16 <= 0x03FF) // HID
         || (value16 >= 0x0180 && value16 <= 0x01BF)) // Remote Controller
            eType =  BT_TYPE_INPUT;

    }

#if 1 /* W/A Code */
    static const char* MEDITHINQ_RCU_PREFIX_LIST[] =
    {
        "98:8B:69",
        "08:EB:29",
        "FF:AA:55"
    };

    char devname[1024];
    if (eType == BT_TYPE_INPUT)
    {
        for (int ii = 0; ii < NELEM(MEDITHINQ_RCU_PREFIX_LIST); ii++)
        {
            if (strncasecmp(MEDITHINQ_RCU_PREFIX_LIST[ii], address, 8) == 0)
            {
                sprintf(devname, "MetascopeRCU_%c%c_%c%c_%c%c",
                         address[9], address[10], address[12], address[13], address[15], address[16]);
                name = devname;
                break;
            }
        }
    }
#endif
    device->setDevice(address, name, connected, paired, eType);
    return 0;
}

void  BluetoothManager::sendEventFromProxy(int eventId, GDBusProxy* proxy)
{
    BluetoothDevice* device = new BluetoothDevice();

    if(getDeviceFromProxy(proxy, device))
    {
        delete device;
        return;
    }

    mEvtQ.sendEvent(eventId, (uintptr_t)device);
}

void BluetoothManager::sendEventError(BluetoothError_e errNum, const char* errMsg)
{
    BtError* err = new BtError(errNum, errMsg);

    mEvtQ.sendEvent(BT_EVENT_ERROR, (uintptr_t)err);
}

void BluetoothManager::getDeviceList(std::vector<BluetoothDevice*>& list)
{
    GList *ll;

    if (check_default_ctrl() == FALSE)
        return;

    for (ll = g_list_first(default_ctrl->devices);
            ll; ll = g_list_next(ll)) {
        GDBusProxy *proxy = (GDBusProxy*)ll->data;

        BluetoothDevice* device = new BluetoothDevice();
        if(!getDeviceFromProxy(proxy, device))
            list.push_back(device);
        else
            delete device;
    }
}

void BluetoothManager::getDeviceListWithoutPaired(std::vector<BluetoothDevice*>& list)
{
    GList *ll;

    if (check_default_ctrl() == FALSE)
        return;

    for (ll = g_list_first(default_ctrl->devices);
            ll; ll = g_list_next(ll)) {
        GDBusProxy *proxy = (GDBusProxy*)ll->data;

        BluetoothDevice* device = new BluetoothDevice();
        if(!getDeviceFromProxy(proxy, device) && !device->isPaired())
            list.push_back(device);
        else
            delete device;
    }
}

void BluetoothManager::getPairedDeviceList(std::vector<BluetoothDevice*>& list)
{
    GList *ll;

    if (check_default_ctrl() == FALSE)
        return;

    for (ll = g_list_first(default_ctrl->devices);
            ll; ll = g_list_next(ll)) {
        GDBusProxy *proxy = (GDBusProxy*)ll->data;
        DBusMessageIter iter;
        dbus_bool_t paired;

        if (g_dbus_proxy_get_property(proxy, "Paired", &iter) == FALSE)
            continue;

        dbus_message_iter_get_basic(&iter, &paired);
        if (!paired)
            continue;

        BluetoothDevice* device = new BluetoothDevice();
        if(!getDeviceFromProxy(proxy, device))
            list.push_back(device);
        else
            delete device;
    }
}

void BluetoothManager::_connect_handler(DBusConnection *connection, void *user_data)
{
__TRACE__
    UNUSED(connection);
    UNUSED(user_data);
    // TBD.
}

void BluetoothManager::_disconnect_handler(DBusConnection *connection, void *user_data)
{
__TRACE__
    UNUSED(connection);
    UNUSED(user_data);
    // TBD.
}

void BluetoothManager::_message_handler(DBusConnection *connection, DBusMessage *message, void *user_data)
{
__TRACE__
    UNUSED(connection);
    UNUSED(message);
    UNUSED(user_data);
    // TBD.
}

void BluetoothManager::_proxy_added(GDBusProxy *proxy, void *user_data)
{
    BluetoothManager* pThis = (BluetoothManager*)user_data;

    pThis->proxy_added(proxy, NULL);
}

void BluetoothManager::_proxy_removed(GDBusProxy *proxy, void *user_data)
{
    BluetoothManager* pThis = (BluetoothManager*)user_data;

    pThis->proxy_removed(proxy, NULL);
}

void BluetoothManager::_property_changed(GDBusProxy *proxy, const char *name, DBusMessageIter *iter, void *user_data)
{
    BluetoothManager* pThis = (BluetoothManager*)user_data;

    pThis->property_changed(proxy, name, iter, NULL);
}

void BluetoothManager::run()
{
__TRACE__
    GDBusClient* client;

    g_dbus_attach_object_manager(dbus_conn);

    client = g_dbus_client_new(dbus_conn, "org.bluez", "/org/bluez");
    g_dbus_client_set_connect_watch(client, _connect_handler, this);
    g_dbus_client_set_disconnect_watch(client, _disconnect_handler, this);
    g_dbus_client_set_signal_watch(client, _message_handler, this);

    g_dbus_client_set_proxy_handlers(client, _proxy_added, _proxy_removed, _property_changed, this);

    g_main_loop_run(main_loop);

    g_dbus_client_unref(client);
}

BluetoothManager::BluetoothManager()
                 : Task("BluetoothManager")
{
    mEvtQ.setHandler(this);
}

BluetoothManager::~BluetoothManager()
{
    mEvtQ.setHandler(NULL);
}

bool BluetoothManager::onPreStart()
{
    auto_register_agent = g_strdup("");
    main_loop = g_main_loop_new(NULL, FALSE);
    dbus_conn = g_dbus_setup_bus(DBUS_BUS_SYSTEM, NULL, NULL);

    return true;
}

void BluetoothManager::onPreStop()
{
    g_main_loop_quit(main_loop);
}

void BluetoothManager::onPostStop()
{
    g_main_loop_unref(main_loop);
    dbus_connection_unref(dbus_conn);

    g_free(auto_register_agent);

    main_loop = NULL;
    dbus_conn = NULL;
    agent_manager = NULL;
    auto_register_agent = NULL;
}

//////////// FROM BT Client Source code ////////////////////////////
void BluetoothManager::proxy_added(GDBusProxy *proxy, void *user_data)
{
    UNUSED(user_data);

    const char *interface;

    interface = g_dbus_proxy_get_interface(proxy);

    LOGI("%s", interface);
    if (!strcmp(interface, "org.bluez.Device1")) {
        device_added(proxy);
    } else if (!strcmp(interface, "org.bluez.Adapter1")) {
        adapter_added(proxy);
    } else if (!strcmp(interface, "org.bluez.AgentManager1")) {
        if (!agent_manager) {
            agent_manager = proxy;

            if (auto_register_agent)
                agent_register(dbus_conn, agent_manager, auto_register_agent);
        }
    }
#if 0  // TODO
    else if (!strcmp(interface, "org.bluez.GattService1")) {
        if (service_is_child(proxy))
            gatt_add_service(proxy);
    } else if (!strcmp(interface, "org.bluez.GattCharacteristic1")) {
        gatt_add_characteristic(proxy);
    } else if (!strcmp(interface, "org.bluez.GattDescriptor1")) {
        gatt_add_descriptor(proxy);
    } else if (!strcmp(interface, "org.bluez.GattManager1")) {
        gatt_add_manager(proxy);
    } else if (!strcmp(interface, "org.bluez.LEAdvertisingManager1")) {
        ad_manager_added(proxy);
    }
#endif
}

void BluetoothManager::proxy_removed(GDBusProxy *proxy, void *user_data)
{
    UNUSED(user_data);

    const char *interface;

    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, "org.bluez.Device1")) {
        device_removed(proxy);
    } else if (!strcmp(interface, "org.bluez.Adapter1")) {
        adapter_removed(proxy);
    } else if (!strcmp(interface, "org.bluez.AgentManager1")) {
        if (agent_manager == proxy) {
            agent_manager = NULL;
            if (auto_register_agent)
                agent_unregister(dbus_conn, NULL);
        }
    }
#if 0  // TODO
    else if (!strcmp(interface, "org.bluez.GattService1")) {
        gatt_remove_service(proxy);

        if (default_attr == proxy)
            set_default_attribute(NULL);
    } else if (!strcmp(interface, "org.bluez.GattCharacteristic1")) {
        gatt_remove_characteristic(proxy);

        if (default_attr == proxy)
            set_default_attribute(NULL);
    } else if (!strcmp(interface, "org.bluez.GattDescriptor1")) {
        gatt_remove_descriptor(proxy);

        if (default_attr == proxy)
            set_default_attribute(NULL);
    } else if (!strcmp(interface, "org.bluez.GattManager1")) {
        gatt_remove_manager(proxy);
    } else if (!strcmp(interface, "org.bluez.LEAdvertisingManager1")) {
        if(!dbus_conn){
            ad_unregister(dbus_conn, NULL);
        }
    }
#endif
}

void BluetoothManager::property_changed(GDBusProxy *proxy, const char *name, DBusMessageIter *iter, void *user_data)
{
    UNUSED(user_data);
    const char *interface;
    struct adapter *ctrl;

    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, "org.bluez.Device1")) {
        if (default_ctrl && device_is_child(proxy, default_ctrl->proxy) == TRUE) {
            DBusMessageIter addr_iter;
            char *str;

            if (g_dbus_proxy_get_property(proxy, "Address",
                            &addr_iter) == TRUE) {
                const char *address;

                dbus_message_iter_get_basic(&addr_iter,
                                &address);
                str = g_strdup_printf("[" COLORED_CHG
                        "] Device %s ", address);
            } else
                str = g_strdup("");

            if (strcmp(name, "Connected") == 0) {
                dbus_bool_t connected;

                dbus_message_iter_get_basic(iter, &connected);

                if (connected && default_dev == NULL)
                {
                    set_default_device(proxy, NULL);
                    sendEventFromProxy(BT_EVENT_CONNECT_DEVICE, proxy);
                }
                else if (!connected && default_dev == proxy)
                {
                    set_default_device(NULL, NULL);
                    sendEventFromProxy(BT_EVENT_DISCONNECT_DEVICE, proxy);
                }
            }

            sendEventFromProxy(BT_EVENT_CHANGE_DEVICE, proxy);

            g_free(str);
        }
    } else if (!strcmp(interface, "org.bluez.Adapter1")) {
        DBusMessageIter addr_iter;
        char *str;

        if (g_dbus_proxy_get_property(proxy, "Address",
                        &addr_iter) == TRUE) {
            const char *address;

            dbus_message_iter_get_basic(&addr_iter, &address);
            str = g_strdup_printf("[" COLORED_CHG
                        "] Controller %s ", address);
        } else
            str = g_strdup("");

        // TBD.
        // print_iter(str, name, iter);
        g_free(str);
    } else if (!strcmp(interface, "org.bluez.LEAdvertisingManager1")) {
        DBusMessageIter addr_iter;
        char *str;

        ctrl = find_ctrl(ctrl_list, g_dbus_proxy_get_path(proxy));
        if (!ctrl)
            return;

        if (g_dbus_proxy_get_property(ctrl->proxy, "Address",
                        &addr_iter) == TRUE) {
            const char *address;

            dbus_message_iter_get_basic(&addr_iter, &address);
            str = g_strdup_printf("[" COLORED_CHG
                        "] Controller %s ",
                        address);
        } else
            str = g_strdup("");

        // TBD.
        // print_iter(str, name, iter);
        g_free(str);
    } else if (proxy == default_attr) {
        char *str;

        str = g_strdup_printf("[" COLORED_CHG "] Attribute %s ",
                        g_dbus_proxy_get_path(proxy));

        // TBD.
        // print_iter(str, name, iter);
        g_free(str);
    }
}

void BluetoothManager::print_adapter(GDBusProxy *proxy, const char *description)
{
    DBusMessageIter iter;
    const char *address, *name;

    if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
        return;

    dbus_message_iter_get_basic(&iter, &address);

    if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
        dbus_message_iter_get_basic(&iter, &name);
    else
        name = "<unknown>";

    LOGD("%s%s%sController %s %s %s",
                description ? "[" : "",
                description ? : "",
                description ? "] " : "",
                address, name,
                default_ctrl &&
                default_ctrl->proxy == proxy ?
                "[default]" : "");

}

void BluetoothManager::print_device(GDBusProxy *proxy, const char *description)
{
    DBusMessageIter iter;
    const char *address, *name;

    if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
        return;

    dbus_message_iter_get_basic(&iter, &address);

    if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
        dbus_message_iter_get_basic(&iter, &name);
    else
        name = "<unknown>";

    LOGD("%s%s%sDevice %s %s",
            description ? "[" : "",
            description ? : "",
            description ? "] " : "",
            address, name);
}

gboolean BluetoothManager::device_is_child(GDBusProxy *device, GDBusProxy *master)
{
    DBusMessageIter iter;
    const char *adapter, *path;

    if (!master)
        return FALSE;

    if (g_dbus_proxy_get_property(device, "Adapter", &iter) == FALSE)
        return FALSE;

    dbus_message_iter_get_basic(&iter, &adapter);
    path = g_dbus_proxy_get_path(master);

    if (!strcmp(path, adapter))
        return TRUE;

    return FALSE;
}

gboolean BluetoothManager::service_is_child(GDBusProxy *service)
{
    GList *l;
    DBusMessageIter iter;
    const char *device, *path;

    if (g_dbus_proxy_get_property(service, "Device", &iter) == FALSE)
        return FALSE;

    dbus_message_iter_get_basic(&iter, &device);

    if (!default_ctrl)
        return FALSE;

    for (l = default_ctrl->devices; l; l = g_list_next(l)) {
        struct GDBusProxy *proxy = (GDBusProxy *)l->data;

        path = g_dbus_proxy_get_path(proxy);

        if (!strcmp(path, device))
            return TRUE;
    }

    return FALSE;
}

struct GDBusProxy* BluetoothManager::find_device(const char *arg)
{
    GDBusProxy *proxy;

    if (!arg || !strlen(arg))
    {
        if (default_dev)
            return default_dev;
        LOGE("Missing device address argument");
        return NULL;
    }

    if (check_default_ctrl() == FALSE)
        return NULL;

    proxy = find_proxy_by_address(default_ctrl->devices, arg);
    if (!proxy)
    {
        LOGE("Device %s not available", arg);
        return NULL;
    }

    return proxy;
}

struct adapter* BluetoothManager::find_parent(GDBusProxy *device)
{
    GList *list;

    for (list = g_list_first(ctrl_list); list; list = g_list_next(list)) {
        struct adapter *adapter = (struct adapter *)list->data;

        if (device_is_child(device, adapter->proxy) == TRUE)
            return adapter;
    }
    return NULL;
}

void BluetoothManager::set_default_device(GDBusProxy *proxy, const char *attribute)
{
    char *desc = NULL;
    DBusMessageIter iter;
    const char *path;

    default_dev = proxy;

    if (proxy == NULL) {
        default_attr = NULL;
        goto done;
    }

    if (!g_dbus_proxy_get_property(proxy, "Alias", &iter)) {
        if (!g_dbus_proxy_get_property(proxy, "Address", &iter))
            goto done;
    }

    path = g_dbus_proxy_get_path(proxy);

    dbus_message_iter_get_basic(&iter, &desc);
    desc = g_strdup_printf(ANSI_COLOR_BLUE "[%s%s%s]" ANSI_COLOR_RESET "# ", desc,
                attribute ? ":" : "",
                attribute ? attribute + strlen(path) : "");

done:
    g_free(desc);
}

void BluetoothManager::device_added(GDBusProxy *proxy)
{
    DBusMessageIter iter;
    struct adapter *adapter = find_parent(proxy);

    if (!adapter) {
        /* TODO: Error */
        return;
    }

    adapter->devices = g_list_append(adapter->devices, proxy);
    print_device(proxy, COLORED_NEW);
    sendEventFromProxy(BT_EVENT_ADD_DEVICE, proxy);

    if (default_dev)
        return;

    if (g_dbus_proxy_get_property(proxy, "Connected", &iter)) {
        dbus_bool_t connected;

        dbus_message_iter_get_basic(&iter, &connected);

        if (connected)
            set_default_device(proxy, NULL);
    }
}

struct adapter* BluetoothManager::adapter_new(GDBusProxy *proxy)
{
    UNUSED(proxy);

    struct adapter *adapter = (struct adapter *)g_malloc0(sizeof(struct adapter));

    ctrl_list = g_list_append(ctrl_list, adapter);

    if (!default_ctrl)
        default_ctrl = adapter;

    return adapter;
}

void BluetoothManager::adapter_added(GDBusProxy *proxy)
{
    struct adapter *adapter;
    adapter = find_ctrl(ctrl_list, g_dbus_proxy_get_path(proxy));
    if (!adapter)
        adapter = adapter_new(proxy);

    adapter->proxy = proxy;

    print_adapter(proxy, COLORED_NEW);
}

void BluetoothManager::ad_manager_added(GDBusProxy *proxy)
{
    struct adapter *adapter;
    adapter = find_ctrl(ctrl_list, g_dbus_proxy_get_path(proxy));
    if (!adapter)
        adapter = adapter_new(proxy);

    adapter->ad_proxy = proxy;
}

void BluetoothManager::set_default_attribute(GDBusProxy *proxy)
{
    const char *path;

    default_attr = proxy;

    path = g_dbus_proxy_get_path(proxy);

    set_default_device(default_dev, path);
}

void BluetoothManager::device_removed(GDBusProxy *proxy)
{
    struct adapter *adapter = find_parent(proxy);
    if (!adapter) {
        /* TODO: Error */
        return;
    }

    adapter->devices = g_list_remove(adapter->devices, proxy);

    print_device(proxy, COLORED_DEL);
    sendEventFromProxy(BT_EVENT_REMOVE_DEVICE, proxy);

    if (default_dev == proxy)
        set_default_device(NULL, NULL);
}

void BluetoothManager::adapter_removed(GDBusProxy *proxy)
{
    GList *ll;

    for (ll = g_list_first(ctrl_list); ll; ll = g_list_next(ll)) {
        struct adapter *adapter = (struct adapter *)ll->data;

        if (adapter->proxy == proxy) {
            print_adapter(proxy, COLORED_DEL);

            if (default_ctrl && default_ctrl->proxy == proxy) {
                default_ctrl = NULL;
                set_default_device(NULL, NULL);
            }

            ctrl_list = g_list_remove_link(ctrl_list, ll);
            g_list_free(adapter->devices);
            g_free(adapter);
            g_list_free(ll);
            return;
        }
    }
}

struct adapter* BluetoothManager::find_ctrl(GList *source, const char *path)
{
    GList *list;

    for (list = g_list_first(source); list; list = g_list_next(list)) {
        struct adapter *adapter = (struct adapter *)list->data;

        if (!strcasecmp(g_dbus_proxy_get_path(adapter->proxy), path))
            return adapter;
    }

    return NULL;
}

struct adapter* BluetoothManager::find_ctrl_by_address(GList *source, const char *address)
{
    GList *list;

    for (list = g_list_first(source); list; list = g_list_next(list)) {
        struct adapter *adapter = (struct adapter *)list->data;
        DBusMessageIter iter;
        const char *str;

        if (g_dbus_proxy_get_property(adapter->proxy,
                    "Address", &iter) == FALSE)
            continue;

        dbus_message_iter_get_basic(&iter, &str);

        if (!strcasecmp(str, address))
            return adapter;
    }

    return NULL;
}

GDBusProxy* BluetoothManager::find_proxy_by_address(GList *source, const char *address)
{
    GList *list;

    for (list = g_list_first(source); list; list = g_list_next(list)) {
        GDBusProxy *proxy = (GDBusProxy *)list->data;
        DBusMessageIter iter;
        const char *str;

        if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
            continue;

        dbus_message_iter_get_basic(&iter, &str);

        if (!strcasecmp(str, address))
            return proxy;
    }

    return NULL;
}

gboolean BluetoothManager::check_default_ctrl(void)
{
    if (!default_ctrl) {
        LOGE("No default controller available");
        return FALSE;
    }

    return TRUE;
}
void BluetoothManager::cmd_bt_scan(void* session, int argc, char** argv, void* param)
{
    UNUSED(session);

    BluetoothManager* pThis = (BluetoothManager*)param;

    if(argc != 2)
        return;

    bool mode = atoi(argv[1]) == 1 ? true : false;
    pThis->scan(mode);
}

void BluetoothManager::cmd_bt_discoverable(void* session, int argc, char** argv, void* param)
{
    UNUSED(session);

    BluetoothManager* pThis = (BluetoothManager*)param;

    if(argc != 2)
        return;

    bool mode = atoi(argv[1]) == 1 ? true : false;
    pThis->discoverable(mode);

}

void BluetoothManager::cmd_bt_pair(void* session, int argc, char** argv, void* param)
{
    UNUSED(session);

    BluetoothManager* pThis = (BluetoothManager*)param;

    if(argc != 2)
        return;

    char dev[1024];
    strncpy(dev, argv[1], sizeof(dev));
    dev[sizeof(dev)-1] = 0;

    pThis->pair(dev);
}

void BluetoothManager::cmd_bt_trust(void* session, int argc, char** argv, void* param)
{
    UNUSED(session);

    BluetoothManager* pThis = (BluetoothManager*)param;

    if(argc != 2)
        return;

    char dev[1024];
    strncpy(dev, argv[1], sizeof(dev));
    dev[sizeof(dev)-1] = 0;

    pThis->trust(dev);
}
