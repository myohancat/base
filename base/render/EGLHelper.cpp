/**
 * My simple shader util source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "EGLHelper.h"

#include "RenderService.h"
#include "Log.h"

#define USE_EGL_IMAGE_KHR

PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = NULL;

namespace EGLHelper
{

void initialize()
{
    static bool _initialized = false;

    if (!_initialized)
    {
        glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");

        _initialized = true;
    }
}

#define MAX_NUM_PLANES 4
EGLImage create_egl_image(int dmabuf_fd, int format, int width, int height, EGLDisplay display)
{
    initialize();

    int n_planes = -1;
    int offsets[MAX_NUM_PLANES] { 0, };
    int strides[MAX_NUM_PLANES] { 0, };

    switch(format)
    {
        case DRM_FORMAT_RGBA8888:
        case DRM_FORMAT_ABGR8888:
        case DRM_FORMAT_ARGB8888:
        {
            int stride = ALIGN(width, 16) * 4;
            n_planes = 1;
            strides[0] = stride;
            break;
        }
        case DRM_FORMAT_RGB888:
        {
            int stride = ALIGN(width, 16) * 3;
            n_planes = 1;
            strides[0] = stride;
            break;
        }
        case DRM_FORMAT_NV12:
        case DRM_FORMAT_NV16:
        {
            int stride = ALIGN(width, 16);
            n_planes = 2;
            strides[0] = stride;
            offsets[1] = width * height;
            strides[1] = stride;
            break;
        }
        default:
            LOGE("Unsupported format %x (%c%c%c%c)", format, format & 0xFF, (format >> 8) & 0xFF, (format >> 16) & 0xFF, (format >> 24) & 0xFF);
            return EGL_NO_IMAGE_KHR;
    }

#ifdef USE_EGL_IMAGE_KHR
    EGLint attr[64];
#else
    EGLAttrib attr[64];
#endif
    int ii = 0;

    attr[ii++] = EGL_WIDTH;
    attr[ii++] = width;
    attr[ii++] = EGL_HEIGHT;
    attr[ii++] = height;
    attr[ii++] = EGL_LINUX_DRM_FOURCC_EXT;
    attr[ii++] = format;

    for (int plane = 0; plane < n_planes; plane++)
    {
        attr[ii++] = EGL_DMA_BUF_PLANE0_FD_EXT + plane * 3;
        attr[ii++] = dmabuf_fd;
        attr[ii++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT + plane * 3;
        attr[ii++] = offsets[plane];
        attr[ii++] = EGL_DMA_BUF_PLANE0_PITCH_EXT + plane * 3;
        attr[ii++] = strides[plane];
    }

    attr[ii++] = EGL_NONE;

    if (display == NULL)
        display = RenderService::getInstance().getDisplay();

#ifdef USE_EGL_IMAGE_KHR
    EGLImage image = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attr);
#else
    EGLImage image = eglCreateImage(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attr);
#endif

    if (!image)
        LOGE("Cannot create image. format %x (%c%c%c%c), %dx%d", format, format & 0xFF, (format >> 8) & 0xFF, (format >> 16) & 0xFF, (format >> 24) & 0xFF, width, height);

    return image;
}

void destroy_egl_image(EGLImage image, EGLDisplay display)
{
    if (image == EGL_NO_IMAGE_KHR)
        return;

    if (display == NULL)
        display = RenderService::getInstance().getDisplay();

#ifdef USE_EGL_IMAGE_KHR
    eglDestroyImageKHR(display, image);
#else
    eglDestroyImage(display, image);
#endif
}

#include <sys/ioctl.h>
#include <linux/dma-heap.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/dma-buf.h>
#include <sys/ioctl.h>

int create_dma_buf(int format, int width, int height)
{
    int stride, size;

    switch(format)
    {
        case DRM_FORMAT_RGBA8888:
        case DRM_FORMAT_ABGR8888:
        case DRM_FORMAT_ARGB8888:
            stride = ALIGN(width, 16) * 4;
            size = stride * height;
            break;
        case DRM_FORMAT_RGB888:
            stride = ALIGN(width, 16) * 3;
            size = stride * height;
            break;
        case DRM_FORMAT_NV12:
            stride = ALIGN(width, 16);
            size = stride * height * 3 / 2;
            break;
        default:
            LOGE("Unsupported format %x (%c%c%c%c)", format, format & 0xFF, (format >> 8) & 0xFF, (format >> 16) & 0xFF, (format >> 24) & 0xFF);
            return -1;
    }

    return create_dma_buf(size);
}

int create_dma_buf(int size)
{
    int heap_fd = open("/dev/dma_heap/system", O_RDWR | O_CLOEXEC);
    if (heap_fd < 0)
    {
        LOGE("Failed to open dma_heap device");
        return -1;
    }

    struct dma_heap_allocation_data alloc_data;
    memset(&alloc_data, 0x00, sizeof(alloc_data));

    alloc_data.fd_flags = O_RDWR | O_CLOEXEC;
    alloc_data.heap_flags = 0;
    alloc_data.len = size;

    if (ioctl(heap_fd, DMA_HEAP_IOCTL_ALLOC, &alloc_data) < 0)
    {
        LOGE("DMA_HEAP_IOCTL_ALLOC failed. %s", strerror(errno));
        close(heap_fd);
        return -1;
    }

    close(heap_fd);

    return alloc_data.fd;
}

void dma_buf_sync(int fd)
{
    struct dma_buf_sync sync = {
        .flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ
    };

    ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);

    sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ;
    ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
}

} // namespace EGLHelper
