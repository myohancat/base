/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "RenderService.h"

#include "MainLoop.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/eglext.h>
#include <algorithm>
#include <unistd.h>
#include "ShaderUtil.h"
#include "SysTime.h"
#include "Log.h"

#define DELAY_FOR_30_FPS         17 /* 1000ms/30 */

#define MSG_ID_ADD_RENDERER      1
#define MSG_ID_REMOVE_RENDERER   2
#define MSG_ID_UPDATE            3

RenderService& RenderService::getInstance()
{
    static RenderService _instance;

    return _instance;
}

RenderService::RenderService()
              : Task(80, "RenderService")
{
    mTimer.setHandler(this);
    DisplayHotplugManager::getInstance().addListener(this);

    MainLoop::getInstance().post([this] {
        if (DisplayHotplugManager::getInstance().isPlugged())
            onDisplayPlugged();
    });
}

RenderService::~RenderService()
{
__TRACE__
    stop();

    DisplayHotplugManager::getInstance().removeListener(this);
    mTimer.setHandler(NULL);
}

void RenderService::start()
{
    Task::start();
}

void RenderService::stop()
{
    Task::stop();
}

bool RenderService::onPreStart()
{
    mExitTask = false;
    mMsgQ.setEOS(false);

    if (!initEGL())
        return false;

    mRendererLock.lock();
    if (mRenderMode == RENDER_MODE_CONTINUOUSLY)
        mTimer.start(DELAY_FOR_30_FPS, true);
    mRendererLock.unlock();

    return true;
}

void RenderService::onPreStop()
{
__TRACE__
    mExitTask = true;
    mTimer.stop();
    mMsgQ.setEOS(true);
}

void RenderService::onPostStop()
{
__TRACE__
    deinitEGL();
}

#include "EGLHelper.h"
#include "FrameBuffer.h"

#define MAX_FBO     3
void RenderService::run()
{
__TRACE__
    if (!eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, mContext))
    {
        LOGE("Could not make the current window current !");
        return;
    }

    FrameBuffer* fbos[MAX_FBO];

    CHECK("RenderService : Create FBO %dx%d", OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
    for (int ii = 0; ii < MAX_FBO; ii++)
        fbos[ii] = new FrameBuffer(DRM_FORMAT_RGB888, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);

    mRendererLock.lock();
    for (RendererList::iterator it = mRenderers.begin(); it != mRenderers.end(); it++)
        (*it)->onSurfaceCreated(OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
    mRendererLock.unlock();

    int index = -1;
    Msg msg;
    while(!mExitTask)
    {
        if (!mMsgQ.get(&msg, -1))
            continue;

        switch(msg.what)
        {
            case MSG_ID_ADD_RENDERER:
            {
                IRenderable* renderer = (IRenderable*)msg.obj;
                renderer->onSurfaceCreated(OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
                update(true);
                break;
            }
            case MSG_ID_REMOVE_RENDERER:
            {
                IRenderable* renderer = (IRenderable*)msg.obj;
                renderer->onSurfaceRemoved();
                update(true);
                break;
            }
            case MSG_ID_UPDATE:
            {
                if (mRenderMode != RENDER_MODE_CONTINUOUSLY)
                {
                    Lock lock(mRendererLock);

                    bool forceToDraw = msg.arg;
                    bool isNeedToDraw = false;

                    for(RendererList::iterator it = mRenderers.begin(); it != mRenderers.end(); it++)
                    {
                        if ((*it)->isNeedToDraw())
                        {
                            isNeedToDraw = true;
                            break;
                        }
                    }

                    if (!forceToDraw && !isNeedToDraw)
                        continue;
                }

                index = (index + 1) % MAX_FBO;

                fbos[index]->bind();

                glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);

                glEnable(GL_BLEND);
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_CULL_FACE);
                glDisable(GL_DITHER);

                glClearColor(0, 0, 0, 0);  // TransParent
                glClear(GL_COLOR_BUFFER_BIT);

                mRendererLock.lock();
                for(RendererList::iterator it = mRenderers.begin(); it != mRenderers.end(); it++)
                    (*it)->onDrawFrame();
                mRendererLock.unlock();

                glFinish();

                if (fbos[index]->dmabuf() != -1)
                {
                    EGLHelper::dma_buf_sync(fbos[index]->dmabuf());

                    Lock lock(mRendererLock);

                    if (mOnDisplayRenderer)
                        mOnDisplayRenderer->update(fbos[index]->dmabuf(), DRM_FORMAT_RGB888, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);

                    for(ObserverList::iterator it = mObservers.begin(); it != mObservers.end(); it++)
                        (*it)->onRenderCompleted(fbos[index]->dmabuf(), DRM_FORMAT_RGB888, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
                }
                break;
            }
        }
    }

    mRendererLock.lock();
    for(RendererList::iterator it = mRenderers.begin(); it != mRenderers.end(); it++)
        (*it)->onSurfaceRemoved();
    mRendererLock.unlock();

    for (int ii = 0; ii < MAX_FBO; ii++)
    {
        SAFE_DELETE(fbos[ii]);
    }
}

#include <EGL/eglext.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

static EGLDisplay _getDisplay()
{
    int rfd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (rfd < 0)
    {
        LOGE("rfd cannot open");
        return NULL;
    }

#if 1 // W/A Code
    drmModeRes* res = drmModeGetResources(rfd);
    for (int i = 0; i < res->count_connectors; i++)
    {
        drmModeConnector* connector = drmModeGetConnector(rfd, res->connectors[i]);
        CHECK("connector : %p, connector->connection : %d", connector, connector->connection);
        drmModeFreeConnector(connector);
    }
#endif

    gbm_device* gbm = gbm_create_device(rfd); // TODO MUST DESTROYED
    if (!gbm)
    {
        LOGE("gbm is NULL");
        return NULL;
    }

    PFNEGLGETPLATFORMDISPLAYEXTPROC gpd = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    EGLDisplay dpy = gpd(EGL_PLATFORM_GBM_KHR, gbm, NULL);

    return dpy;
}

bool RenderService::initEGL()
{
    EGLint    numConfigs;
    EGLint    majorVersion;
    EGLint    minorVersion;
    EGLConfig config;

    mDisplay = _getDisplay();
    if (mDisplay == EGL_NO_DISPLAY)
    {
        LOGE("failed to get EGLDisplay ...");
        return false;
    }

    if (!eglInitialize(mDisplay, &majorVersion, &minorVersion))
    {
        LOGE("failed to EGL Initialize ...");
        return false;
    }

    LOGD("init success");
    if (!eglBindAPI(EGL_OPENGL_ES_API))
    {
        LOGE("eglBindAPI GLES failed (0x%x)", eglGetError());
        return false;
    }

    const  EGLint attribs[] =
    {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      0,
        EGL_NONE
    };

    if ((eglGetConfigs(mDisplay, NULL, 0, &numConfigs) != EGL_TRUE) || (numConfigs == 0))
    {
        LOGE("failed to get configuration ...");
        return false;
    }

    if ((eglChooseConfig(mDisplay, attribs, &config, 1, &numConfigs) != EGL_TRUE) || (numConfigs != 1))
    {
        LOGE("failed to choose configuration ...");
        return false;
    }

    const  EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE, EGL_NONE };
    mContext = eglCreateContext(mDisplay, config, NULL, contextAttribs);
    if (mContext == EGL_NO_CONTEXT)
    {
        LOGE("failed to create EGLContext");
        return false;
    }

    return true;
}


