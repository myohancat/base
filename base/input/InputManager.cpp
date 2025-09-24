/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "InputManager.h"

#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <linux/uinput.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "Log.h"

#define DEVICE_PATH "/dev/input"

#define EVENT_ID_KEY   0
#define EVENT_ID_MOUSE 1
static bool _is_input_device(const char* path)
{
    struct stat st;

    if (strstr(path, "mouse") || strstr(path, "mice"))
        return false;

    lstat(path, &st);

    if ((st.st_mode & S_IFMT) != S_IFCHR)
        return false;

    return true;
}

InputDevice::InputDevice(const std::string& path)
            : mFD(-1),
              mPath(path),
              mName("Unknown"),
              mVersion(-1),
              mVendor(-1),
              mProduct(-1),
              mId(-1)
{
    mFD = open(path.c_str(), O_RDWR);
    if(mFD < 0)
    {
        LOGE("cannot open file ! %s - %s", path.c_str(), strerror(errno));
        return;
    }
    ioctl(mFD, EVIOCGRAB, 1);

    loadAttrib();
    LOGI("InputDevice [%s] ver : %d, vendor : %d, product : %d", mName.c_str(), mVersion, mVendor, mProduct);
}

InputDevice::~InputDevice()
{
    if(mFD >= 0)
        close(mFD);
}

int InputDevice::getFD()
{
    return mFD;
}

const char* InputDevice::getPath()
{
    return mPath.c_str();
}

const char* InputDevice::getName()
{
    return mName.c_str();
}

int InputDevice::getVendor()
{
    return mVendor;
}

int InputDevice::getProduct()
{
    return mProduct;
}

int InputDevice::getId()
{
    return mId;
}

bool InputDevice::operator==(const std::string& path) const
{
    return (mPath == path);
}

void InputDevice::loadAttrib()
{
    char buffer[80];
    struct input_id inputId;

    if (ioctl(mFD, EVIOCGNAME(sizeof(buffer) - 1), &buffer) < 1)
    {
        LOGE("Cannot getValue Input Device Name");
        mName = "Unknown";
        return;
    }
    else
    {
        buffer[sizeof(buffer) - 1] = '\0';
        mName = buffer;
    }

    if(ioctl(mFD, EVIOCGID, &inputId))
    {
        LOGE("Cannot getValue Input Device ID");
        return;
    }
    else
    {
        mVendor  = inputId.vendor;
        mProduct = inputId.product;
        mVersion = inputId.version;
    }

    mId = HAL_Input_GetDeviceID(mVersion, mVendor, mProduct, mName.c_str());
}

InputManager& InputManager::getInstance()
{
    static InputManager instance;

    return instance;
}

InputManager::InputManager()
             : Task(90, "InputManager"),
               mExitProc(false)
{
    mFD = inotify_init();
    mWD = inotify_add_watch(mFD, DEVICE_PATH, IN_CREATE | IN_DELETE);

#ifdef ENABLE_VIRUTAL_REPEAT_KEY
    mLastKeyCode = -1;
    mRepeatTimer.setHandler(this);
#endif

    mEventQ.setHandler(this);

    scanInputDevice();

    start();
}

InputManager::~InputManager()
{
    mExitProc = true;
    stop();

    if(mWD >= 0)
        close(mWD);
    if(mFD >= 0)
        close(mFD);
}

void InputManager::dispatchKeyEvent(int keycode, int state)
{
    KeyEvent event(keycode, state);
    mEventQ.sendEvent(EVENT_ID_KEY, &event, sizeof(event));
}

void InputManager::dispatchMouseEvent(int dx, int dy)
{
#if 0
    MouseEvent event(dx, dy);
    mEventQ.sendEvent(EVENT_ID_MOUSE, &event, sizeof(event));
#else
    mMouseListeners.forEach([dx, dy](IMouseListener* listener) {
        return listener->onMouseReceived(dx, dy);
    });
#endif
}

#ifdef ENABLE_VIRUTAL_REPEAT_KEY
void InputManager::onTimerExpired(const ITimer* timer)
{
    UNUSED(timer);

    if (mLastKeyCode == -1)
        return;

    mRepeatTimer.setInterval(100);

    /* Generate Repeat Key */
    dispatchKeyEvent(mLastKeyCode, KEY_REPEATED);
}
#endif

