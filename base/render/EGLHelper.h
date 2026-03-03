/**
 * My simple shader util source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __EGL_HELPER_H_
#define __EGL_HELPER_H_

#include <GLES/gl.h>
#include <GLES/glext.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <drm/drm_fourcc.h>

#define HW_ALIGN_SIZE    64

/* You must use this api after EGLHelper::initialize() */
extern PFNEGLCREATEIMAGEKHRPROC            eglCreateImageKHR;
extern PFNEGLDESTROYIMAGEKHRPROC           eglDestroyImageKHR;
extern PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

namespace EGLHelper
{

bool initialize();

/* format : DRM_FORMAT... */
EGLImageKHR create_egl_image(int dmabuf_fd, int format, int width, int height, EGLDisplay display = EGL_NO_DISPLAY);
void destroy_egl_image(EGLImageKHR image, EGLDisplay display = EGL_NO_DISPLAY);

/* format : DRM_FORMAT... */
int get_dma_buf_size(uint32_t format, int width, int height);
int create_dma_buf(int size, bool continuous = false);
int create_dma_buf(int format, int width, int height, bool continuous = false);

/* RAII */
class DmabufSync
{
public:
    DmabufSync(int fd, bool needToRead = true, bool needToWrite = false);
    ~DmabufSync();

private:
    int mFD = -1;
    bool mNeedToRead  = false;
    bool mNeedToWrite = false;
};

void dma_buf_sync(int fd, bool needToRead = true, bool needToWrite = false);

} // namespace EGLHelper

#endif /* __EGL_HELPER_H_ */