void RenderService::addRenderer(IRenderable* renderer)
{
__TRACE__
    Lock lock(mRendererLock);

    if(!renderer)
        return;

    RendererList::iterator it = std::find(mRenderers.begin(), mRenderers.end(), renderer);
    if(it != mRenderers.end())
        return;

    mRenderers.push_back(renderer);
    mRenderers.sort(IRenderable::compare);
    if (state() == TASK_STATE_RUNNING)
    {
        Msg msg(MSG_ID_ADD_RENDERER, renderer);
        mMsgQ.put(msg, -1);
    }
}

void RenderService::removeRenderer(IRenderable* renderer)
{
    Lock lock(mRendererLock);
    if(!renderer)
        return;

    for(RendererList::iterator it = mRenderers.begin(); it != mRenderers.end(); it++)
    {
        if(renderer == *it)
        {
            mRenderers.erase(it);
            mRenderers.sort(IRenderable::compare);
            if (state() == TASK_STATE_RUNNING)
            {
                Msg msg(MSG_ID_REMOVE_RENDERER, renderer);
                mMsgQ.put(msg, -1);
            }
            return;
        }
    }
}

void RenderService::addRenderObserver(IRenderObserver* observer)
{
    Lock lock(mRendererLock);

    if(!observer)
        return;

    ObserverList::iterator it = std::find(mObservers.begin(), mObservers.end(), observer);
    if(it != mObservers.end())
    {
        LOGW("RenderObserver is alreay exsit !!");
        return;
    }

    mObservers.push_back(observer);
}

void RenderService::removeRenderObserver(IRenderObserver* observer)
{
    Lock lock(mRendererLock);

    if(!observer)
        return;

    for(ObserverList::iterator it = mObservers.begin(); it != mObservers.end(); it++)
    {
        if(observer == *it)
        {
            mObservers.erase(it);
            return;
        }
    }
}

void RenderService::setRenderMode(RenderMode_e eMode)
{
    Lock lock(mRendererLock);

    if (mRenderMode == eMode)
        return;

    if (eMode == RENDER_MODE_CONTINUOUSLY)
        mTimer.start(DELAY_FOR_30_FPS, true);
    else
        mTimer.stop();

    mRenderMode = eMode;
}

void RenderService::sortRenderer()
{
    Lock lock(mRendererLock);
    mRenderers.sort(IRenderable::compare);
}

void RenderService::onTimerExpired(const ITimer* timer)
{
    UNUSED(timer);

    Msg msg(MSG_ID_UPDATE, false);
    mMsgQ.remove(MSG_ID_UPDATE);
    mMsgQ.put(msg);
}

void RenderService::onDisplayPlugged()
{
    Lock lock(mRendererLock);
    CHECK("@@@@@@@@@@ onDisplayPlugged()");

    SAFE_DELETE(mOnDisplayRenderer);

    mOnDisplayRenderer = new OnDisplayRenderer();
    mOnDisplayRenderer->setCpuAffinity(getCpuAffinity());
    mOnDisplayRenderer->start();
    update();
}

void RenderService::onDisplayRemoved()
{
    Lock lock(mRendererLock);
    CHECK("@@@@@@@@@@ onDisplayRemoved()");

    SAFE_DELETE(mOnDisplayRenderer);
}

void RenderService::update(bool forceToDraw)
{
    if (mRenderMode == RENDER_MODE_CONTINUOUSLY)
        return;

    Msg msg(MSG_ID_UPDATE, forceToDraw);
    mMsgQ.remove(MSG_ID_UPDATE);
    mMsgQ.put(msg);
}

void RenderService::deinitEGL()
{
    // TODO
}