void InputManager::onEventReceived(int id, void* data, int dataLen)
{
    if (!data || dataLen == 0)
        return;

    if (id == EVENT_ID_KEY)
    {
        KeyEvent* event = (KeyEvent*)data;

        //LOGT("--- [KEY] code : %d, state : %s", event->mKeyCode, (event->mKeyState == KEY_RELEASED)?"RELEASED":(event->mKeyState == KEY_PRESSED)?"PRESSED":"REPEATED");
        mKeyListeners.forEach([event](IKeyListener* listener) {
            return listener->onKeyReceived(event->mKeyCode, event->mKeyState);
        });
    }
    else if (id == EVENT_ID_MOUSE)
    {
        MouseEvent* event = (MouseEvent*)data;

        //LOGT("--- [MOUSE] dx : %d, dy : %d", event->mDX, event->mDY);
        mMouseListeners.forEach([event](IMouseListener* listener) {
            return listener->onMouseReceived(event->mDX, event->mDY);
        });
    }
}

void InputManager::addKeyListener(IKeyListener* listener, int priority)
{
    mKeyListeners.addListener(listener, priority);
}

void InputManager::removeKeyListener(IKeyListener* listener)
{
    mKeyListeners.removeListener(listener);
}

void InputManager::addMouseListener(IMouseListener* listener, int priority)
{
    mMouseListeners.addListener(listener, priority);
}

void InputManager::removeMouseListener(IMouseListener* listener)
{
    mMouseListeners.removeListener(listener);
}

void InputManager::addTouchListener(ITouchListener* listener, int priority)
{
    mTouchListeners.addListener(listener, priority);
}

void InputManager::removeTouchListener(ITouchListener* listener)
{
    mTouchListeners.removeListener(listener);
}

void InputManager::setRawKeyListener(IRawKeyListener* listener)
{
    Lock lock(mRawKeyListenerLock);

    mRawKeyListener = listener;
}

void InputManager::addInputDevice(const std::string& devpath)
{
    for(InputDeviceList::iterator it = mInputDevices.begin(); it != mInputDevices.end(); it++)
    {
        InputDevice* device = *it;
        if(*device == devpath)
        {
            LOGE("device is alreay exsit : %s !!", devpath.c_str());
            return;
        }
    }

    InputDevice* dev = new InputDevice(devpath);
    if (dev->getFD() == -1)
    {
        LOGE("fd is invalid, skip add input device");
        delete dev;
        return;
    }

    LOGD("---> add device : %s", devpath.c_str());
    mInputDevices.push_back(dev);
}

void InputManager::removeInputDevice(const std::string& devpath)
{
    for(InputDeviceList::iterator it = mInputDevices.begin(); it != mInputDevices.end(); it++)
    {
        InputDevice* device = *it;
        if(*device == devpath)
        {
            LOGD("---> remove device : %s", devpath.c_str());
            mInputDevices.erase(it);
            return;
        }
    }
}

