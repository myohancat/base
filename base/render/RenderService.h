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
#include "Queue.h"
#include "TimerTask.h"
#include "Platform.h"
#include "DisplayHotplugManager.h"

struct Msg
{
    int   what;
    int   arg;
    void* obj;

    Msg() : what(0) { }
    Msg(int _what) : what(_what) { }
    Msg(int _what, int _arg) : what(_what), arg(_arg) { }
    Msg(int _what, void* _obj) : what(_what), obj(_obj) { }
    Msg(int _what, int _arg, void* _obj) : what(_what), arg(_arg), obj(_obj) { }
};

class IRenderable
{
public:
    virtual ~IRenderable() { }

    virtual int  getZOrder() { return 0; };
    virtual void onSurfaceCreated(int screenWidth, int screenHeight) = 0;
    virtual bool isNeedToDraw() = 0;
    virtual void onDrawFrame() = 0;
    virtual void onSurfaceRemoved() = 0;

public:
    static int compare(IRenderable* a, IRenderable* b)
    {
        return a->getZOrder() < b->getZOrder();
    }
};

class IRenderObserver
{
public:
    virtual ~IRenderObserver() { }

    virtual void onRenderCompleted(GLuint texture) = 0;
};

#define OFFSCREEN_WIDTH  1920
#define OFFSCREEN_HEIGHT 1080

typedef enum
{
    RENDER_MODE_CONTINUOUSLY,
    RENDER_MODE_WHEN_DIRTY,

    MAX_RENDER_MODE
} RenderMode_e;

class RenderService : public Task, ITimerHandler, IDiplayHotplugListener
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

    void setRenderMode(RenderMode_e eMode);
    void sortRenderer();

    EGLDisplay getDisplay();
    EGLContext getContext();
    int  getScreenWidth() const;
    int  getScreenHeight() const;

    void update(bool forceToDraw = false);

private:
    RenderService();

    bool onPreStart();
    void onPreStop();
    void onPostStop();
    void run();

    void onTimerExpired(const ITimer* timer);

    void onDisplayPlugged();
    void onDisplayRemoved();

    bool initEGL();
    void deinitEGL();

private:
    IPlatform*   mPlatform;

    EGLDisplay   mDisplay;
    EGLContext   mContext;
    EGLSurface   mSurface;

    GLuint       mProgram;
    GLuint       mAttribPos;
    GLuint       mAttribTex;

    int          mRealScreenWidth;
    int          mRealScreenHeight;

    Queue<Msg, 64> mMsgQ;

    Mutex mRendererLock;
    typedef std::list<IRenderable*> RendererList;
    RendererList mRenderers;

    typedef std::list<IRenderObserver*> ObserverList;
    ObserverList mObservers;

    bool         mExitTask = false;
    TimerTask    mTimer;
    RenderMode_e mRenderMode = RENDER_MODE_CONTINUOUSLY;

    bool         mDisplayON = false;
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

#endif /* __RENDER_SERVICE_H_ */
