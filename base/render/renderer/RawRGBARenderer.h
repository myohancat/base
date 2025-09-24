/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __RAW_RGBA_RENDERER_H_
#define __RAW_RGBA_RENDERER_H_

#include "Mutex.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <errno.h>

#include "Renderer.h"

class RawRGBARenderer : public IRenderer
{
public:
    RawRGBARenderer(float alpha = 1, ColorMode_e eMode = COLOR_MODE_RGBA);
    virtual ~RawRGBARenderer();

    void setColorMode(ColorMode_e eMode);
    void setView(int x, int y, int width, int height);
    void setCrop(int x, int y, int width, int height);
    void setAlpha(float alpha);
    void setMVP(float* mvp);

    void draw(ImageFrame* frame);

protected:
    Mutex  mLock;

    GLuint mProgram;
    GLuint mAttribPos;
    GLuint mAttribTex;

    GLuint mTexture;
    GLuint mUniformAlpha;
    GLuint mUniformColorFactor;
    GLuint mUniformMVP;
    GLuint mUniformTexture;

    int    mX;
    int    mY;
    int    mWidth;
    int    mHeight;

    float mMVP[16];
    float mTexCoords[8];
    float mAlpha;

    ColorMode_e  mColorMode;
};

inline void RawRGBARenderer::setView(int x, int y, int width, int height)
{
    mX = x;
    mY = y;
    mWidth = width;
    mHeight = height;
}

inline void RawRGBARenderer::setCrop(int x, int y, int width, int height)
{
    /* TODO. IMPLEMENTS HERE */
    UNUSED(x);
    UNUSED(y);
    UNUSED(width);
    UNUSED(height);
}

#endif // __RAW_RGBA_RENDERER_H_
