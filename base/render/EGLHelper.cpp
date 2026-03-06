/**
 * My simple shader util source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "EGLHelper.h"

#include <sys/ioctl.h>
#include <linux/dma-heap.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/dma-buf.h>
#include <sys/ioctl.h>

#include "RenderService.h"
#include "Log.h"

PFNEGLCREATEIMAGEKHRPROC            eglCreateImageKHR  = nullptr;
PFNEGLDESTROYIMAGEKHRPROC           eglDestroyImageKHR = nullptr;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = nullptr;

namespace EGLHelper
{

bool initialize()
{
    static bool _initialized = false;

    if (!_initialized)
    {
        eglCreateImageKHR            = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
        eglDestroyImageKHR           = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
        glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");

        if (!eglCreateImageKHR || !eglDestroyImageKHR || !glEGLImageTargetTexture2DOES)
        {
            LOGE("Failed to load required EGL functions");
            return false;
        }
        _initialized = true;
    }

    return true;
}

#define MAX_NUM_PLANES 4
EGLImageKHR create_egl_image(int dmabuf_fd, int format, int width, int height, EGLDisplay display)
{
    if (!initialize())
        return EGL_NO_IMAGE_KHR;

    int n_planes = -1;
    int offsets[MAX_NUM_PLANES] { 0, };
    int strides[MAX_NUM_PLANES] { 0, };

    switch (format)
    {
        case DRM_FORMAT_RGBA8888:
        case DRM_FORMAT_ABGR8888:
        case DRM_FORMAT_ARGB8888:
        {
            int stride = ALIGN(width, HW_ALIGN_SIZE) * 4;
            n_planes = 1;
            strides[0] = stride;
            break;
        }
        case DRM_FORMAT_RGB888:
        {
            int stride = ALIGN(width, HW_ALIGN_SIZE) * 3;
            n_planes = 1;
            strides[0] = stride;
            break;
        }
        case DRM_FORMAT_NV12:
        case DRM_FORMAT_NV16:
        case DRM_FORMAT_NV21:
        case DRM_FORMAT_NV61:
        {
            int stride = ALIGN(width, HW_ALIGN_SIZE);
            n_planes = 2;
            strides[0] = stride;

            if (format == DRM_FORMAT_NV12 || format == DRM_FORMAT_NV21
             || format == DRM_FORMAT_NV16 || format == DRM_FORMAT_NV61)
            {
                offsets[1] = stride * height;
                strides[1] = stride;
            }
            else
            {
                offsets[1] = stride * height;
                strides[1] = stride * 2;
            }
            break;
        }
        default:
            LOGE("Unsupported format %x (%c%c%c%c)", format, format & 0xFF, (format >> 8) & 0xFF, (format >> 16) & 0xFF, (format >> 24) & 0xFF);
            return EGL_NO_IMAGE_KHR;
    }

    EGLint attr[64];
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

    if (display == EGL_NO_DISPLAY)
        display = RenderService::getInstance().getDisplay();
    if (display == EGL_NO_DISPLAY)
    {
        LOGE("No Display, Cannot create EGLImageKHR.");
        return EGL_NO_IMAGE_KHR;
    }

    EGLImageKHR image = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attr);
    if (image == EGL_NO_IMAGE_KHR)
    {
        EGLint err = eglGetError();
        LOGE("eglCreateImageKHR failed err=0x%x format=%x (%c%c%c%c) %dx%d", err, format, format & 0xFF, (format>>8)&0xFF, (format>>16)&0xFF, (format>>24)&0xFF, width, height);
    }

    return image;
}

void destroy_egl_image(EGLImageKHR image, EGLDisplay display)
{
    if (!initialize())
        return;

    if (image == EGL_NO_IMAGE_KHR)
        return;

    if (display == EGL_NO_DISPLAY)
        display = RenderService::getInstance().getDisplay();

    if (display == EGL_NO_DISPLAY)
        return;

    eglDestroyImageKHR(display, image);
}

int get_dma_buf_size(uint32_t format, int width, int height)
{
    int stride, size;

    switch (format)
    {
        case DRM_FORMAT_RGBA8888:
        case DRM_FORMAT_ABGR8888:
        case DRM_FORMAT_ARGB8888:
            stride = ALIGN(width, HW_ALIGN_SIZE) * 4;
            size = stride * height;
            break;
        case DRM_FORMAT_RGB888:
            stride = ALIGN(width, HW_ALIGN_SIZE) * 3;
            size = stride * height;
            break;
        case DRM_FORMAT_YUYV:
            stride = ALIGN(width, HW_ALIGN_SIZE);
            size   = stride * height * 2;
            break;
        case DRM_FORMAT_NV12:
        case DRM_FORMAT_NV21:
            stride = ALIGN(width, HW_ALIGN_SIZE);
            size = stride * height * 3 / 2;
            break;
        case DRM_FORMAT_NV16:
        case DRM_FORMAT_NV61:
            stride = ALIGN(width, HW_ALIGN_SIZE);
            size = stride * height * 2;
            break;
        default:
            LOGE("Unsupported format %x (%c%c%c%c)", format, format & 0xFF, (format >> 8) & 0xFF, (format >> 16) & 0xFF, (format >> 24) & 0xFF);
            return -1;
    }

    return size;
}

int create_dma_buf(int format, int width, int height, bool continuous)
{
    int size = get_dma_buf_size(format, width, height);
    if (size == -1)
        return -1;

    return create_dma_buf(size, continuous);
}

int create_dma_buf(int size, bool continuous)
{
    int heap_fd = -1;

    if (continuous)
        heap_fd = open("/dev/dma_heap/cma", O_RDWR | O_CLOEXEC);
    else
        heap_fd = open("/dev/dma_heap/system", O_RDWR | O_CLOEXEC);

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

/* RAII Pattern for Dmabuf Sync */
DmabufSync::DmabufSync(int fd, bool needToRead, bool needToWrite)
    : mFD(fd), mNeedToRead(needToRead), mNeedToWrite(needToWrite)
{
    if (mFD < 0) return;

    struct dma_buf_sync sync = {0};
    sync.flags = DMA_BUF_SYNC_START;

    if (mNeedToRead) sync.flags  |= DMA_BUF_SYNC_READ;
    if (mNeedToWrite) sync.flags |= DMA_BUF_SYNC_WRITE;

    if (ioctl(mFD, DMA_BUF_IOCTL_SYNC, &sync) < 0)
        LOGE("DMA_BUF_IOCTL_SYNC START failed: %s", strerror(errno));
}

DmabufSync::~DmabufSync()
{
    if (mFD < 0) return;

    struct dma_buf_sync sync = {0};
    sync.flags = DMA_BUF_SYNC_END;

    if (mNeedToRead) sync.flags  |= DMA_BUF_SYNC_READ;
    if (mNeedToWrite) sync.flags |= DMA_BUF_SYNC_WRITE;

    if (ioctl(mFD, DMA_BUF_IOCTL_SYNC, &sync) < 0)
        LOGE("DMA_BUF_IOCTL_SYNC END failed: %s", strerror(errno));
}

void dma_buf_sync(int fd, bool needToRead, bool needToWrite)
{
    struct dma_buf_sync sync;
    ZERO(sync);

    sync.flags = DMA_BUF_SYNC_START;
    if (needToRead)  sync.flags |= DMA_BUF_SYNC_READ;
    if (needToWrite) sync.flags |= DMA_BUF_SYNC_WRITE;

    ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);

    sync.flags = DMA_BUF_SYNC_END;
    if (needToRead)  sync.flags |= DMA_BUF_SYNC_READ;
    if (needToWrite) sync.flags |= DMA_BUF_SYNC_WRITE;

    ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
}

} // namespace EGLHelper
