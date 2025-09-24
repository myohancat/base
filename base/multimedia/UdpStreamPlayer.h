/**
 * Simple multimedia using gstreamer
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Copyright (c) 2024, MedithinQ. All rights reserved.
 */
#ifndef __UDP_STREAM_PLAYER_H_
#define __UDP_STREAM_PLAYER_H_

#include <gst/app/gstappsrc.h>

#include "GstAppsinkRenderable.h"

class UdpStreamPlayer : public GstAppsinkRenderable, Task
{
public:
    UdpStreamPlayer();
    ~UdpStreamPlayer();

    bool start(int port);
    void stop();

protected:
    GstElement* createGstPipeline();
    GstElement* getAppSink();
    void destroyGstPipeline();

    bool onPreStart();
    void onPreStop();
    void onPostStop();
    void run();

    int  getFD() const;

private:
    int          mPipe[2];
    int          mSock;
    int          mPort;

    GstElement*  mPipeline;
    GstElement*  mAppSrc;
    GstElement*  mAppSink;

    bool mExitTask;
};

inline GstElement* UdpStreamPlayer::getAppSink()
{
    return mAppSink;
}

inline int UdpStreamPlayer::getFD() const
{
    return mSock;
}
#endif /* __UDP_STREAM_PLAYER_H_ */
