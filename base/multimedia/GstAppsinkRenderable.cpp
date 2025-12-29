/**
 * Simple multimedia using gstreamer
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "GstAppsinkRenderable.h"

#include "Log.h"
#include "RenderService.h"

#include "GstHelper.h"

#include <sstream>

#define _GST_OBJ_UNREF(x)   if (x != NULL) { \
                                gst_object_unref(x); \
                                x = NULL; }

GstAppsinkRenderable::GstAppsinkRenderable()
          : mRenderer(NULL),
            mIsRunning(false),
            mZorder(10),
            mVisible(false),
            mAlpha(1.0f),
            mPipeline(NULL)
{
    GstHelper::initialize();
}

GstAppsinkRenderable::~GstAppsinkRenderable()
{
    mVisible = false;
    stopRenderer();
}

bool GstAppsinkRenderable::startRenderer()
{
__TRACE__
    Lock lock(mLock);
    if (mIsRunning)
        return false;

    mPipeline = createGstPipeline();
    if (!mPipeline)
        return false;

    // Need to EGLContext
    GstPipeline* pipeline = GST_PIPELINE(mPipeline);
    GstBus* bus = gst_pipeline_get_bus(pipeline);

    gst_bus_enable_sync_message_emission (bus);
    g_signal_connect (bus, "sync-message", G_CALLBACK (sync_bus_call), NULL);
    gst_object_unref(bus); // TODO. Check THIS

    if (getAppSink())
    {
        GstAppSinkCallbacks cb;
        memset(&cb, 0, sizeof(GstAppSinkCallbacks));

        cb.eos         = onEOS;
        cb.new_preroll = onPreroll;
        cb.new_sample  = onBuffer;

        gst_app_sink_set_callbacks(GST_APP_SINK(getAppSink()), &cb, (void*)this, NULL);
    }

    mSampleQueue.setEOS(false);

    const GstStateChangeReturn result = gst_element_set_state(mPipeline, GST_STATE_PLAYING);
    UNUSED(result);

    RenderService::getInstance().addRenderer(this);

    mIsRunning = true;
    return true;
}

void GstAppsinkRenderable::stopRenderer()
{
    if (!mIsRunning)
        return;

    RenderService::getInstance().removeRenderer(this);
    RenderService::getInstance().setRenderMode(RENDER_MODE_CONTINUOUSLY);

    mVisible = false;
    mSampleQueue.setEOS(true);
    mSampleQueue.flush();

    if (getAppSink())
    {
        GstAppSinkCallbacks cb;
        memset(&cb, 0, sizeof(GstAppSinkCallbacks));
        gst_app_sink_set_callbacks(GST_APP_SINK(getAppSink()), &cb, NULL, NULL);
    }

    gst_element_set_state(GST_ELEMENT(mPipeline), GST_STATE_NULL);

    destroyGstPipeline();

    for (std::unordered_map<int,EGLImage>::iterator it = mFrameCache.begin(); it != mFrameCache.end() ; it++)
    {
        EGLImage image = it->second;
        GstHelper::destroy_egl_image(image);
    }
    mFrameCache.clear();

    mIsRunning = false;
}

bool GstAppsinkRenderable::isNeedToDraw()
{
    return mSampleQueue.size() > 0;
}

void GstAppsinkRenderable::onSurfaceCreated(int screenWidth, int screenHeight)
{
__TRACE__
    Lock lock(mLock);

    mRenderer = new EGLImageRenderer();
    mRenderer->setAlpha(mAlpha);
    mRenderer->setMVP(mMVP);

    if (mRectView.isValid())
        mRenderer->setView(mRectView.getX(), mRectView.getY(), mRectView.getWidth(), mRectView.getHeight());
    else
        mRenderer->setView(0, 0, screenWidth, screenHeight);

    if (mRectCrop.isValid())
        mRenderer->setCrop(mRectCrop.getX(), mRectCrop.getY(), mRectCrop.getWidth(), mRectCrop.getHeight());
    else
        mRenderer->setCrop(0, 0, 0, 0);
}

#if defined(CONFIG_SOC_XAVIER_NX)
#include <nvbufsurface.h>
#endif
void GstAppsinkRenderable::onDrawFrame()
{
    if (!mVisible)
        return;

    GstSample* sample;
    if (!mSampleQueue.get(&sample, 0))
    {
        mRenderer->draw(NULL);
        return;
    }

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstCaps* gstCaps = gst_sample_get_caps(sample);
    GstStructure* gstCapsStruct = gst_caps_get_structure(gstCaps, 0);

    int width;
    int height;
    bool needToDestroy = false;
    if (gst_structure_get_int(gstCapsStruct, "width", &width) && gst_structure_get_int(gstCapsStruct, "height", &height))
    {
        GstVideoMeta* meta = gst_buffer_get_video_meta(buffer);
        GstMemory* mem = gst_buffer_peek_memory(buffer, 0);

        GstMapInfo map;
        gst_buffer_map(buffer, &map, GST_MAP_READ);

        const gchar* format = gst_structure_get_string(gstCapsStruct, "format");
        GstCapsFeatures* gstCapsFeatures = gst_caps_get_features(gstCaps, 0);
        //LOGT("%s, %s, %dx%d", gst_caps_features_to_string (gstCapsFeatures), format, width, height);

        /* Get eglImage */
        EGLImage image = NULL;
        if (gst_caps_features_contains(gstCapsFeatures, "memory:DMABuf")
         || gst_caps_features_contains(gstCapsFeatures, "memory:SystemMemory"))
        {
            if (gst_is_dmabuf_memory(mem))
            {
                int dmabuf_fd = gst_dmabuf_memory_get_fd(mem);
                if (mFrameCache.find(dmabuf_fd) == mFrameCache.end())
                {
                    if (meta)
                        image = GstHelper::create_egl_image_from_dmabuf(dmabuf_fd, meta);
                    else
                        image = GstHelper::create_egl_image_from_dmabuf(mem, format, width, height);

                    mFrameCache[dmabuf_fd] = image;
                }
                else
                    image = mFrameCache[dmabuf_fd];

                needToDestroy = false;
            }
        }
        else if (gst_caps_features_contains(gstCapsFeatures, "memory:GLMemory"))
        {
            if (gst_is_gl_memory(mem))
                image = gst_gl_memory_egl_get_image ((GstGLMemoryEGL*) mem);
        }
