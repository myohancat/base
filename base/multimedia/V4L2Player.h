/*
 * Simple multimedia using gstreamer
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Author: Soyun.Park   < sypark@medithinq.com >
 * Copyright (c) 2024, MedithinQ. All rights reserved.
 */
#ifndef __V4L2_PLAYER_H_
#define __V4L2_PLAYER_H_

#include "GstAppsinkRenderable.h"

class V4L2Player : GstAppsinkRenderable
{
public:
    V4L2Player(const std::string& device);
    ~V4L2Player();

    bool startViewer();
    void stopViewer();

    void setAlpha(float alpha);
    void setView(int x, int y, int width, int height);
    void setCrop(int x, int y, int width, int height);
    void setFlip(bool horizontal, bool vertical);

protected:
    GstElement* createGstPipeline();
    GstElement* getAppSink();
    void destroyGstPipeline();

    void requestUpdate();

private:
    std::string  mDevice;

    GstElement*  mPipeline;
    GstElement*  mAppSink;

    bool mSkipUpdate = false;
};

inline GstElement* V4L2Player::getAppSink()
{
    return mAppSink;
}

inline void V4L2Player::requestUpdate()
{
    if (mSkipUpdate)
        return;

    GstAppsinkRenderable::requestUpdate();
}

inline void V4L2Player::setFlip(bool horizontal, bool vertical)
{
    float mvp[] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    if (horizontal)
        mvp[0] = -1;
    if (vertical)
        mvp[5] = -1;

    setMVP(mvp);
}

#endif /* __V4L2_PLAYER_H_ */
