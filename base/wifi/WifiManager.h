/**
 * Simple Wifi Manager
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Copyright (c) 2022, MedithinQ. All rights reserved.
 */
#ifndef __WIFI_MANAGER_H_
#define __WIFI_MANAGER_H_

#include "Task.h"
#include "CondVar.h"

#include <string>
#include <vector>
#include <list>

#include "wpa_ctrl.h"

#define MANAGER_FREQ 5180
#define MANAGER_FREQ_6G 6615

const char* get_passphrase(char* passphrase, const char* ssid, const char* psk);

class WifiDevice
{
public:
    WifiDevice()
        : mBssid(""),
          mFrequency(0),
          mSignalLevel(0),
          mFlags(""),
          mSsid("") { }

    WifiDevice(const char* str);

    WifiDevice(WifiDevice* dev)
        : mBssid(dev->mBssid),
          mFrequency(dev->mFrequency),
          mSignalLevel(dev->mSignalLevel),
          mFlags(dev->mFlags),
          mSsid(dev->mSsid) { }

    WifiDevice(const std::string& bssid, int frequency, int signalLevel, const std::string& flags, const std::string& ssid)
        : mBssid(bssid),
          mFrequency(frequency),
          mSignalLevel(signalLevel),
          mFlags(flags),
          mSsid(ssid) { }

    ~WifiDevice() { }

    void setDevice(const std::string& bssid, int frequency, int signalLevel, const std::string& flags, const std::string& ssid)
    {
        mBssid       = bssid;
        mFrequency   = frequency;
        mSignalLevel = signalLevel;
        mFlags       = flags;
        mSsid        = ssid;
    }

    const std::string& getBssid() { return mBssid; }

    int getFrequency() { return mFrequency; }
    int getSignalLevel() { return mSignalLevel; }

    const char* getFlags() const { return mFlags.c_str(); }
    const char* getSsid() const { return mSsid.c_str(); }


private:
    std::string     mBssid;
    int             mFrequency;
    int             mSignalLevel;
    std::string     mFlags;
    std::string     mSsid;
};

class IWifiListener
{
public:
    virtual ~IWifiListener() { }

    virtual void onWifiConnected() { };
    virtual void onWifiDisconnected(const char* str) { UNUSED(str); };
    virtual void onWifiScanStarted() { };
    virtual void onWifiScanCompleted(std::vector<WifiDevice*>& list) { UNUSED(list); };
};

class WifiManager : public Task
{
public:
    static WifiManager& getInstance();

    void addListener(IWifiListener* listener);
    void removeListener(IWifiListener* listener);

    bool enable();
    void disable();

    bool isConnected();
    bool connect(const WifiDevice& dev, const char* psk);
    bool connect(const char* ssid, const char* psk, const char* keymgmt="WPA-PSK", const char* proto=NULL, const char* pairwise=NULL, const char* group=NULL);
    void disconnect();

    bool startApMode(const char* ssid, const char* psk);

    bool scan();
    void scanResults(std::vector<WifiDevice*>& list);

    bool wifiDevInfo(const char* ssid, WifiDevice& dev);

private:
    WifiManager();
    ~WifiManager();

    void insmod();
    void rmmod();

    bool start() { return Task::start(); }
    void stop()  { Task::stop(); }

    bool onPreStart();
    void onPreStop();
    void onPostStop();

    int  listNetwork();
    bool addNetwork();

    const char* getReason(const char* str);

    bool setSSID(const char* ssid);
    bool setScanSsid(int val);
    bool setPsk(const char* psk);
    bool setSAE(const char* pwd);
    bool setCommand(const char* cmd);

    bool setKeyMgmt(const char* keyMgmt);
    bool setProto(const char* proto);
    bool setPairwise(const char* pairwise);
    bool setGroup(const char* group);

    bool enableNetwork(int networkNum);
    void disableNetwork(int networkNum);
    bool removeNetwork(int networkNum);

    bool setApScan(int value);
    bool setCountry(const char* country);
    bool setFrequency(int value);
    bool setMode(int value);
    bool setRequireHt40(int value);

    void processEvent(const char* str);
    void run();

    bool startSupplicant();
    void stopSupplicant();

    bool connectSupplicant();
    void disconnectSupplicant();

    bool sendWpaCtrlCommand(const char* cmd, char* buf = NULL, size_t len = 0);

private:
    Mutex   mMutex;
    CondVar mWaitCond;

    bool mExitTask;

    typedef std::list<IWifiListener*> WifiListenerList;
    WifiListenerList mWifiListeners;

    typedef std::vector<WifiDevice*> WifiDeviceList;
    WifiDeviceList mDeviceList;

    struct wpa_ctrl* mCtrlConn;
    struct wpa_ctrl* mMonitorConn;
};

#endif /* __WIFI_MANAGER_H_ */
