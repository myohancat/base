/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "UsbStorageManager.h"

#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <vector>

#include "Util.h"
#include "Log.h"

#define MOUNT_PREFIX "/mnt/usb"

#define MSG_ID_USB_ADD      0
#define MSG_ID_USB_REMOVE   1

static uint32_t  gBitmapId = 0;
#define MAX_ID   (sizeof(gBitmapId) * 8)

static int alloc_id()
{
    uint32_t mask = 1;
    for (size_t ii = 0; ii < MAX_ID; ii++, mask <<= 1)
    {
        if (!(gBitmapId & mask))
        {
            gBitmapId |= mask;
            return ii;
        }
    }

    return -1;
}

static void set_id(int id)
{
    uint32_t mask = (0x01 << id);
    gBitmapId |= mask;
}

static void release_id(int id)
{
    uint32_t mask = (0x01 << id);
    gBitmapId &= (~mask);
}

static bool is_scsi_disk(const char* name)
{
    // sd[a~z][1~9]
    if(strlen(name) < 3 || strncmp(name, "sd", 2) || !isalpha(*(name + 2)))
        return false;

    if (strlen(name) > 4 && !isdigit(*(name+3)))
        return false;

    return true;
}

static bool get_fstype(const char* name, char* fstype)
{
    bool ret = false;

    char line[1024];
    sprintf(line, "lsblk -o FSTYPE /dev/%s", name);

    FILE* fp = popen(line, "r");
    if (!fp)
        return ret;

    int index = 0;
    while(fgets(line, sizeof(line), fp))
    {
        if (index == 1)
        {
            strcpy(fstype, trim(line));
            ret = true;
        }

       index++;
    }

    pclose(fp);

    return ret;
}

#include <sys/stat.h>

static bool is_exist_dir(const char* path)
{
    struct stat sb;
    if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode))
        return true;

    return false;
}

#include <sys/mount.h>
static bool mount_device(const char* dev_name, const char* fstype, const char* mount_point)
{
    char dev[64];

    if (!is_exist_dir(mount_point))
    {
        if (mkdir(mount_point, 0777))
        {
            LOGE("Failed to create directory : %s", mount_point);
            return false;
        }
    }

    sprintf(dev, "/dev/%s", dev_name);
    //if (mount(dev, mount_point, fstype, MS_MGC_VAL | MS_RDONLY | MS_NOSUID, ""))
    if (mount(dev, mount_point, fstype, MS_MGC_VAL | MS_NOSUID, ""))
    {
        LOGE("Mount %s to %s failed. [%s]", dev, mount_point, strerror(errno));
        rmdir(mount_point);
        return false;
    }

    LOGD("mount %s -> %s", dev, mount_point);
    return true;
}

static void umount_device(const char* mount_point)
{
    if (is_exist_dir(mount_point))
    {
        if (umount(mount_point) != 0)
        {
            LOGE("Failed to umount %s : %d(%s)", mount_point, errno, strerror(errno));
        }
        rmdir(mount_point);
        LOGD("umount %s", mount_point);
    }
}

static void umount_all()
{
    char line[2*1023];
    FILE* fp = popen("mount", "r");
    if (!fp)
        return;

    int prefixLen = strlen(MOUNT_PREFIX);
    while(fgets(line, sizeof(line), fp))
    {
        char* p = strstr(line, "on");
        p = ltrim(p + 2);
        if (strncmp(p, MOUNT_PREFIX, prefixLen) == 0)
        {
            char* e = strchr(p, ' ');
            *e = 0;
            umount_device(p);
        }
    }

    pclose(fp);
}

static int get_usb_disk_list(std::vector<std::string>& devList)
{
    char line[1024];
    FILE* fp = fopen("/proc/partitions", "r");

    if (!fp)
        return 0;

    while(fgets(line ,sizeof(line), fp))
    {
        int major, minor, block;
        char name[1024];
        if (sscanf(line, "%d %d %d %s", &major, &minor, &block, name) == 4)
        {
            if (is_scsi_disk(name))
                devList.push_back(name);
        }
    }

    fclose(fp);

    return devList.size();
}

static const char* get_mount_dir(const char* dev, char* mount_dir)
{
    char line[1024];
    char path[1024];

    FILE* fp = fopen("/proc/mounts", "r");

    if (!fp)
        return 0;

    sprintf(path, "/dev/%s", dev);
    while(fgets(line ,sizeof(line), fp))
    {
        char _path[1024];
        char _mountDir[1024];
        if (sscanf(line, "%s %s", _path, _mountDir) == 2)
        {
            if(strcmp(path, _path) == 0)
            {
                strcpy(mount_dir, _mountDir);
                fclose(fp);
                return mount_dir;
            }
        }
    }

    fclose(fp);

    return NULL;
}

UsbStorageManager& UsbStorageManager::getInstance()
{
    static UsbStorageManager _instance;

    return _instance;
}

