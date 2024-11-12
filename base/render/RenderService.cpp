/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "RenderService.h"

#include "MainLoop.h"

#include <GLES2/gl2.h>
#include <algorithm>
#include <unistd.h>
#include "ShaderUtil.h"
#include "SysTime.h"
#include "Log.h"

#include "WaylandPlatform.h"

#define DELAY_FOR_30_FPS         33 /* 1000ms/30 */

#define MSG_ID_ADD_RENDERER      1
#define MSG_ID_REMOVE_RENDERER   2
#define MSG_ID_UPDATE            3
#define MSG_ID_RENDER_MODE       4
#define MSG_ID_DISPLAY_ATTACHED  5
#define MSG_ID_DISPLAY_REMOVED   6

static char VERTEX_SHADER_SRC[] =
    "#version 300 es\n"
    "layout(location = 0) in vec4 a_Position;\n"
    "layout(location = 1) in vec2 a_TexCoords;\n"
    "out vec2 v_TexCoords;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = a_Position;\n"
    "    v_TexCoords = a_TexCoords;\n"
    "}";

static char FRAGMENT_SHADER_SRC[] =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec2 v_TexCoords;\n"
    "uniform sampler2D u_Texture;\n"
    "out vec4 fragColor;\n"
    "void main()\n"
    "{\n"
    "    fragColor = texture(u_Texture, vec2(v_TexCoords.x, v_TexCoords.y));\n"
    "}";

static float VERTICES[] =
{
    -1.0f, +1.0f,
    +1.0f, +1.0f,
    -1.0f, -1.0f,
    +1.0f, -1.0f,
};

static float TEX_COORDS[] =
{
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f
};

RenderService& RenderService::getInstance()
{
    static RenderService _instance;

    return _instance;
}

RenderService::RenderService()
              : Task(1, "RenderService"),
                mPlatform(NULL)
{
    mMsgQ.setEOS(true);

    mTimer.setHandler(this);
    DisplayHotplugManager::getInstance().addListener(this);
}

RenderService::~RenderService()
{
__TRACE__
    stop();
    DisplayHotplugManager::getInstance().removeListener(this);
    mTimer.setHandler(NULL);
    SAFE_DELETE(mPlatform);
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
    if (!DisplayHotplugManager::getInstance().isPlugged())
        return false;

    mExitTask = false;
    mMsgQ.setEOS(false);

    mPlatform = new WaylandPlatform();

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
    mExitTask = true;
    mTimer.stop();
    mMsgQ.setEOS(true);
}

void RenderService::onPostStop()
{
    deinitEGL();

    SAFE_DELETE(mPlatform);
}

void RenderService::run()
{
__TRACE__
    if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext))
    {
        LOGE("Could not make the current window current !");
        return;
    }

    GLuint texture;
    GLuint frameBuffer;

    mProgram = ShaderUtil::createProgram(VERTEX_SHADER_SRC, FRAGMENT_SHADER_SRC);
    mAttribPos = glGetAttribLocation(mProgram, "a_Position");
    mAttribTex = glGetAttribLocation(mProgram, "a_TexCoords");

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    mRendererLock.lock();
    for (RendererList::iterator it = mRenderers.begin(); it != mRenderers.end(); it++)
        (*it)->onSurfaceCreated(OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
    mRendererLock.unlock();

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
                bool forceToDraw = msg.arg;
                bool isNeedToDraw = false;

                mRendererLock.lock();
                for(RendererList::iterator it = mRenderers.begin(); it != mRenderers.end(); it++)
                {
                    if ((*it)->isNeedToDraw())
                    {
                        isNeedToDraw = true;
                        break;
                    }
                }
                mRendererLock.unlock();

                if (!forceToDraw && !isNeedToDraw)
                    continue;

                /* OFF SCREEN RENDERING */
                glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
                
                glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);

                glEnable(GL_BLEND);
                glDisable(GL_DEPTH_TEST);
                glClearColor(0, 0, 0, 0);  // TransParent
                glClear(GL_COLOR_BUFFER_BIT);

                mRendererLock.lock();
                for(RendererList::iterator it = mRenderers.begin(); it != mRenderers.end(); it++)
                    (*it)->onDrawFrame();
                mRendererLock.unlock();
    
                forceToDraw = false;

                /* ON SCREEN RENDERERING */
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                glViewport(0, 0, mRealScreenWidth, mRealScreenHeight);

                glDisable(GL_BLEND);
                glDisable(GL_DEPTH_TEST);

                glUseProgram(mProgram);

                glVertexAttribPointer(mAttribPos, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), VERTICES);
                glVertexAttribPointer(mAttribTex, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), TEX_COORDS);

                glEnableVertexAttribArray(mAttribPos);
                glEnableVertexAttribArray(mAttribTex);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture);

                //glDrawElements(GL_TRIANGLES, sizeof(INDICES) / sizeof(INDICES[0]), GL_UNSIGNED_SHORT, INDICES);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                //eglSwapInterval(mDisplay, 1);
                eglSwapBuffers(mDisplay, mSurface);

                //int diff = SysTime::getTickCountMs() - startTime;
                //if (diff > 10)
                //    LOGD("diff : %d", diff);

                mRendererLock.lock();
                for(ObserverList::iterator it = mObservers.begin(); it != mObservers.end(); it++)
                    (*it)->onRenderCompleted(texture);
                mRendererLock.unlock();

                break;
            }
        }
    }

    glDisableVertexAttribArray(mAttribPos);
    glDisableVertexAttribArray(mAttribTex);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    mRendererLock.lock();
    for(RendererList::iterator it = mRenderers.begin(); it != mRenderers.end(); it++)
        (*it)->onSurfaceRemoved();
    mRendererLock.unlock();

    glDeleteFramebuffers(1, &frameBuffer);
    glDeleteTextures(1, &texture);
}

