/**
 * Simple multimedia using gstreamer
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Copyright (c) 2024, MedithinQ. All rights reserved.
 */
#include "GstHelper.h"

#include <drm/drm_fourcc.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include "RenderService.h"
#include "Log.h"

#define MAX_NUM_PLANES 2

//#define ENABLE_DUMP_ATTR

namespace GstHelper
{

void initialize()
{
    static bool _initialized = false;

    if (!_initialized)
    {
        gst_init(NULL, NULL);
        _initialized = true;
    }
}

EGLImage create_egl_image_from_dmabuf(GstMemory* mem, const char* format, int width, int height)
{
    int dmabuf_fd = gst_dmabuf_memory_get_fd(mem);
    int n_planes = -1;
    int offset[MAX_NUM_PLANES] { 0, };
    int stride[MAX_NUM_PLANES] { 0, };

    EGLint fourcc = 0;
    if (strcasecmp(format, "RGBA") == 0)
    {
        fourcc = DRM_FORMAT_ABGR8888; // TODO
        n_planes = 1;
        offset[0] = 0;
        stride[0] = width * 4;
    }
    else if (strcasecmp(format, "BGR") == 0)
    {
        fourcc = DRM_FORMAT_RGB888; // TODO
        n_planes = 1;
        offset[0] = 0;
        stride[0] = width * 3;
    }
    else if (strcasecmp(format, "NV12") == 0)
    {
        fourcc = DRM_FORMAT_NV12;
        n_planes = 2;
        int height2 = mem->maxsize / width / n_planes;
        height2 = MAX(height2, height);
        offset[0] = 0;
        stride[0] = width;
        offset[1] = width * height2;
        stride[1] = width;
    }
    else if (strcasecmp(format, "NV16") == 0)
    {
        fourcc = DRM_FORMAT_NV16;
        n_planes = 2;
        offset[0] = 0;
        stride[0] = width;
        offset[1] = width * height;
        stride[1] = width;
    }
#if 1 /* TODO. rk3588 does not support NV24 for EGL */
    else if (strcasecmp(format, "NV24") == 0)
    {
        LOGW("video format NV24");
        fourcc = DRM_FORMAT_NV24;
        n_planes = 2;
        offset[0] = 0;
        stride[0] = width;
        offset[1] = width * height;
        stride[1] = width * 2;
    }
#endif
    else
    {
        LOGE("Unsupported format %s", format);
        return NULL;
    }

#ifdef USE_EGL_IMAGE_KHR
    EGLint attr[6 + 6*(MAX_NUM_PLANES) + 1] =
#else
    EGLAttrib attr[6 + 6*(MAX_NUM_PLANES) + 1] =
#endif
    {
        EGL_WIDTH, width,
        EGL_HEIGHT, height,
        EGL_LINUX_DRM_FOURCC_EXT, fourcc
    };

    int ii = 6;
    attr[ii++] = EGL_DMA_BUF_PLANE0_FD_EXT;
    attr[ii++] = dmabuf_fd;
    attr[ii++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
    attr[ii++] = offset[0];
    attr[ii++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
    attr[ii++] = stride[0];

#ifdef ENABLE_DUMP_ATTR
    LOGD("Without Meta, Generated.");
    LOGD("%c%c%c%c %dx%d", fourcc & 0xFF, (fourcc >> 8) & 0xFF, (fourcc >> 16) & 0xFF, (fourcc >> 24) & 0xFF,  width, height);
    LOGD("[0] offset:%d, stride: %d", offset[0], stride[0]);
#endif
    if (n_planes > 1)
    {
        attr[ii++] = EGL_DMA_BUF_PLANE1_FD_EXT;
        attr[ii++] = dmabuf_fd;
        attr[ii++] = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
        attr[ii++] = offset[1];
        attr[ii++] = EGL_DMA_BUF_PLANE1_PITCH_EXT;
        attr[ii++] = stride[1];
#ifdef ENABLE_DUMP_ATTR
    LOGD("[1] offset:%d, stride: %d", offset[1], stride[1]);
#endif
    }
    attr[ii] = EGL_NONE;

#ifdef USE_EGL_IMAGE_KHR
    EGLImage image = eglCreateImageKHR(RenderService::getInstance().getDisplay(), EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attr);
#else
    EGLImage image = eglCreateImage(RenderService::getInstance().getDisplay(), EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attr);
#endif
    if (!image)
        LOGE("Cannot create image. format : %04X, %dx%d", fourcc, width, height);

    return image;
}

EGLImage create_egl_image_from_dmabuf(int dmabuf_fd, GstVideoMeta* meta)
{
    EGLint fourcc = -1;
    switch(meta->format)
    {
        case GST_VIDEO_FORMAT_RGBA:
            fourcc = DRM_FORMAT_ABGR8888; // TODO
            break;
        case GST_VIDEO_FORMAT_BGR:
            fourcc = DRM_FORMAT_RGB888; // TODO
            break;
        case GST_VIDEO_FORMAT_NV12:
            fourcc = DRM_FORMAT_NV12;
            break;
        case GST_VIDEO_FORMAT_NV16:
            fourcc = DRM_FORMAT_NV16;
            break;
#if 1 /* TODO. rk3588 does not support NV24 for EGL */
        case GST_VIDEO_FORMAT_NV24:
            LOGW("video format NV24");
            fourcc = DRM_FORMAT_NV24;
            break;
#endif
        default:
            LOGE("Meta format Unsupported format : %04X", meta->format);
            return NULL;
    }

#ifdef USE_EGL_IMAGE_KHR
    EGLint attr[6 + 6*(MAX_NUM_PLANES) + 1] =
#else
    EGLAttrib attr[6 + 6*(MAX_NUM_PLANES) + 1] =
#endif
    {
        EGL_WIDTH,  (EGLint)meta->width,
        EGL_HEIGHT, (EGLint)meta->height,
        EGL_LINUX_DRM_FOURCC_EXT, fourcc
    };

    int ii = 6;
    attr[ii++] = EGL_DMA_BUF_PLANE0_FD_EXT;
    attr[ii++] = dmabuf_fd;
    attr[ii++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
    attr[ii++] = meta->offset[0];
    attr[ii++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
    attr[ii++] = meta->stride[0];
#ifdef ENABLE_DUMP_ATTR
    LOGD("From Meta");
    LOGD("%c%c%c%c %dx%d", fourcc & 0xFF, (fourcc >> 8) & 0xFF, (fourcc >> 16) & 0xFF, (fourcc >> 24) & 0xFF, meta->width, meta->height);
    LOGD("[0] offset:%d, stride: %d", meta->offset[0], meta->stride[0]);
#endif

    if (meta->n_planes > 1)
    {
        attr[ii++] = EGL_DMA_BUF_PLANE1_FD_EXT;
        attr[ii++] = dmabuf_fd;
        attr[ii++] = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
        attr[ii++] = meta->offset[1];
        attr[ii++] = EGL_DMA_BUF_PLANE1_PITCH_EXT;
        attr[ii++] = meta->stride[1];
#ifdef ENABLE_DUMP_ATTR
        LOGD("[1] offset:%d, stride: %d", meta->offset[1], meta->stride[1]);
#endif
    }
    attr[ii] = EGL_NONE;

#ifdef USE_EGL_IMAGE_KHR
    EGLImage image = eglCreateImageKHR(RenderService::getInstance().getDisplay(), EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attr);
#else
    EGLImage image = eglCreateImage(RenderService::getInstance().getDisplay(), EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attr);
#endif

    if (!image)
        LOGE("Cannot create image. format : %04X, %dx%d", fourcc, meta->width, meta->height);

    return image;
}

void destroy_egl_image(EGLImage image)
{
#ifdef USE_EGL_IMAGE_KHR
    eglDestroyImageKHR(RenderService::getInstance().getDisplay(), image);
#else
    eglDestroyImage(RenderService::getInstance().getDisplay(), image);
#endif
}

};

