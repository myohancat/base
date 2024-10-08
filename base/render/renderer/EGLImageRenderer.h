/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __EGL_IMAGE_RENDERER_H_
#define __EGL_IMAGE_RENDERER_H_

#include "Mutex.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <errno.h>

#include "RendererCommon.h"

class EGLImageRenderer
{
public:
    EGLImageRenderer(float alpha = 1, ColorMode_e eMode = COLOR_MODE_RGBA);
    virtual ~EGLImageRenderer();

    void setView(int x, int y, int width, int height);
    void setAlpha(float alpha);
    void setColorMode(ColorMode_e eMode);

    void setMVP(float* mvp);

    void onDraw(EGLImageKHR image);

protected:
    Mutex  mLock;

    int    mX;
    int    mY;
    int    mWidth;
    int    mHeight;

    GLuint mProgram;
    GLuint mAttribPos;
    GLuint mAttribTex;

    GLuint mTexture;
    GLuint mUniformAlpha;
    GLuint mUniformColorFactor;
    GLuint mUniformMVP;
    GLuint mUniformTexture;

    float        mMVP[16];
    float        mAlpha;
    ColorMode_e  mColorMode;
};

inline void EGLImageRenderer::setView(int x, int y, int width, int height)
{
    mX      = x;
    mY      = y;
    mWidth  = width;
    mHeight = height;
}

#endif // __EGL_IMAGE_RENDERER_H_
