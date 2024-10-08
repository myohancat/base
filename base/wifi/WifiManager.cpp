/**
 * Simple Wifi Manager
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Copyright (c) 2022, MedithinQ. All rights reserved.
 */
#include "WifiManager.h"

#include <time.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <algorithm>
#include <stdlib.h>
#include <cstdlib>

#include "ProcessUtil.h"
#include "Log.h"
#include "Util.h"

#define IFCONFIG_WLAN0_UP    "ifconfig wlan0 up"
#define WPA_SUPPLICANT       "wpa_supplicant"
#define WPA_SUPPLICANT_START "wpa_supplicant -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf -B"  
#define CTRL_IFACE_DIR       "/var/run/wpa_supplicant"
#define WIFI_INTF            "wlan0"
#define WIFI_DRIVER          "8852ce"

const char* get_passphrase(char* passphrase, const char* ssid, const char* psk)
{
    const char* ret = NULL;
    char cmd[1024];
    char line[1024];

    sprintf(cmd, "/usr/bin/wpa_passphrase \"%s\" %s", ssid, psk);

    FILE* fp = popen(cmd, "r");
    if(fp == NULL)
        return ret;

    while(fgets(line, sizeof(line), fp) != NULL)
    {
        char* p = ltrim(line);
        if(strncmp(p, "psk=", 4) == 0)
        {
            p = rtrim(p+4);
            strcpy(passphrase, p);
            ret = passphrase;
            break;
        }
    }

    pclose(fp);

    return ret;
}

enum
{
    WIFI_DEVICE_BSSID = 0,
    WIFI_DEVICE_FREQUENCY,
    WIFI_DEVICE_SIGNAL_LEVEL,
    WIFI_DEVICE_FLAGS,
    WIFI_DEVICE_SSID
};

WifiDevice::WifiDevice(const char* str)
{
    char buf[1024];
    char* tok, * saveptr;
    int pos;

    strncpy(buf, str, sizeof(buf));

    for (pos = 0, tok = strtok_r(buf, "\t", &saveptr); tok; tok = strtok_r(NULL, "\t", &saveptr), pos++)
    {
        switch (pos)
        {
            case WIFI_DEVICE_BSSID:
            {
                mBssid = tok;
                break;
            }
            case WIFI_DEVICE_FREQUENCY:
            {
                mFrequency = atoi(tok);
                break;
            }
            case WIFI_DEVICE_SIGNAL_LEVEL:
            {
                mSignalLevel = atoi(tok);
                break;
            }
            case WIFI_DEVICE_FLAGS:
            {
                //mFlags = tok;
				if(strstr(tok, "PSK") != NULL)
					mFlags = "PSK";
				else
					mFlags = "SAE";
                break;
            }
            case WIFI_DEVICE_SSID:
            {
                mSsid = tok;
                break;
            }
        }
    }
}


WifiManager& WifiManager::getInstance()
{
    static WifiManager _obj;

    return _obj;
}

WifiManager::WifiManager()
            : Task("WifiManager"),
              mExitTask(false),
              mCtrlConn(NULL),
              mMonitorConn(NULL)
{
}

WifiManager::~WifiManager()
{
}

bool WifiManager::enable()
{
__TRACE__
    ProcessUtil::system(IFCONFIG_WLAN0_UP);

    if(!startSupplicant())
        return false;

    return start();
}

void  WifiManager::disable()
{
    stop();

    stopSupplicant();
}

void WifiManager::insmod()
{
    ProcessUtil::system("modprobe " WIFI_DRIVER " > /dev/null 2>&1");
    ProcessUtil::system("ifup " WIFI_INTF " > /dev/null 2>&1");
}

void WifiManager::rmmod()
{
    ProcessUtil::system("ifdown " WIFI_INTF " > /dev/null 2>&1");
    ProcessUtil::system("rmmod " WIFI_DRIVER " > /dev/null 2>&1");
}

//fix func connect(dev, psk) 
bool WifiManager::connect(const WifiDevice& dev, const char* psk)
{
    return connect(dev.getSsid(), psk, dev.getFlags());
}

