/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "FrameBuffer.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "Log.h"
#include "RenderService.h"
#include "EGLHelper.h"

FrameBuffer::FrameBuffer(int format, int width, int height)
           : mFormat(format), mWidth(width), mHeight(height),
             mTexture(0), mFramebuffer(0), mPreviousFramebuffer(0),
             mEglImage(EGL_NO_IMAGE_KHR), mDmaBufFD(-1)
{
    init();
}

FrameBuffer::~FrameBuffer()
{
    cleanup();
}

void FrameBuffer::init()
{
    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);

    int size = EGLHelper::get_dma_buf_size(mFormat, mWidth, mHeight);
    mDmaBufFD = EGLHelper::create_dma_buf(size);
    mMmapPtr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, mDmaBufFD, 0);
    mMmapSize = size;

    mEglImage = EGLHelper::create_egl_image(mDmaBufFD, mFormat, mWidth, mHeight);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, mEglImage);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &mFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture, 0);
}

void FrameBuffer::cleanup()
{
    if (mFramebuffer)
    {
        glDeleteFramebuffers(1, &mFramebuffer);
        mFramebuffer = 0;
    }
    if (mTexture)
    {
        glDeleteTextures(1, &mTexture);
        mTexture = -1;
    }
    if (mEglImage != EGL_NO_IMAGE_KHR)
    {
        EGLHelper::destroy_egl_image(mEglImage);
        mEglImage = EGL_NO_IMAGE_KHR;
    }

    if (mMmapPtr)
    {
        munmap(mMmapPtr, mMmapSize);
        mMmapPtr = NULL;
    }

    if (mDmaBufFD)
        SAFE_CLOSE(mDmaBufFD);
}

void FrameBuffer::bind()
{
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mPreviousFramebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
}

void FrameBuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, mPreviousFramebuffer);
}

