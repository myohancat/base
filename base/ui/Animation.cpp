#include "Animation.h"

#include "Window.h"

Animation::Animation(Window* window)
          : mWindow(window)
{
    mTimer.setHandler(this);
}

Animation::~Animation()
{
    mTimer.stop();
}

void Animation::start()
{
    onAnimationStart();
    mRepeatCnt = getRepeatCount();
    mTimer.start(33, true);
    if (mListener)
        mListener->onAnimationStarted();
}

void Animation::stop()
{
    mTimer.stop();
    onAnimationEnd();
    if (mListener)
        mListener->onAnimationEnded();
}

void Animation::setListener(IAnimationListener* listener)
{
    mListener = listener;
}

void Animation::onTimerExpired(UNUSED_PARAM const ITimer* timer)
{
    onAnimationRepeat();
    mRepeatCnt --;
    if (mRepeatCnt <= 0)
    {
        MainLoop::getInstance().post([this] {
                stop();
                });
    }
}


#define STEP  10
AlphaAnimation::AlphaAnimation(Window* window, float fromAlpha, float toAlpha)
              : Animation(window)
{
    mFromAlpha = fromAlpha;
    mToAlpha   = toAlpha;
}

void AlphaAnimation::onAnimationStart()
{
    if (mWindow)
    {
        mCurAlpha = mFromAlpha;
        mWindow->setAlpha(mCurAlpha);
    }
}

void AlphaAnimation::onAnimationEnd()
{
    if (mWindow)
    {
        mCurAlpha = mToAlpha;
        mWindow->setAlpha(mCurAlpha);
    }
}

void AlphaAnimation::onAnimationRepeat()
{
    float diff = (mToAlpha - mFromAlpha) / STEP;
    mCurAlpha += diff;

    if (mWindow)
    {
        mWindow->setAlpha(mCurAlpha);
        mWindow->update();
    }
}

int AlphaAnimation::getRepeatCount()
{
    return STEP;
}