bool WifiManager::connect(const char* ssid, const char* psk, const char* keymgmt, const char* proto, const char* pairwise, const char* group)
{
	if (!addNetwork())
    {
        LOGE("addNetwork error");
        goto ERROR;
    }

    if (!setApScan(1))
    {
        LOGE("AP_SCAN error");
        goto ERROR;
    }
    if (!setSSID(ssid))
    {   
        LOGE("setSSID error");
        goto ERROR;
    }

    if (strcasecmp(keymgmt, "WPA-PSK") == 0)
    {
        if (!setKeyMgmt("WPA-PSK"))
        {
            LOGE("setKeyMgmt error");
            goto ERROR;
        }

        char passphrase[2048];
        if (!setPsk(get_passphrase(passphrase, ssid, psk)))
        {
            LOGE("setPsk error");
            goto ERROR;
        }
    }
    else if (strcasecmp(keymgmt, "SAE") == 0)
    {
        if (!setKeyMgmt("SAE"))
        {
            LOGE("setKeyMgmt error");
            goto ERROR;
        }

        if (!setSAE(psk))
        {
            LOGE("set SAE passward error");
            goto ERROR;
        }

        if (!setCommand("ieee80211w 2"))
        {
            LOGE("set ieee80211w error");
            goto ERROR;
        }
    }
    else
    {
        LOGE("Unsupported keymgmt : %s", keymgmt);
        goto ERROR;
    }

    if (!setScanSsid(1))
    {
        LOGE("setScanSsid error");
        goto ERROR;
    }

    if (proto)
    {
        if (!setProto(proto))
        {
            LOGE("setProto error\n");
            goto ERROR;
        }
    }
    if (pairwise)
    {
        if (!setPairwise(pairwise))
        {
            LOGE("setPairwise error\n");
            goto ERROR;
        }
    }
    if (group)
    {
        if (!setGroup(group))
        {
            LOGE("setGroup error\n");
            goto ERROR;
        }
    }

    if (!enableNetwork(0))
    {   
        LOGE("enableNetwork error");
        goto ERROR;
    }

    return true;

ERROR:
    LOGE("connect failed");
    return false;
}

bool WifiManager::startApMode(const char* ssid, const char* psk)
{
    if(!addNetwork())
    {
        LOGE("addNetwork error");
        goto ERROR;
    }

    if(!setApScan(2))
    {
        LOGE("AP_SCAN error");
        goto ERROR;
    }

    if (!setCountry("US"))
    {
        LOGE("set country US error");
        goto ERROR;
    }

    if(!setSSID(ssid))
    {   
        LOGE("setSSID error");
        goto ERROR;
    }

    if(!setKeyMgmt("WPA-PSK"))
    {   
        LOGE("setKeyMgmt error");
        goto ERROR;
    }

    if(!setPsk(psk))
    {   
        LOGE("setPsk error");
        goto ERROR;
    }

    if(!setFrequency(MANAGER_FREQ))
    {   
        LOGE("setFrequency error");
        goto ERROR;
    }

    if(!setMode(2))
    {   
        LOGE("setMode error");
        goto ERROR;
    }

     if(!setRequireHt40(1))
    {   
        LOGE("setRequire_ht40 error");
        goto ERROR;
    }

    if(!enableNetwork(0))
    {   
        LOGE("enableNetwork error");
        goto ERROR;
    }

    return true;

ERROR:
    LOGE("connect failed : %s", ssid);
    return false;

}

void WifiManager::disconnect()
{
__TRACE__
    disableNetwork(0);
    removeNetwork(0);
}

bool WifiManager::sendWpaCtrlCommand(const char* cmd, char* buf, size_t len)
{
    int ret;
    char tmp[4*1024];
    if (!buf)
    {
        buf = tmp;
        len = sizeof(tmp);
    }

    if (mCtrlConn == NULL)
        return false;

    LOGT("WPA_CTRL : send command : %s", cmd);
    ret = wpa_ctrl_request(mCtrlConn, cmd, strlen(cmd), buf, &len, NULL);
    LOGT("WPA_CTRL : ret : %d", ret);
    if (ret == -2)
    {
        LOGE("'%s' command timed out.", cmd);
        return false;
    }
    else if (ret < 0)
    {
        LOGE("'%s' command failed.", cmd);
        return false;
    }

    buf[len -1] = '\0';
    LOGT("WPA_CTRL%s", buf);

    return true;
}