bool RenderService::initEGL()
{
    EGLint    numConfigs;
    EGLint    majorVersion;
    EGLint    minorVersion;
    EGLConfig config;

    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };

    EGLint attribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
//      EGL_ALPHA_SIZE,      8,
        EGL_NONE
    };

    mDisplay = mPlatform->getEGLDisplay();
    if (!mDisplay)
    {
        mDisplay = eglGetDisplay(mPlatform->getDisplay());
    }

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

    mSurface = eglCreateWindowSurface(mDisplay, config, mPlatform->getWindow(), NULL);
    if (mSurface == EGL_NO_SURFACE)
    {
        LOGE("failed to create EGLSurface");
        return false;
    }

    mContext = eglCreateContext(mDisplay, config, NULL, contextAttribs);
    if (mContext == EGL_NO_CONTEXT)
    {
        LOGE("failed to create EGLContext");
        return false;
    }

    eglQuerySurface(mDisplay, mSurface, EGL_WIDTH, &mRealScreenWidth);
    eglQuerySurface(mDisplay, mSurface, EGL_HEIGHT, &mRealScreenHeight);
    
    LOGI("Screen %dx%d", mRealScreenWidth, mRealScreenHeight);

    return true;
}


void RenderService::addRenderer(IRenderable* renderer)
{
    Lock lock(mRendererLock);

    if(!renderer)
        return;

    RendererList::iterator it = std::find(mRenderers.begin(), mRenderers.end(), renderer);
    if(*it == renderer)
        return;

    mRenderers.push_back(renderer);
    mRenderers.sort(IRenderable::compare);
    Msg msg(MSG_ID_ADD_RENDERER, renderer);
    mMsgQ.put(msg, -1);
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
            Msg msg(MSG_ID_REMOVE_RENDERER, renderer);
            mMsgQ.put(msg, -1);
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
    if(observer == *it)
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

    if (state() != TASK_STATE_RUNNING)
        return;

    mRenderMode = eMode;

    if (eMode == RENDER_MODE_CONTINUOUSLY)
        mTimer.start(DELAY_FOR_30_FPS, true);
    else
        mTimer.stop();
}

void RenderService::sortRenderer()
{
    Lock lock(mRendererLock);
    mRenderers.sort(IRenderable::compare);
}

void RenderService::onTimerExpired(const ITimer* timer)
{
    UNUSED(timer);

    update();
}

void RenderService::onDisplayPlugged()
{
    start();
    
    Msg msg(MSG_ID_UPDATE, true);
    mMsgQ.put(msg);
}

void RenderService::onDisplayRemoved()
{
    stop();
}

void RenderService::update(bool forceToDraw)
{
    Msg msg(MSG_ID_UPDATE, forceToDraw);
    if (!mMsgQ.put(msg, 0))
    {
        if (mMsgQ.isFull())
            LOGW("MsgQ is full.. drop it.");
    }
}

void RenderService::deinitEGL()
{
    eglDestroySurface(mDisplay, mSurface);
}
