/**
 * Simple multimedia using gstreamer
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
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

    return startRenderer();
}

void FilePlayer::stop()
{
    stopRenderer();
}

void FilePlayer::restart()
{
    stopRenderer();
    startRenderer();
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
    ss << " ! mppvideodec name=dec format=23 dma-feature=TRUE ";
    ss << " ! appsink name=sink sync=true";

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