void WifiManager::addListener(IWifiListener* listener)
{
    Lock lock(mMutex);

    if(!listener)
        return;

    WifiListenerList::iterator it = std::find(mWifiListeners.begin(), mWifiListeners.end(), listener);
    if(listener == *it)
        return;

    mWifiListeners.push_back(listener);
}

void WifiManager::removeListener(IWifiListener* listener)
{
    Lock lock(mMutex);

    if(!listener)
        return;

    for(WifiListenerList::iterator it = mWifiListeners.begin(); it != mWifiListeners.end(); it++)
    {
        if(listener == *it)
        {
            mWifiListeners.erase(it);
            return;
        }
    }
}

bool WifiManager::onPreStart()
{
    mExitTask = false;
    return connectSupplicant();
}

void WifiManager::onPreStop()
{
    mExitTask = true;
}

void WifiManager::onPostStop()
{
    disconnectSupplicant();
}

void WifiManager::processEvent(const char* str)
{
    if (strncmp(str, "CTRL-EVENT-CONNECTED", 20) == 0)
    {
        Lock lock(mMutex);
        for(WifiListenerList::iterator it = mWifiListeners.begin(); it != mWifiListeners.end(); it++)
            (*it)->onWifiConnected();
    }
    else if (strncmp(str, "CTRL-EVENT-DISCONNECTED", 23) == 0)
    {
		const char* reason;
		reason = getReason(str);

		Lock lock(mMutex);
        for(WifiListenerList::iterator it = mWifiListeners.begin(); it != mWifiListeners.end(); it++)
        {
            LOGD("onWifiDisconnected call~!!");
            (*it)->onWifiDisconnected(reason);
        }
    }
    else if (strncmp(str, "CTRL-EVENT-SCAN-STARTED", 23) == 0)
    {
        Lock lock(mMutex);
        for(WifiListenerList::iterator it = mWifiListeners.begin(); it != mWifiListeners.end(); it++)
            (*it)->onWifiScanStarted();
    }
    else if (strncmp(str, "CTRL-EVENT-SCAN-RESULTS", 23) == 0)
    {
        scanResults(mDeviceList);

        Lock lock(mMutex);
        for(WifiListenerList::iterator it = mWifiListeners.begin(); it != mWifiListeners.end(); it++)
            (*it)->onWifiScanCompleted(mDeviceList);
    }
}

void WifiManager::run()
{
__TRACE__
    struct pollfd sPollFd;
    int ret;
    char line[2*1024];

    sPollFd.fd = wpa_ctrl_get_fd(mMonitorConn);
    sPollFd.events = POLLPRI|POLLIN|POLLERR|POLLHUP|POLLNVAL|POLLRDHUP;
    sPollFd.revents = 0;

    while (!mExitTask)
    {
        ret = poll(&sPollFd, 1, 100);
        if (ret > 0)
        {
            if(sPollFd.revents & (POLLRDHUP | POLLERR | POLLHUP | POLLNVAL))
                break;
            
            if (sPollFd.revents & POLLIN)
            {
                read(sPollFd.fd, line, sizeof(line));
                char* p = line;
                if(line[0] == '<')
                    p = strchr(p + 1, '>') + 1;

                LOGD("%s", p);
                processEvent(p);
				memset(line, '\0', sizeof(line));
            }
        }
    }
}

bool WifiManager::startSupplicant()
{
    int pid = ProcessUtil::get_pid_from_proc_by_name(WPA_SUPPLICANT);

    LOGD("pid : %d", pid);
    if(pid < 0)
    {
        ProcessUtil::system(WPA_SUPPLICANT_START);
    }
    else
        LOGW("wpa_supplicant already executed");
    
    return true;
}

void WifiManager::stopSupplicant()
{
    ProcessUtil::kill(WPA_SUPPLICANT);
}

