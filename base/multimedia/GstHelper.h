/**
 * Simple multimedia using gstreamer 
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Copyright (c) 2024, MedithinQ. All rights reserved.
 */
#ifndef __GST_HELPER_H_
#define __GST_HELPER_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <gst/video/gstvideometa.h>
#include <gst/allocators/gstdmabuf.h>

#include <gst/gl/gl.h>
#include <gst/gl/gstglfuncs.h>
#include <gst/gl/egl/gstglmemoryegl.h>

#if defined(CONFIG_SOC_RK3588)
#define USE_EGL_IMAGE_KHR
#endif

namespace GstHelper
{

void initialize();

EGLImage create_egl_image_from_dmabuf(GstMemory* mem, const char* format, int width, int height);
EGLImage create_egl_image_from_dmabuf(int dmabuf_fd, GstVideoMeta* meta);

void destroy_egl_image(EGLImage image);

};

#endif /* __GST_HELPER_H_ */