UsbStorageManager::UsbStorageManager()
                  : Task("UsbStorageManager")
{
    start();
}

UsbStorageManager::~UsbStorageManager()
{
    stop();
}

bool UsbStorageManager::addListener(IUsbStorageListener* listener)
{
    Lock lock(mLock);

    if(!listener)
        return false;

    std::list<IUsbStorageListener*>::iterator it = std::find(mListeners.begin(), mListeners.end(), listener);
    if (it != mListeners.end())
    {
        LOGE("IUsbStorageListener is alreay exsit !!");
        return false;
    }

    mListeners.push_back(listener);

    return true;
}

void UsbStorageManager::removeListener(IUsbStorageListener* listener)
{
    Lock lock(mLock);

    if(!listener)
        return;

    for(std::list<IUsbStorageListener*>::iterator it = mListeners.begin(); it != mListeners.end(); it++)
    {
        if (listener == *it)
        {
            mListeners.erase(it);
            return;
        }
    }
}

std::vector<std::string> UsbStorageManager::getMountPoints()
{
    Lock lock(mLock);

    std::vector<std::string> array;

    std::map<std::string, int>::iterator it;
    for (it = mMountPoints.begin(); it != mMountPoints.end(); it++)
    {
        char mountPoint[2*1024];
        sprintf(mountPoint, "%s%d", MOUNT_PREFIX, it->second);
        array.push_back(mountPoint);
    }

    return array;
}

bool UsbStorageManager::onPreStart()
{
    mExitTask = false;

    mMsgQ.setEOS(false);
    UsbHotplugManager::getInstance().addListener(this);

    std::vector<std::string> vt;
    int cnt = get_usb_disk_list(vt);
    char mountDir[1024];

    for (int ii = 0; ii < cnt; ii++)
    {
        const char* name = vt[ii].c_str();
        if (get_mount_dir(name, mountDir))
        {
            int id = 0;
            if (sscanf(mountDir, "/mnt/usb%d", &id) == 1)
            {
                LOGW("Device is already mounted %s -> %s", name, mountDir);
                set_id(id);
                mMountPoints[name] = id;
                continue;
            }
            else
                umount_device(mountDir);
        }

        UsbMessage msg(MSG_ID_USB_ADD, strdup(name));
        mMsgQ.put(msg);
    }

    return true;
}

void UsbStorageManager::onPreStop()
{
    mExitTask = true;
    mMsgQ.setEOS(true);
    UsbHotplugManager::getInstance().removeListener(this);
}

void UsbStorageManager::run()
{
    UsbMessage msg;

    while(!mExitTask)
    {
        if (mMsgQ.get(&msg))
        {
            char* name = (char*)msg.mParam;

            if (msg.mId == MSG_ID_USB_ADD)
            {
                if (mMountPoints.find(name) == mMountPoints.end())
                {
                    int id = alloc_id();
                    if (id < 0)
                        LOGE("MOUNT is FULL");
                    else
                    {
                        char fstype[64];
                        if (get_fstype(name, fstype))
                        {
                            char mount_point[64];
                            sprintf(mount_point, "/mnt/usb%d", id);
                            if (mount_device(name, fstype, mount_point))
                            {
                                Lock lock(mLock);
                                mMountPoints[name] = id;
                                for(std::list<IUsbStorageListener*>::iterator it = mListeners.begin(); it != mListeners.end(); it++)
                                    (*it)->onUsbStorageAdded(name, mount_point);
                            }
                            else
                                release_id(id);
                        }
                    }
                }
            }
            else if (msg.mId == MSG_ID_USB_REMOVE)
            {
                if (mMountPoints.find(name) != mMountPoints.end())
                {
                    char mount_point[64];
                    sprintf(mount_point, "/mnt/usb%d", mMountPoints[name]);
                    umount_device(mount_point);
                    release_id(mMountPoints[name]);

                    Lock lock(mLock);
                    mMountPoints.erase(name);
                    for(std::list<IUsbStorageListener*>::iterator it = mListeners.begin(); it != mListeners.end(); it++)
                        (*it)->onUsbStorageRemoved(name, mount_point);
                }
            }
            SAFE_FREE(name);
        }
    }

    mMsgQ.flush();
}

void UsbStorageManager::onHotplugChanged(UsbHotplugEvent_e event, const char* devPath)
{
    const char* p = strrchr(devPath, '/');
    if (!p)
        return;

    p++;
    if (is_scsi_disk(p))
    {
        if (event == USB_HOTPLUG_EVENT_ADD)
        {
            LOGI("plugged : %s", p);
            UsbMessage msg(MSG_ID_USB_ADD, strdup(p));
            mMsgQ.put(msg);
        }
        else if(event == USB_HOTPLUG_EVENT_REMOVE)
        {
            LOGI("Removed : %s", p);
            UsbMessage msg(MSG_ID_USB_REMOVE, strdup(p));
            mMsgQ.put(msg);
        }
    }
}
