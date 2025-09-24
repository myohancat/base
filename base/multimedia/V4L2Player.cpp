/**
 * Simple multimedia using gstreamer
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Author: Soyun.Park   < sypark@medithinq.com >
 * Copyright (c) 2024, MedithinQ. All rights reserved.
 */
#include "V4L2Player.h"

#include "GstHelper.h"
#include <sstream>

#define _GST_OBJ_UNREF(x)   if (x != NULL) { \
                                gst_object_unref(x); \
                                x = NULL; }

V4L2Player::V4L2Player(const std::string& device)
          : mDevice(device),
            mPipeline(NULL),
            mAppSink(NULL)
{
    GstHelper::initialize();
}

V4L2Player::~V4L2Player()
{
    stopRenderer();
}

bool V4L2Player::startViewer()
{
    return startRenderer();
}

void V4L2Player::stopViewer()
{
    stopRenderer();
}

void V4L2Player::setView(int x, int y, int width, int height)
{
    return GstAppsinkRenderable::setView(x, y, width, height);
}

void V4L2Player::setAlpha(float alpha)
{
    return GstAppsinkRenderable::setAlpha(alpha);
}

void V4L2Player::setCrop(int x, int y, int width, int height)
{
    return GstAppsinkRenderable::setCrop(x, y, width, height);
}

GstElement* V4L2Player::createGstPipeline()
{
    GError* err  = NULL;

    std::ostringstream ss;

    if (mDevice.length() == 0)
        return NULL;

    ss << "v4l2src io-mode=4 device=" << mDevice << " do-timestamp=FALSE";
    //ss << " ! glupload name=upload ";
    ss << " ! appsink name=sink sync=false async=false";

    mPipeline = gst_parse_launch(ss.str().c_str(), &err);
    if (err != NULL)
    {
        LOGE("gst_parse_launch() failed. (%s)", err->message);
        return NULL;
    }

    GstPipeline* pipeline = GST_PIPELINE(mPipeline);
    mAppSink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");

    return mPipeline;
}

void V4L2Player::destroyGstPipeline()
{
    // TODO IMPLEMENTS HERE
    _GST_OBJ_UNREF(mAppSink);
    _GST_OBJ_UNREF(mPipeline);
}
