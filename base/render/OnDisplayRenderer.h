/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __ON_DISPLAY_RENDERER_H_
#define __ON_DISPLAY_RENDERER_H_

#include <stdint.h>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GLES3/gl3.h>

#include "Task.h"
#include "Queue.h"
#include "Platform.h"
#include "Log.h"

class OnDisplayRenderer : public Task
{
public:
    OnDisplayRenderer();
    ~OnDisplayRenderer();

    void update(int dmafd, int format, int width, int height);

private:
    bool onPreStart();
    void onPreStop();
    void onPostStop();
    void run();

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
    GLuint       mUniformTexture;
    GLuint       mTexture;

    bool         mExitTask;
    int          mRealScreenWidth;
    int          mRealScreenHeight;

    std::unordered_map<int, EGLImage> mImageCache;

    struct DmaBufImage
    {
        int mDmaFD;
        int mFormat;
        int mWidth;
        int mHeight;

        DmaBufImage() : mDmaFD(-1), mFormat(0), mWidth(0), mHeight(0) { }

        DmaBufImage(int dmafd, int format, int width, int height)
            : mDmaFD(dmafd), mFormat(format), mWidth(width), mHeight(height) { }
    };

    class DmaBufImageQueue : public Queue<DmaBufImage, 1>
    {
    protected:
        void dispose(UNUSED_PARAM DmaBufImage image)
        {
            LOGD("OnDisplayRenderer Drop.");
        }
    };

    DmaBufImageQueue mMsgQ;
};


#endif /* __ON_DISPLAY_RENDERER_H_ */
