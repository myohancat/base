/**
 * Simple multimedia using gstreamer
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __GST_HELPER_H_
#define __GST_HELPER_H_

#include "EGLHelper.h"

#include <gst/video/gstvideometa.h>
#include <gst/allocators/gstdmabuf.h>

#include <gst/gl/gl.h>
#include <gst/gl/gstglfuncs.h>
#include <gst/gl/egl/gstglmemoryegl.h>

namespace GstHelper
{


void initialize();

EGLImageKHR create_egl_image_from_dmabuf(GstMemory* mem, const char* format, int width, int height);
EGLImageKHR create_egl_image_from_dmabuf(int dmabuf_fd, GstVideoMeta* meta);

void destroy_egl_image(EGLImageKHR image);


}; // namespace GstHelper

#endif /* __GST_HELPER_H_ */
