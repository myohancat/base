/**
 * My simple shader util source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __EGL_HELPER_H_
#define __EGL_HELPER_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <drm/drm_fourcc.h>

extern PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

namespace EGLHelper
{

void initialize();

/* format : DRM_FORMAT... */
EGLImage create_egl_image(int dmabuf_fd, int format, int width, int height, EGLDisplay display = nullptr);
void destroy_egl_image(EGLImage image, EGLDisplay display = nullptr);

/* format : DRM_FORMAT... */
int create_dma_buf(int format, int width, int height);
int create_dma_buf(int size);
void dma_buf_sync(int fd);

} // namespace EGLHelper

#endif /* __EGL_HELPER_H_ */
