/**
 * Simple multimedia using gstreamer 
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Copyright (c) 2024, MedithinQ. All rights reserved.
 */
#include "UdpStreamPlayer.h"

#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>

#include "NetUtil.h"
#include "GstHelper.h"
#include "Log.h"

#define _GST_OBJ_UNREF(x)   if (x != NULL) { \
                                gst_object_unref(x); \
                                x = NULL; }

#define DEFAULT_PORT         5090

#define KERNEL_RCVBUF_SIZE   41467500
#define RCV_TIMEOUT          1000

UdpStreamPlayer::UdpStreamPlayer()
          : Task("UdpStreamPlayer"),
            mSock(-1),
            mPort(DEFAULT_PORT),
            mPipeline(NULL),
            mAppSrc(NULL),
            mAppSink(NULL),
            mExitTask(false)
{
    GstHelper::initialize();
    socketpair(AF_UNIX, SOCK_STREAM, 0, mPipe);
}

UdpStreamPlayer::~UdpStreamPlayer()
{
    stop();
}

bool UdpStreamPlayer::start(int port)
{
    mPort = port;
    
    if (GstAppsinkRenderable::start())
        return Task::start();

    return false;
}

void UdpStreamPlayer::stop()
{
    Task::stop();
    GstAppsinkRenderable::stop();
}

GstElement* UdpStreamPlayer::createGstPipeline()
{
    GError* err  = NULL;
    
    std::ostringstream ss;

    ss << "appsrc name=src stream-type=0 is-live=true format=3 do-timestamp=false blocksize=4096 max-bytes=0 ";
    ss << " ! application/x-rtp,media=video,clock-rate=90000,encoding-name=H264,payload=96 ";
    ss << " ! rtph264depay max-reorder=0 ! h264parse ";
#if defined(CONFIG_SOC_RK3588)
    ss << " ! mppvideodec name=dec dma-feature=TRUE ";
#elif defined(CONFIG_SOC_IMX8MQ)
    ss << " ! vpudec name=dec ";
#elif defined(CONFIG_SOC_XAVIER_NX)
    ss << " ! nvv4l2decoder name=dec ! nvvidconv ";
    ss << " ! video/x-raw(memory:NVMM), format=(string)RGBA";
#endif
    ss << " ! appsink name=sink async=false sync=false";

    mPipeline = gst_parse_launch(ss.str().c_str(), &err);
    if (err != NULL)
    {
        LOGE("gst_parse_launch failed (%s)", err->message);
        return NULL;
    }

    GstPipeline* pipeline = GST_PIPELINE(mPipeline);
    mAppSrc  = gst_bin_get_by_name(GST_BIN(pipeline), "src");
    mAppSink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");

    return mPipeline;
}

void UdpStreamPlayer::destroyGstPipeline()
{
    // TODO IMPLEMENTS HERE
    _GST_OBJ_UNREF(mAppSink);
    _GST_OBJ_UNREF(mPipeline);
}

bool UdpStreamPlayer::onPreStart()
{
    mExitTask = false;

    mSock = NetUtil::socket(SOCK_TYPE_UDP);
    if (mSock < 0)
        return false;

    /* Increase Kernel RCV Buffer Size */
    NetUtil::socket_set_recv_buf_size(mSock, KERNEL_RCVBUF_SIZE);

    if (NetUtil::bind(mSock, NULL, mPort) < 0)
    {
        SAFE_CLOSE(mSock);
        return false;
    }

    return true;
}

void UdpStreamPlayer::onPreStop()
{
    mExitTask = true;

    write(mPipe[1], "T", 1);
}

void UdpStreamPlayer::onPostStop()
{
    SAFE_CLOSE(mSock);
    fsync(mPipe[0]);
}

#define BUF_LEN   (64*1024)

void UdpStreamPlayer::run()
{
__TRACE__
    static uint8_t data[BUF_LEN];

    while(!mExitTask)
    {
        int ret = NetUtil::recv(mSock, data, BUF_LEN, RCV_TIMEOUT, mPipe[0]);
        if (ret == NET_ERR_TIMEOUT)
            continue;

        if (ret > 0)
        {
#if 0
            uint16_t seq = data[2] << 8 | data[3];
            uint32_t timestamp = ((uint32_t)data[4]  << 8 * 3) \
                               | ((uint32_t)data[5]  << 8 * 2) \
                               | ((uint32_t)data[6]  << 8 * 1) \
                               | ((uint32_t)data[7]  << 8 * 0);

            LOGD("V(%02d), P(%d), X(%d), CC(%02d), M(%d), PT(%02X), SEQ(%d), TIMESTAMP(%u)",
                    (data[0] >> 6) & 0x03, (data[0] >> 5) & 0x01, (data[0] >> 4) & 0x01, data[0] & 0x0F,
                    (data[1] >> 7) & 0x01, data[1] & 0x7F,
                    seq, timestamp);
#endif

            GstMapInfo map;
            GstBuffer* buffer = gst_buffer_new_and_alloc(ret);
            gst_buffer_map(buffer, &map, GST_MAP_WRITE);
            memcpy(map.data, data, ret);
            gst_buffer_unmap(buffer, &map);

            //GST_BUFFER_PTS (buffer) = timestamp;
            //GST_BUFFER_DURATION (buffer) = 0;

            gst_app_src_push_buffer(GST_APP_SRC(mAppSrc), buffer);
        }
    }
}
