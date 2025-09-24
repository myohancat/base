/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __POINTER_TRACKER_H_
#define __POINTER_TRACKER_H_

#include "InputManager.h"
#include "Mutex.h"

#include <list>

class IPointerListener
{
public:
    virtual ~IPointerListener() { }


    virtual bool onPointerMoved(int absX, int absY) = 0;
};

class PointerTracker : public IMouseListener
{
public:
    static PointerTracker& getInstance();
    virtual ~PointerTracker();

    void setSensitivity(float sensitivity);

    int getX() const;
    int getY() const;

    void addListener(IPointerListener* listener);
    void removeListener(IPointerListener* listener);

protected:
    PointerTracker();

    bool onMouseReceived(int dx, int dy) override;

private:
    int   mX;
    int   mY;
    float mSensitivity;

    Mutex mLock;
    std::list<IPointerListener*> mListeners;
};

inline int PointerTracker::getX() const
{
    return mX;
}

inline int PointerTracker::getY() const
{
    return mY;
}

#endif /* __POINTER_TRACKER_H_ */
