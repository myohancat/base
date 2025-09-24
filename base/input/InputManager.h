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
#include "MouseListener.h"
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

    struct MouseEvent
    {
        int mDX;
        int mDY;

        MouseEvent(int dx, int dy)
        {
            mDX = dx;
            mDY = dy;
        }
    };

    static InputManager& getInstance();

    void addKeyListener(IKeyListener* listener, int priority = 0);
    void removeKeyListener(IKeyListener* listener);

    void addMouseListener(IMouseListener* listener, int priority = 0);
    void removeMouseListener(IMouseListener* listener);

    void addTouchListener(ITouchListener* listener, int priority = 0);
    void removeTouchListener(ITouchListener* listener);

    void setRawKeyListener(IRawKeyListener* listener);

    void dispatchKeyEvent(int keycode, int state);
    void dispatchMouseEvent(int dx, int dy);

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

    template <typename TListener>
    struct ListenerEntry
    {
        TListener* mListener;
        int mPriority;
        uint32_t   mOrder;

        bool operator<(const ListenerEntry<TListener>& other) const
        {
            if (mPriority != other.mPriority)
                return mPriority > other.mPriority;

            return mOrder > other.mOrder;
        }
    };

    template <typename TListener>
    class ListenerList
    {
    public:
        void addListener(TListener* listener, int priority)
        {
            Lock lock(mLock);

            if(!listener)
                return;

            auto it = std::find_if(mListeners.begin(), mListeners.end(),
                [listener](const ListenerEntry<TListener>& entry)
                {
                    return entry.mListener == listener;
                });

            if (it != mListeners.end())
            {
                if (it->mPriority == priority)
                    return;

                it->mPriority = priority;
            }
            else
                mListeners.push_back({ listener, priority, mNextOrder++ });

            std::sort(mListeners.begin(), mListeners.end());
        }

        void removeListener(TListener* listener)
        {
            Lock lock(mLock);

            if(!listener)
                return;

            auto it = std::find_if(mListeners.begin(), mListeners.end(),
                [listener](const ListenerEntry<TListener>& entry)
                {
                    return entry.mListener == listener;
                });

            if (it != mListeners.end())
                mListeners.erase(it);
        }

        void clear()
        {
            Lock lock(mLock);
            mListeners.clear();
        }

        template <typename Func>
        void forEach(Func&& func)
        {
            Lock lock(mLock);
            for (auto& entry : mListeners)
            {
                if(func(entry.mListener))
                    break;
            }
        }

    private:
        RecursiveMutex mLock;
        std::vector<ListenerEntry<TListener>> mListeners;
        uint32_t mNextOrder = 0;
    };

    ListenerList<IKeyListener>   mKeyListeners;
    ListenerList<IMouseListener> mMouseListeners;
    ListenerList<ITouchListener> mTouchListeners;

    RecursiveMutex mRawKeyListenerLock;
    IRawKeyListener* mRawKeyListener;

    /* Input Device */
    typedef std::list<InputDevice*> InputDeviceList;
    InputDeviceList mInputDevices;

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
