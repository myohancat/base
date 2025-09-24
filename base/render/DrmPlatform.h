/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __DRM_PLATFORM_H_
#define __DRM_PLATFORM_H_

#include "Platform.h"
#include "Task.h"
#include "MsgQ.h"
#include "CondVar.h"

#include <memory>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

//#define USE_SYNC_FLIP /* It cause hangup in 60FPS */
#define FLIP_CALL_DIRECTLY

class DrmPlatform : public IPlatform, Task
{
public:
    DrmPlatform(const char* device = NULL);
    ~DrmPlatform();

    NativeDisplayType getDisplay() override;
    NativeWindowType  getWindow() override;

    EGLDisplay        getEGLDisplay() override;

    int getScreenWidth();
    int getScreenHeight();

    void flip();

protected:
    bool initDRM(const char* device);
    bool initGBM();

private:
    bool onPreStart();
    void onPreStop();
    void onPostStop();

    void run();

    void doFlip();

private:
    MsgQ    mMsgQ;
    bool    mExitTask = false;
    Mutex   mLock;
    CondVar mWait;

    int mFD = -1;
    struct gbm_device*  mGbmDev = NULL;
    struct gbm_surface* mSurface = NULL;

    drmModeModeInfo* mDrmMode = NULL;
    uint32_t  mCrtcId      = 0;
    uint32_t  mCrtcIndex   = 0;
    uint32_t  mConnectorId = 0;

    int    mScreenWidth  = 0;
    int    mScreenHeight = 0;

    struct gbm_bo* mPrevBO = NULL;

#ifdef USE_SYNC_FLIP
    drmEventContext mEvCtx;
    bool mWaitingForFlip = false;
#endif
};

inline int DrmPlatform::getScreenWidth()
{
    return mScreenWidth;
}

inline int DrmPlatform::getScreenHeight()
{
    return mScreenHeight;
}

#endif /* __DRM_PLATFORM_H_ */
