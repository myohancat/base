/**
 * Simple multimedia using gstreamer 
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Copyright (c) 2024, MedithinQ. All rights reserved.
 */
#include "FilePlayer.h"

#include <sstream>

#include "MainLoop.h"
#include "GstHelper.h"
#include "Log.h"

#define _GST_OBJ_UNREF(x)   if (x != NULL) { \
                                gst_object_unref(x); \
                                x = NULL; }

FilePlayer::FilePlayer()
          : mPipeline(NULL),
            mAppSink(NULL)
{
    GstHelper::initialize();
}

FilePlayer::~FilePlayer()
{
}

bool FilePlayer::start(const std::string& filePath, bool isLoop)
{
    mFilePath = filePath;
    mIsLoop = isLoop;

    return GstAppsinkRenderable::start();
}

void FilePlayer::stop()
{
    GstAppsinkRenderable::stop();
}

void FilePlayer::restart()
{
    GstAppsinkRenderable::stop();
    GstAppsinkRenderable::start();
}

GstElement* FilePlayer::createGstPipeline()
{
    GError* err  = NULL;
    
    std::ostringstream ss;

    if (mFilePath.length() == 0)
        return NULL;

    if (::access(mFilePath.c_str(), F_OK))
    {
        LOGE("%s is not found.", mFilePath.c_str());
        return NULL;
    }

    const char* ext = strrchr(mFilePath.c_str(), '.');
    const char* demuxer = "qtdemux";
    if (ext && strcasecmp(ext, ".mkv") == 0)
        demuxer = "matroskademux";

    ss << "filesrc location=" << mFilePath;
    ss << " ! " << demuxer;
    ss << " ! h264parse ";
#if defined(CONFIG_SOC_RK3588)
    ss << " ! mppvideodec name=dec format=23 dma-feature=TRUE ";
#elif defined(CONFIG_SOC_IMX8MQ)
    ss << " ! vpudec name=dec";
#elif defined(CONFIG_SOC_XAVIER_NX)
    ss << " ! nvv4l2decoder name=dec ! nvvidconv ";
    ss << " ! video/x-raw(memory:NVMM), format=(string)RGBA";
#else
    ss << " ! v4l2slh264dec ! glupload ";
#endif
    ss << " ! appsink name=sink";

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

void FilePlayer::destroyGstPipeline()
{
    _GST_OBJ_UNREF(mAppSink);
    _GST_OBJ_UNREF(mPipeline);
}

void FilePlayer::onEOS()
{
    if (mIsLoop)
    {
        MainLoop::getInstance().post([this] {
            restart();
        });
    }
}
