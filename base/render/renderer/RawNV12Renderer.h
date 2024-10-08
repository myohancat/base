/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __RAW_NV12_RENDERER_H_
#define __RAW_NV12_RENDERER_H_

#include "Mutex.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <errno.h>

#include "RendererCommon.h"

class RawNV12Renderer
{
public:
    RawNV12Renderer(float alpha = 1, ColorMode_e eMode = COLOR_MODE_RGBA);
    virtual ~RawNV12Renderer();

    void setView(int x, int y, int width, int height);

    void setAlpha(float alpha);
    void setColorMode(ColorMode_e eMode);
    void setMVP(float* mvp);

    void onDraw(RawImageFrame* image);

protected:
    Mutex  mLock;

    int    mFrameWidth;
    int    mFrameHeight;

    GLuint mProgram;
    GLuint mAttribPos;
    GLuint mAttribTex;

    GLuint mTextureIdY;
    GLuint mTextureIdUV;

    GLuint mUniformAlpha;
    GLuint mUniformColorFactor;
    GLuint mUniformMVP;
    GLuint mUniformTextureY;
    GLuint mUniformTextureUV;

    int   mX;
    int   mY;
    int   mWidth;
    int   mHeight;

    float        mMVP[16];
    float        mAlpha;
    ColorMode_e  mColorMode;
};

inline void RawNV12Renderer::setView(int x, int y, int width, int height)
{
    mX = x;
    mY = y;
    mWidth = width;
    mHeight = height;
}
#endif // __RAW_NV12_RENDERER_H_
