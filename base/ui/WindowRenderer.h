/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __WINDOW_RENDERER_H_
#define __WINDOW_RENDERER_H_

#include "Mutex.h"
#include "Rectangle.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <errno.h>

class Window;

class WindowRenderer
{
public:
    WindowRenderer(Window* window);
    virtual ~WindowRenderer();

    virtual void setCrop(int x, int y, int width, int height);
    virtual void setMVP(float* mvp);

    virtual void onDraw(void* pixels);

protected:
    Mutex   mLock;

    Window* mWindow;

    GLuint  mProgram;
    GLuint  mAttribPos;
    GLuint  mAttribTex;

    GLuint  mTexture;
    GLuint  mUniformAlpha;
    GLuint  mUniformMVP;
    GLuint  mUniformTexture;

    Rectangle mRectCrop;

    float   mMVP[16];
    float   mTexCoords[8];
};


#endif // __UI_RENDERER_H_
