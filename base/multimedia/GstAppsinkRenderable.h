/**
 * Simple multimedia using gstreamer 
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Copyright (c) 2024, MedithinQ. All rights reserved.
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

    void setView(int x, int y, int width, int height);
    void setZOrder(int zorder);

protected:
    bool start();
    void stop();

    virtual GstElement* createGstPipeline()  = 0;
    virtual GstElement* getAppSink()         = 0;
    virtual void destroyGstPipeline()        = 0;

    virtual void onPreroll() { };
    virtual bool onBuffer(UNUSED_PARAM GstSample* sample) { return false; };
    virtual void onEOS() { };

protected:
    EGLImageRenderer*  mRenderer;

private:
    /* DO NOT OVERRIDE THIS */
    int  getZOrder();
    bool isVisible();
    void onSurfaceCreated(int screenWidth, int screenHeight);
    bool isNeedToDraw();
    void onDrawFrame();
    void onSurfaceRemoved();

private:
    class GstSampleQueue : public Queue<GstSample*, 1>
    {
        protected:
            void dispose(GstSample* sample)
            {
                LOGD("Drop.");
                gst_sample_unref(sample);
            }
    };

private:
    Mutex        mLock;
    bool         mIsRunning;

    int          mZorder;
    bool         mVisible;
    Rectangle    mRectView;

    GstSampleQueue mSampleQueue;

    GstElement*  mPipeline;

private:
    static void sync_bus_call(GstBus* bus, GstMessage* msg, gpointer data);

    static GstFlowReturn onPreroll(GstAppSink* sink, void* user_data);
    static GstFlowReturn onBuffer(GstAppSink* sink, void* user_data);
    static void onEOS(GstAppSink* sink, void* user_data);
};

inline void GstAppsinkRenderable::setView(int x, int y, int width, int height)
{
    mRectView.setRectangle(x, y, width, height);
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
