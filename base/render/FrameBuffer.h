/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __FRAMEBUFFER_H_
#define __FRAMEBUFFER_H_

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "EGLHelper.h"

class FrameBuffer
{
public:
    /* format : DRM_FORMAT... */
    FrameBuffer(int format, int width, int height);
    ~FrameBuffer();

    void bind();
    void unbind();

    int  texture() const;
    int  dmabuf() const;

protected:
    void init();
    void cleanup();

private:
    int      mFormat;
    int      mWidth;
    int      mHeight;

    GLuint   mTexture;
    GLuint   mFramebuffer;

    GLint    mPreviousFramebuffer;

    EGLImage mEglImage;
    int      mDmaBufFD;
};

inline int FrameBuffer::texture() const
{
    return mTexture;
}

inline int FrameBuffer::dmabuf() const
{
    return mDmaBufFD;
}
#endif /* __FRAMEBUFFER_H_ */
