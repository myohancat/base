/**
 * Simple multimedia using gstreamer
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __GST_APP_SINK_RENDERABLE_H_
#define __GST_APP_SINK_RENDERABLE_H_

#include "Rectangle.h"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string>

#include "RenderService.h"
#include "EGLImageRenderer.h"

#include "Queue.h"
#include "Log.h"

class GstAppsinkRenderable : public IRenderable
{
public:
    GstAppsinkRenderable();
    ~GstAppsinkRenderable();

    void setAlpha(float alpha);
    void setView(int x, int y, int width, int height);
    void setCrop(int x, int y, int width, int height);
    void setMVP(float* mvp);

    void setZOrder(int zorder);

protected:
    bool startRenderer();
    void stopRenderer();

    virtual GstElement* createGstPipeline()  = 0;
    virtual GstElement* getAppSink()         = 0;
    virtual void destroyGstPipeline()        = 0;

    virtual void onPreroll() { };
    virtual bool onBuffer(UNUSED_PARAM GstSample* sample) { return false; };
    virtual void onEOS() { };

    virtual void requestUpdate();

protected:
    std::unordered_map<int, EGLImage> mFrameCache;
    EGLImageRenderer*  mRenderer;

private:
    /* DO NOT OVERRIDE THIS */
    int  getZOrder();
    bool isVisible();
    void onSurfaceCreated(int screenWidth, int screenHeight) override;
    bool isNeedToDraw() override;
    void onDrawFrame() override;
    void onSurfaceRemoved() override;

private:
    class GstSampleQueue : public Queue<GstSample*, 1>
    {
        protected:
            void dispose(GstSample* sample)
            {
                //LOGD("Drop.");
                gst_sample_unref(sample);
            }
    };

private:
    Mutex        mLock;
    bool         mIsRunning;

    int          mZorder;
    bool         mVisible;

    float        mAlpha;
    Rectangle    mRectView;
    Rectangle    mRectCrop;
    float        mMVP[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    GstSampleQueue mSampleQueue;

    GstElement*  mPipeline;

private:
    static void sync_bus_call(GstBus* bus, GstMessage* msg, gpointer data);

    static GstFlowReturn onPreroll(GstAppSink* sink, void* user_data);
    static GstFlowReturn onBuffer(GstAppSink* sink, void* user_data);
    static void onEOS(GstAppSink* sink, void* user_data);
};

inline void GstAppsinkRenderable::setAlpha(float alpha)
{
    Lock lock(mLock);

    mAlpha = alpha;
    if (mRenderer)
        mRenderer->setAlpha(alpha);
}

inline void GstAppsinkRenderable::setView(int x, int y, int width, int height)
{
    Lock lock(mLock);

    mRectView.setRectangle(x, y, width, height);
    if (mRenderer)
        mRenderer->setView(x, y, width, height);
}

inline void GstAppsinkRenderable::setCrop(int x, int y, int width, int height)
{
    Lock lock(mLock);

    mRectCrop.setRectangle(x, y, width, height);
    if (mRenderer)
        mRenderer->setCrop(x, y, width, height);
}

inline void GstAppsinkRenderable::setMVP(float* mvp)
{
    Lock lock(mLock);

    for(int ii = 0; ii < 16; ii++)
        mMVP[ii] = mvp[ii];

    if (mRenderer)
        mRenderer->setMVP(mvp);
}

inline int GstAppsinkRenderable::getZOrder()
{
    return mZorder;
}

inline void GstAppsinkRenderable::setZOrder(int zorder)
{
    mZorder = zorder;
}

inline bool GstAppsinkRenderable::isVisible()
{
    return mVisible;
}

#endif /* __GST_APP_SINK_RENDERABLE_H_ */
