/**
 * Simple multimedia using gstreamer
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __FILE_PLAYER_H_
#define __FILE_PLAYER_H_

#include "GstAppsinkRenderable.h"

class FilePlayer : public GstAppsinkRenderable
{
public:
    FilePlayer();
    ~FilePlayer();

    bool start(const std::string& filePath, bool isLoop = false);
    void stop();

protected:
    GstElement* createGstPipeline();
    GstElement* getAppSink();
    void destroyGstPipeline();

    void restart();
    void onEOS();

private:
    std::string  mFilePath;
    bool         mIsLoop;

    GstElement*  mPipeline;
    GstElement*  mAppSink;
};

inline GstElement* FilePlayer::getAppSink()
{
    return mAppSink;
}

#endif /* __FILE_PLAYER_H_ */
