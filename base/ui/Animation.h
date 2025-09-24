/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __ANIMATION_H_
#define __ANIMATION_H_

#include "Timer.h"

class Window;

class Animation : public ITimerHandler
{
public:
    class IAnimationListener
    {
    public:
        virtual void onAnimationStarted() = 0;
        virtual void onAnimationEnded()   = 0;
    };

    Animation(Window* window);
    virtual ~Animation();

    void start();
    void stop();
    void setListener(IAnimationListener* listener);

protected:
    virtual int  getDuration() { return 33; } // 33ms
    virtual int  getRepeatCount()    = 0;
    virtual void onAnimationStart()  = 0;
    virtual void onAnimationRepeat() = 0;
    virtual void onAnimationEnd()    = 0;

protected:
    void onTimerExpired(const ITimer* timer);

protected:
    Window* mWindow    = NULL;
    int     mRepeatCnt = 0;
    Timer   mTimer;
    IAnimationListener* mListener = NULL;
};

class AlphaAnimation : public Animation
{
public:
    AlphaAnimation(Window* window, float fromAlpha, float toAlpha);

protected:
    int  getRepeatCount();

    void onAnimationStart();
    void onAnimationRepeat();
    void onAnimationEnd();

protected:
    float   mFromAlpha = 0.0f;
    float   mToAlpha   = 1.0f;
    float   mCurAlpha  = 0.0f;
};

#endif /* __ANIMATION_H_ */
