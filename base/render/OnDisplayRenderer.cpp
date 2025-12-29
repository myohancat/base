/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "OnDisplayRenderer.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/eglext.h>
#include <algorithm>
#include <unistd.h>
#include "ShaderUtil.h"
#include "EGLHelper.h"
#include "Log.h"
#include "SysTime.h"
#ifdef USE_DRM_PLATFORM
#include "DrmPlatform.h"
#else
#include "WaylandPlatform.h"
#endif

static char VERTEX_SHADER_SRC[] = R"(#version 300 es
layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec2 a_TexCoords;
out vec2 v_TexCoords;
void main()
{
    gl_Position = a_Position;
    v_TexCoords = a_TexCoords;
}
)";

static char FRAGMENT_SHADER_SRC[] = R"(#version 300 es
precision highp float;
in vec2 v_TexCoords;
uniform sampler2D u_Texture;
out vec4 fragColor;

void main()
{
    fragColor = texture(u_Texture, v_TexCoords);
}
)";

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

OnDisplayRenderer::OnDisplayRenderer()
              : Task(80, "OnDisplayRenderer")
{
}

OnDisplayRenderer::~OnDisplayRenderer()
{
__TRACE__
    stop();
}

bool OnDisplayRenderer::onPreStart()
{
    mExitTask = false;
    mMsgQ.setEOS(false);

#ifdef USE_DRM_PLATFORM
    mPlatform = new DrmPlatform();
#else
    mPlatform = new WaylandPlatform();
#endif

    if (!initEGL())
        return false;

    return true;
}

void OnDisplayRenderer::onPreStop()
{
__TRACE__
    mExitTask = true;
    mMsgQ.setEOS(true);
}

void OnDisplayRenderer::onPostStop()
{
__TRACE__
    deinitEGL();

    SAFE_DELETE(mPlatform);
}

void OnDisplayRenderer::run()
{
__TRACE__
    if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext))
    {
        LOGE("Could not make the current window current !");
        return;
    }

    mProgram = ShaderUtil::createProgram(VERTEX_SHADER_SRC, FRAGMENT_SHADER_SRC);
    mAttribPos          = glGetAttribLocation(mProgram, "a_Position");
    mAttribTex          = glGetAttribLocation(mProgram, "a_TexCoords");
    mUniformTexture     = glGetUniformLocation(mProgram, "u_Texture");

    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    eglSwapInterval(mDisplay, 0);

    DmaBufImage image;
    while(!mExitTask)
    {
        if (!mMsgQ.get(&image, -1))
            continue;

        EGLImage eglImage = NULL;
        if (image.mDmaFD != -1)
        {
            if (mImageCache.find(image.mDmaFD) == mImageCache.end())
            {
                eglImage = EGLHelper::create_egl_image(image.mDmaFD, image.mFormat, image.mWidth, image.mHeight, mDisplay);
                mImageCache[image.mDmaFD] = eglImage;
            }
            else
                eglImage = mImageCache[image.mDmaFD];
        }

        glViewport(0, 0, mRealScreenWidth, mRealScreenHeight);

        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        glUseProgram(mProgram);

        glVertexAttribPointer(mAttribPos, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), VERTICES);
        glVertexAttribPointer(mAttribTex, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), TEX_COORDS);

        glEnableVertexAttribArray(mAttribPos);
        glEnableVertexAttribArray(mAttribTex);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mTexture);
        glUniform1i(mUniformTexture, 0);

        if (eglImage)
            glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        eglSwapBuffers(mDisplay, mSurface);
        mPlatform->flip();
    }

    glDisableVertexAttribArray(mAttribPos);
    glDisableVertexAttribArray(mAttribTex);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    glDeleteProgram(mProgram);
    glDeleteTextures(1, &mTexture);

    for (auto &kv : mImageCache)
        eglDestroyImageKHR(mDisplay, kv.second);
    mImageCache.clear();
}

bool OnDisplayRenderer::initEGL()
{
    EGLint    numConfigs;
    EGLint    majorVersion;
    EGLint    minorVersion;
    EGLConfig config;

    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE, EGL_NONE };

    EGLint attribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      0,
        EGL_NONE
    };

    mDisplay = mPlatform->getEGLDisplay();
    if (!mDisplay)
        mDisplay = eglGetDisplay(mPlatform->getDisplay());

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

    CHECK("Screen %dx%d", mRealScreenWidth, mRealScreenHeight);

    return true;
}

void OnDisplayRenderer::deinitEGL()
{
    eglDestroySurface(mDisplay, mSurface);
}


void OnDisplayRenderer::update(int dmafd, int format, int width, int height)
{
    UNUSED(dmafd);
    UNUSED(width);
    UNUSED(height);

    mMsgQ.put(DmaBufImage(dmafd, format, width, height));
}