void InputManager::scanInputDevice()
{
    char devname[PATH_MAX];
    DIR* dir;
    struct dirent* de;

    dir = opendir(DEVICE_PATH);
    if(!dir)
    {
        LOGE("opendir failed : %s", DEVICE_PATH);
        return;
    }

    while((de = readdir(dir)))
    {
        if(de->d_name[0] == '.')
        {
            if(de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0'))
                continue;
        }

        sprintf(devname, "%s/%s", DEVICE_PATH, de->d_name);

        if (!_is_input_device(devname))
            continue;

        addInputDevice(devname);
    }

    closedir(dir);
}

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * (EVENT_SIZE + 16) )
void InputManager::run()
{
    static int _dx, _dy = 0;

    fd_set sReadFds;
    struct timeval sWait;
    int    nLastFD = 0;
    int    nCnt    = 0;

    while(!mExitProc)
    {
        FD_ZERO(&sReadFds);

        FD_SET(mFD, &sReadFds); // For Watch device [ add | delete ]
        nLastFD = mFD;

        for(InputDeviceList::iterator it = mInputDevices.begin(); it != mInputDevices.end(); it++)
        {
            int devFD = (*it)->getFD();
            FD_SET(devFD, &sReadFds);
            nLastFD = MAX(nLastFD, devFD);
        }
        nLastFD++;

        sWait.tv_sec = 0;
        sWait.tv_usec = 100 * 1000;

        nCnt = select(nLastFD, &sReadFds, NULL, NULL, &sWait);
        if(nCnt < 0)
        {
            if(nCnt == -1 && errno != EINTR)
            {
                LOGE("select error oucced!!! errno=%d", errno);
                break;
            }

            continue; /* TIMEOUT */
        }

        if(FD_ISSET(mFD, &sReadFds))
        {
            char buffer[BUF_LEN];
            int ii = 0;
            int len = read(mFD, buffer, BUF_LEN);

            while( ii < len)
            {
                struct inotify_event *event = (struct inotify_event *)&buffer[ii];
                char devpath[PATH_MAX];

                if(event->len)
                {
                    if(event->mask & IN_CREATE)
                    {
                        sprintf(devpath, "%s/%s", DEVICE_PATH, event->name);

                        if (_is_input_device(devpath))
                            addInputDevice(devpath);
                    }
                    else if(event->mask & IN_DELETE)
                    {
                        sprintf(devpath, "%s/%s", DEVICE_PATH, event->name);
                        removeInputDevice(devpath);
                    }
                }

                ii += EVENT_SIZE + event->len;
            }

            continue;
        }

        for(InputDeviceList::iterator it = mInputDevices.begin(); it != mInputDevices.end(); it++)
        {
            struct input_event event;
            int res = 0;

            int devFD = (*it)->getFD();
            if(FD_ISSET(devFD, &sReadFds))
            {
                res = read(devFD, &event, sizeof(event));
                if(res >= (int)sizeof(event))
                {
                    if(event.type == EV_KEY)
                    {
                        if(event.value == 0
                           || event.value == 1
                           || event.value == 2
                        )
                        {
                            int deviceId = (*it)->getId();
                            int keycode  = HAL_Input_GetKeyCode(deviceId, event.code);
                            int state    = event.value;

                            LOGT("--- [RAW_KEY] deviceid : %d, code : %d, state : %d", deviceId, event.code, event.value);
                            mRawKeyListenerLock.lock();
                            if (mRawKeyListener)
                                mRawKeyListener->onRawKeyReceived(deviceId, event.code, event.value);
                            mRawKeyListenerLock.unlock();

#ifdef ENABLE_VIRUTAL_REPEAT_KEY
                            if (state == KEY_PRESSED)
                            {
                                mRepeatTimer.start(900, true);
                                mLastKeyCode = keycode;
                            }
                            else if (state == KEY_RELEASED)
                            {
                                mRepeatTimer.stop();
                                mLastKeyCode = -1;
                            }
#endif
                            dispatchKeyEvent(keycode, state);
                        }
                    }
                    else if (event.type == EV_REL)
                    {
                        if (event.code == REL_X)
                            _dx += event.value;
                        else if (event.code == REL_Y)
                            _dy += event.value;
                    }
                    else if (event.type == EV_SYN)
                    {
                        if (event.code == SYN_REPORT)
                        {
                            if (_dx || _dy)
                            {
                                dispatchMouseEvent(_dx, _dy);
                                _dx = _dy = 0;
                            }
                        }
                    }
                    else if (event.type == EV_ABS)
                    {
                        if (event.code == ABS_MT_TRACKING_ID) // 57
                        {
                            if (event.value != -1)
                            {
                                // TOUCH START
                                LOGD("TOUCH START");
                            }
                            else
                            {
                                // TOUCH END
                                LOGD("TOUCH END");
                            }
                        }
                        else if (event.code == ABS_MT_SLOT) // 47
                        {
                            // event.value : 0, 1, ... multitouch
                        }
                        else if (event.code == ABS_MT_POSITION_X) // 53
                        {
                            //
                        }
                        else if (event.code == ABS_MT_POSITION_Y) // 54
                        {
                            //
                        }
                        else if (event.code == ABS_X) // 0
                        {
                            LOGD("X : %d", event.value);
                        }
                        else if (event.code == ABS_Y) // 1)
                        {
                            LOGD("Y : %d", event.value);
                        }
                    }
                }
            }
        }
    }
}