#if defined(CONFIG_SOC_XAVIER_NX)
        else if (gst_caps_features_contains(gstCapsFeatures, "memory:NVMM"))
        {
            NvBufSurface* surf = (NvBufSurface*)map.data;
            image = surf->surfaceList[0].mappedAddr.eglImage;
            if (image == NULL)
            {
                NvBufSurfaceMapEglImage(surf, 0);
                image = surf->surfaceList->mappedAddr.eglImage;
            }
        }
#endif

        /* Draw eglImage */
        if (image)
        {
            ImageFrame frame;
            frame.mFmt = PIXEL_FORMAT_NOT_SUPPORT; // TODO
            frame.mWidth = width;
            frame.mHeight = height;
            frame.mMemType = MEM_TYPE_EGL;
            frame.mEglImg = image;

            mRenderer->draw(&frame);
            //glFinish();

#if defined(CONFIG_SOC_XAVIER_NX)
            if (gst_caps_features_contains(gstCapsFeatures, "memory:NVMM"))
            {
                NvBufSurface* surf = (NvBufSurface*)map.data;
                NvBufSurfaceUnMapEglImage(surf, 0);
            }
#endif
            if (needToDestroy)
                GstHelper::destroy_egl_image(image);
        }
        else
        {
            LOGE("Cannot get eglImage. %s is not supported.", gst_caps_features_to_string (gstCapsFeatures));
        }

        gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);
}

void GstAppsinkRenderable::onSurfaceRemoved()
{
__TRACE__
    Lock lock(mLock);
    SAFE_DELETE(mRenderer);
}

void GstAppsinkRenderable::onEOS(GstAppSink* sink, void* user_data)
{
    UNUSED(sink);

    GstAppsinkRenderable* pThis = (GstAppsinkRenderable*)user_data;

    RenderService::getInstance().setRenderMode(RENDER_MODE_CONTINUOUSLY);

    pThis->onEOS();
}

GstFlowReturn GstAppsinkRenderable::onPreroll(GstAppSink* sink, void* user_data)
{
    GstAppsinkRenderable* pThis = (GstAppsinkRenderable*)user_data;

    UNUSED(sink);

    RenderService::getInstance().setRenderMode(RENDER_MODE_WHEN_DIRTY);
    pThis->mVisible = true;

    pThis->onPreroll();

    return GST_FLOW_OK;
}

GstFlowReturn GstAppsinkRenderable::onBuffer(GstAppSink* sink, void* user_data)
{
    GstAppsinkRenderable* pThis = (GstAppsinkRenderable*)user_data;

    GstSample* sample = gst_app_sink_pull_sample(sink);
    if (!sample)
        return GST_FLOW_OK;

    Lock lock(pThis->mLock);
    if (!pThis->mRenderer)
    {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    if (pThis->onBuffer(sample))
        return GST_FLOW_OK;

    if (!pThis->mSampleQueue.putForce(sample))
    {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    pThis->requestUpdate();

    return GST_FLOW_OK;
}

void GstAppsinkRenderable::requestUpdate()
{
    RenderService::getInstance().update();
}

// Need to EGLContext
void GstAppsinkRenderable::sync_bus_call(GstBus* bus, GstMessage* msg, gpointer data)
{
    UNUSED(bus);
    UNUSED(data);

    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_NEED_CONTEXT:
        {
            const gchar *context_type;
            gst_message_parse_context_type (msg, &context_type);

            LOGD(" Context type: %s", context_type);
            if (g_strcmp0 (context_type, GST_GL_DISPLAY_CONTEXT_TYPE) == 0)
            {
                GstGLDisplay* gl_display = GST_GL_DISPLAY(gst_gl_display_egl_new_with_egl_display(RenderService::getInstance().getDisplay()));

                GstContext* context = gst_context_new (GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
                gst_context_set_gl_display (context, gl_display);
                gst_element_set_context (GST_ELEMENT (msg->src), context);
                gst_context_unref (context);
            }
            else if (g_strcmp0 (context_type, "gst.gl.app_context") == 0)
            {
                LOGW("TODO. Implements HERE");
                // TODO
            }

            break;
        }
        default:
            //LOGW("Unused msg : %d", msg);
            break;
    }
}
