/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "PointerTracker.h"

#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080

#define MIN_SENSITIVITY     0.5f
#define MAX_SENSITIVITY     5.0f
#define DEF_SENSITIVITY     1.5f

#define GUARD 3

PointerTracker& PointerTracker::getInstance()
{
    static PointerTracker _instance;

    return _instance;
}

PointerTracker::PointerTracker()
{
    mX = SCREEN_WIDTH / 2;
    mY = SCREEN_HEIGHT / 2;

    mSensitivity = DEF_SENSITIVITY;

    InputManager::getInstance().addMouseListener(this);
}

PointerTracker::~PointerTracker()
{
    InputManager::getInstance().removeMouseListener(this);
}

void PointerTracker::setSensitivity(float sensitivity)
{
    if (sensitivity < MIN_SENSITIVITY)
        sensitivity = MIN_SENSITIVITY;
    if (sensitivity > MAX_SENSITIVITY)
        sensitivity = MAX_SENSITIVITY;

    mSensitivity = sensitivity;
}

void PointerTracker::addListener(IPointerListener* listener)
{
    Lock lock(mLock);

    for (auto* l : mListeners)
    {
        if (l == listener)
            return;
    }

    mListeners.push_back(listener);
}

void PointerTracker::removeListener(IPointerListener* listener)
{
    Lock lock(mLock);

    mListeners.remove(listener);
}

bool PointerTracker::onMouseReceived(int dx, int dy)
{
    int newX = mX + (dx * mSensitivity);
    int newY = mY + (dy * mSensitivity);

    if (newX < 0)
        newX = 0;
    else if (newX > SCREEN_WIDTH - GUARD)
        newX = SCREEN_WIDTH - GUARD;

    if (newY < 0)
        newY = 0;
    else if (newY > SCREEN_HEIGHT - GUARD)
        newY = SCREEN_HEIGHT - GUARD;

    if (newX != mX || newY != mY)
    {
        for (auto* listener : mListeners)
        {
            if (listener->onPointerMoved(mX, mY))
                return false;
        }

        mX = newX;
        mY = newY;
    }

    return false;
}
