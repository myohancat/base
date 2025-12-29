/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __RENDER_SERVICE_H_
#define __RENDER_SERVICE_H_

#include <stdint.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GLES3/gl3.h>

#include <memory>
#include <list>

#include "Task.h"
#include "MsgQ.h"
#include "TimerTask.h"
#include "Platform.h"
#include "DisplayHotplugManager.h"
#include "OnDisplayRenderer.h"

#ifndef OFFSCREEN_WIDTH
#define OFFSCREEN_WIDTH  1920
#endif

#ifndef OFFSCREEN_HEIGHT
#define OFFSCREEN_HEIGHT 1080
#endif

class IRenderable
{
public:
    virtual ~IRenderable() { }

    virtual int  getZOrder() { return 0; }
    virtual void onSurfaceCreated(int screenWidth, int screenHeight) = 0;
    virtual bool isNeedToDraw() = 0;
    virtual void onDrawFrame() = 0;
    virtual void onSurfaceRemoved() = 0;

public:
    static bool compare(IRenderable* a, IRenderable* b)
    {
        return a->getZOrder() < b->getZOrder();
    }
};

class IRenderObserver
{
public:
    virtual ~IRenderObserver() { }

    virtual void onRenderCompleted(int dmafd, int format, int width, int height) = 0;
};

typedef enum
{
    RENDER_MODE_CONTINUOUSLY,
    RENDER_MODE_WHEN_DIRTY,

    MAX_RENDER_MODE
} RenderMode_e;

class RenderService : public Task, ITimerHandler, IDisplayHotplugListener
{
public:
    static RenderService& getInstance();
    ~RenderService();

    void start();
    void stop();

    void addRenderer(IRenderable* renderer);
    void removeRenderer(IRenderable* renderer);

    void addRenderObserver(IRenderObserver* observer);
    void removeRenderObserver(IRenderObserver* observer);

    RenderMode_e getRenderMode() const;
    void setRenderMode(RenderMode_e eMode);
    void sortRenderer();

    EGLDisplay getDisplay();
    EGLContext getContext();
    int  getScreenWidth() const;
    int  getScreenHeight() const;

    void update(bool forceToDraw = false);

private:
    RenderService();

    bool onPreStart() override;
    void onPreStop() override;
    void onPostStop() override;
    void run() override;

    void onTimerExpired(const ITimer* timer) override;

    void onDisplayPlugged() override;
    void onDisplayRemoved() override;

    bool initEGL();
    void deinitEGL();

private:
    OnDisplayRenderer* mOnDisplayRenderer = nullptr;

    EGLDisplay   mDisplay;
    EGLContext   mContext;
    EGLSurface   mSurface;

    int          mRealScreenWidth;
    int          mRealScreenHeight;

    MsgQ         mMsgQ;

    RecursiveMutex mRendererLock;
    typedef std::list<IRenderable*> RendererList;
    RendererList mRenderers;

    typedef std::list<IRenderObserver*> ObserverList;
    ObserverList mObservers;

    bool         mExitTask = false;
    TimerTask    mTimer;
    RenderMode_e mRenderMode = RENDER_MODE_CONTINUOUSLY;
};

inline EGLDisplay RenderService::getDisplay()
{
    return mDisplay;
}

inline EGLContext RenderService::getContext()
{
    return mContext;
}

inline int RenderService::getScreenWidth() const
{
    return OFFSCREEN_WIDTH;
}

inline int RenderService::getScreenHeight() const
{
    return OFFSCREEN_HEIGHT;
}

inline RenderMode_e RenderService::getRenderMode() const
{
    return mRenderMode;
}

#endif /* __RENDER_SERVICE_H_ */