bool WifiManager::connectSupplicant()
{
    int ii;
    char ctrl_path[1024];
    sprintf(ctrl_path, "%s/%s", CTRL_IFACE_DIR, WIFI_INTF);

    /* W/A code. Wait until to make unix domain socket for IPC */
    for(ii = 0; ii < 1000; ii++)
    {
        if(::access(ctrl_path, F_OK) == 0)
            break;

        usleep(10 * 1000);
    }
    
    if(ii == 1000)
    {
        LOGE("Failed to find ctrl_path : %s", ctrl_path);
        return false;
    }
    /* W/A */

    mCtrlConn = wpa_ctrl_open(ctrl_path);
    if(mCtrlConn == NULL)
    {
        LOGE("Cannot open ctrl interface : %s", ctrl_path);
        return false;
    }

    mMonitorConn = wpa_ctrl_open(ctrl_path);
    if(mMonitorConn == NULL)
    {
        LOGE("Cannot open monitor interface : %s", ctrl_path);
        wpa_ctrl_close(mCtrlConn);
        mCtrlConn = NULL;
        return false;
    }

    wpa_ctrl_attach(mMonitorConn);

    return true;
}

void WifiManager::disconnectSupplicant()
{
    if(mMonitorConn)
    {
        wpa_ctrl_detach(mMonitorConn);
        wpa_ctrl_close(mMonitorConn);
    }

    if(mCtrlConn)
        wpa_ctrl_close(mCtrlConn);
}

bool WifiManager::isConnected()
{
__TRACE__
    char* tok, * saveptr;
    char buf[4*1024];

    if(!sendWpaCtrlCommand("STATUS", buf, sizeof(buf)))
    {
        LOGE("Cannot get status");
        return false;
    }

    for (tok = strtok_r(buf, "\r\n", &saveptr); tok; tok = strtok_r(NULL, "\r\n", &saveptr))
    {
        char* value = strchr(tok, '=') + 1;
        if (strncmp(tok, "wpa_state", 9) == 0)
        {
            if(strncmp(value, "COMPLETED", 9) == 0)
                return true;
        }
    }

    return false;

}

bool WifiManager::scan()
{
    char buf[255];

    if (!sendWpaCtrlCommand("SCAN", buf, sizeof(buf)))
        return false;

    if(strncmp(buf, "OK", 2) != 0)
    {
        LOGE("Failed to scan : %s", buf);
        return false;
    }

    return true;
}

void WifiManager::scanResults(std::vector<WifiDevice*>& list)
{
    char buf[4096];
    char* tok, * saveptr;

    std::vector<WifiDevice*>().swap(mDeviceList);

    if(sendWpaCtrlCommand("SCAN_RESULTS", buf, sizeof(buf)))
    {
        tok = strtok_r(buf, "\r\n", &saveptr);
        for (tok = strtok_r(NULL, "\r\n", &saveptr); tok; tok = strtok_r(NULL, "\r\n", &saveptr))
        {
            WifiDevice* data = new WifiDevice(tok);
            list.push_back(data);
        }
    }
}

bool WifiManager::wifiDevInfo(const char* ssid, WifiDevice& dev)
{
    UNUSED(dev);
    if ((int)mDeviceList.size() < 0)
    {
        LOGE("not exist wifi device");
        return false;
    }

    for (size_t ii = 0; ii < mDeviceList.size(); ii++)
    {
        if (strncmp(mDeviceList.at(ii)->getSsid(), ssid, 16) == 0)
        {
            dev = mDeviceList.at(ii);
            return true;
        }
    }

    return false;
}

const char* WifiManager::getReason(const char* str)
{
	//TODO
	UNUSED(str);
	char disconnStr[1024];
	strcpy(disconnStr, str);

	char* reason = strstr(disconnStr, "reason=");
	if(reason != NULL)
	{
		reason = strtok(reason, "=");
		reason = strtok(NULL, " ");

		return reason;
	}
	else
		return NULL;
}

