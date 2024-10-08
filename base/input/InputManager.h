/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __INPUT_MANAGER_H_
#define __INPUT_MANAGER_H_

#include "Keycode.h"
#include "Types.h"
#include "Task.h"
#include "Mutex.h"
#include "EventQueue.h"
#include "TimerTask.h"

#include "KeyListener.h"
#include "TouchListener.h"

#include <string>
#include <list>

//////////////// HAL ///////////////////
extern "C" int HAL_Input_GetDeviceID(int version, int vendor, int product, const char* name);
extern "C" int HAL_Input_GetKeyCode(int deviceId, int rawKeyCode);
////////////////////////////////////////

//#define ENABLE_VIRUTAL_REPEAT_KEY

class InputDevice
{
public:
    InputDevice(const std::string& path);
    ~InputDevice();

    int getFD();

    const char*   getPath();
    const char*   getName();
    int           getVendor();
    int           getProduct();

    int           getId();

    bool operator==(const std::string& path) const;

private:
    int mFD;

    std::string   mPath;
    std::string   mName;
    int           mVersion;
    int           mVendor;
    int           mProduct;
    int           mId;

private:
    void loadAttrib();
};

class InputManager : public Task, IEventHandler
#ifdef ENABLE_VIRUTAL_REPEAT_KEY
                          , ITimerHandler
#endif
{
public:
    struct KeyEvent
    {
        int mKeyCode;
        int mKeyState;

        KeyEvent(int keyCode, int keyState)
        {
            mKeyCode = keyCode;
            mKeyState = keyState;
        }
    };

    static InputManager& getInstance();

    void addKeyListener(IKeyListener* listener);
    void removeKeyListener(IKeyListener* listener);

    void addTouchListener(ITouchListener* listener);
    void removeTouchListener(ITouchListener* listener);

    void setRawKeyListener(IRawKeyListener* listener);

    void dispatchKeyEvent(int keycode, int state);

private:
    InputManager();
    ~InputManager();

    int mFD; /* inotify_init() */
    int mWD; /* inotify_add_watch() */

    EventQueue mEventQ;

    bool      mExitProc;

#ifdef ENABLE_VIRUTAL_REPEAT_KEY
    int       mLastKeyCode;
    TimerTask mRepeatTimer;
#endif

    RecursiveMutex mKeyListenerLock;
    typedef std::list<IKeyListener*> KeyListenerList;
    KeyListenerList mKeyListeners;

    RecursiveMutex mTouchListenerLock;
    typedef std::list<ITouchListener*> TouchListenerList;
    TouchListenerList mTouchListeners;

    RecursiveMutex mRawKeyListenerLock;
    IRawKeyListener* mRawKeyListener;

    /* Input Device */
    typedef std::list<InputDevice*> InputDeviceList;
    InputDeviceList     mInputDevices;

private:
    void onEventReceived(int id, void* data, int dataLen);

#ifdef ENABLE_VIRUTAL_REPEAT_KEY
    void onTimerExpired(const ITimer* timer);
#endif

    void scanInputDevice();
    void addInputDevice(const std::string& devpath);
    void removeInputDevice(const std::string& devpath);

    bool startKeyEventTask();
    bool stopKeyEventTask();

    void run();
};

#endif /* __INPUT_MANAGER_H_ */
