/**
 * Simple multimedia using gstreamer 
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Copyright (c) 2024, MedithinQ. All rights reserved.
 */
#ifndef __V4L2_PLAYER_H_
#define __V4L2_PLAYER_H_

#include "GstAppsinkRenderable.h"

class V4L2Player : public GstAppsinkRenderable
{
public:
    V4L2Player();
    ~V4L2Player();

    bool start(const std::string& device);
    void stop();

protected:
    GstElement* createGstPipeline();
    GstElement* getAppSink();
    void destroyGstPipeline();

private:
    std::string  mDevice;

    GstElement*  mPipeline;
    GstElement*  mAppSink;
};

inline GstElement* V4L2Player::getAppSink()
{
    return mAppSink;
}

#endif /* __V4L2_PLAYER_H_ */