int  WifiManager::listNetwork()
{
    char buf[255];
    char* tok, * saveptr;

    int n = -1;
    if(sendWpaCtrlCommand("LIST_NETWORKS", buf, sizeof(buf)))
    {
        for (tok = strtok_r(buf, "\r\n", &saveptr); tok; tok = strtok_r(NULL, "\r\n", &saveptr))
            n++;

        return n;
    }

    return 0;
}

bool WifiManager::addNetwork()
{
    char buf[255];

    if (listNetwork() > 0)
        removeNetwork(0);

    if(sendWpaCtrlCommand("ADD_NETWORK", buf, sizeof(buf)))
    {
        return true;
    }

    return false;
}

bool WifiManager::setSSID(const char* ssid)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "SET_NETWORK 0 ssid \"%s\"", ssid);
    if (!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set SSID");
        return false;
    }

    if(strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}

bool WifiManager::setScanSsid(int val)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "SET_NETWORK 0 scan_ssid %d", val);
    if (!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set scan_ssid");
        return false;
    }

    if(strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}

bool WifiManager::setKeyMgmt(const char* keyMgmt)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "SET_NETWORK 0 key_mgmt %s", keyMgmt);
    if (!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set key_mgmt");
        return false;
    }

    if(strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}

bool WifiManager::setSAE(const char* pwd)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "SET_NETWORK 0 sae_password %s", pwd);
    if (!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set sae_password");
        return false;
    }

    if(strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}

bool WifiManager::setCommand(const char* cmd)
{
    char buf[1024];
    char uniteCmd[1024];

    sprintf(uniteCmd, "SET_NETWORK 0 %s", cmd);
    if (!sendWpaCtrlCommand(uniteCmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set %s command", cmd);
        return false;
    }

    if(strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}


bool WifiManager::setProto(const char* proto)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "SET_NETWORK 0 proto %s", proto);
    if (!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set key_mgmt");
        return false;
    }

    if(strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}

bool WifiManager::setPairwise(const char* pairwise)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "SET_NETWORK 0 pairwise %s", pairwise);
    if (!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set key_mgmt");
        return false;
    }

    if(strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}

bool WifiManager::setGroup(const char* group)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "SET_NETWORK 0 group %s", group);
    if (!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set key_mgmt");
        return false;
    }

    if(strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}

bool WifiManager::setPsk(const char* psk)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "SET_NETWORK 0 psk %s", psk);
    if (!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set psk");
        return false;
    }

    if(strncmp(buf, "OK", 2) != 0)
        return false;

    return true;

}

bool WifiManager::enableNetwork(int networkNum)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "ENABLE_NETWORK %d", networkNum);

    sendWpaCtrlCommand(cmd, buf, sizeof(buf));

    if(strncmp(buf, "OK", 2) != 0)
        return false;
    
    return true;
}

void WifiManager::disableNetwork(int networkNum)
{
__TRACE__
    char cmd[1024];

    sprintf(cmd, "DISABLE_NETWORK %d", networkNum);
    sendWpaCtrlCommand(cmd);
}

void WifiManager::removeNetwork(int networkNum)
{
__TRACE__
    char cmd[1024];

    sprintf(cmd, "REMOVE_NETWORK %d", networkNum);
    sendWpaCtrlCommand(cmd);
}

bool WifiManager::setCountry(const char* country)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "SET country %s", country);

    if(!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set country US");
        return false;
    }

    if(strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}

bool WifiManager::setApScan(int value)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "AP_SCAN %d", value);

    if(!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to ap scan");
        return false;
    }

    if(strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}

bool WifiManager::setFrequency(int value)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "SET_NETWORK 0 frequency %d", value);

    if (!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set frequency");
        return false;
    }

    if (strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}

bool WifiManager::setMode(int value)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "SET_NETWORK 0 mode %d", value);

    if (!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set mode");
        return false;
    }

    if (strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}

bool WifiManager::setRequireHt40(int value)
{
    char buf[1024];
    char cmd[1024];

    sprintf(cmd, "SET_NETWORK 0 ht40 %d", value);

    if (!sendWpaCtrlCommand(cmd, buf, sizeof(buf)))
    {
        LOGE("Failed to set require_ht 40");
        return false;
    }

    if (strncmp(buf, "OK", 2) != 0)
        return false;

    return true;
}
