/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "DrmPlatform.h"
#include "Log.h"

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "SysTime.h"
#include <drm_fourcc.h>

#include <GLES2/gl2.h>
#include <algorithm>

#define MAX_DRM_DEVICES 64

static int open_drm_device(const char* device, drmModeRes** resources)
{
    int fd = open(device, O_RDWR);
    if (fd < 0)
        return -1;

    *resources = drmModeGetResources(fd);
    if (*resources == NULL)
    {
        close(fd);
        return -1;
    }

    return fd;
}

static int find_drm_device(drmModeRes** resources)
{
    drmDevicePtr devices[MAX_DRM_DEVICES] = { NULL };
    int num_devices, fd = -1;

    num_devices = drmGetDevices2(0, devices, MAX_DRM_DEVICES);
    if (num_devices < 0)
    {
        LOGE("drmGetDevices2 failed: %s", strerror(-num_devices));
        return -1;
    }

    for (int ii = 0; ii < num_devices; ii++)
    {
        drmDevicePtr device = devices[ii];

        if (!(device->available_nodes & (1 << DRM_NODE_PRIMARY)))
            continue;
        /* OK, it's a primary device. If we can get the
         * drmModeResources, it means it's also a
         * KMS-capable device.
         */
        fd = open_drm_device(device->nodes[DRM_NODE_PRIMARY], resources);
        if (fd > 0)
            break;

        close(fd);
        fd = -1;
    }
    drmFreeDevices(devices, num_devices);

    if (fd < 0)
        LOGE("no drm device found!");

    return fd;
}

static drmModeConnector* find_drm_connector(int fd, drmModeRes *resources, int connector_id)
{
    drmModeConnector *connector = NULL;
    int i;

    if (connector_id >= 0)
    {
        if (connector_id >= resources->count_connectors)
            return NULL;

        connector = drmModeGetConnector(fd, resources->connectors[connector_id]);
        if (connector && connector->connection == DRM_MODE_CONNECTED)
            return connector;

        drmModeFreeConnector(connector);
        return NULL;
    }

    for (i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(fd, resources->connectors[i]);
        if (connector && connector->connection == DRM_MODE_CONNECTED) {
            /* it's connected, let's use this! */
            break;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }

    return connector;
}

static int32_t find_crtc_for_encoder(const drmModeRes *resources, const drmModeEncoder *encoder)
{
    for (int ii = 0; ii < resources->count_crtcs; ii++)
    {
        /* possible_crtcs is a bitmask as described here:
         * https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
         */
        const uint32_t crtc_mask = 1 << ii;
        const uint32_t crtc_id = resources->crtcs[ii];
        if (encoder->possible_crtcs & crtc_mask)
            return crtc_id;
    }

    /* no match found */
    return -1;
}

static int32_t find_crtc_for_connector(int fd, const drmModeRes *resources, const drmModeConnector *connector)
{
    for (int ii = 0; ii < connector->count_encoders; ii++)
    {
        const uint32_t encoder_id = connector->encoders[ii];
        drmModeEncoder *encoder = drmModeGetEncoder(fd, encoder_id);

        if (encoder)
        {
            const int32_t crtc_id = find_crtc_for_encoder(resources, encoder);

            drmModeFreeEncoder(encoder);
            if (crtc_id != 0) {
                return crtc_id;
            }
        }
    }

    /* no match found */
    return -1;
}

static void page_flip_handler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void* data)
{
    UNUSED(fd);
    UNUSED(frame);
    UNUSED(sec);
    UNUSED(usec);

    *(bool *)data = 0;
}

DrmPlatform::DrmPlatform(const char* device)
           : Task("DrmPlatform")
{
__TRACE__
    if (initDRM(device))
    {
        if (initGBM())
            start();
    }

#ifdef USE_SYNC_FLIP
    memset(&mEvCtx, 0x00, sizeof(mEvCtx));
    mEvCtx.version = DRM_EVENT_CONTEXT_VERSION;
    mEvCtx.page_flip_handler = page_flip_handler;
#endif

}

DrmPlatform::~DrmPlatform()
{
__TRACE__
    stop();

    if (mGbmDev)
        gbm_device_destroy(mGbmDev);

    drmModeSetCrtc(mFD, mCrtcId, 0, 0, 0, NULL, 0, NULL);

    SAFE_CLOSE(mFD);
}

bool DrmPlatform::onPreStart()
{
    mExitTask = false;
    mMsgQ.setEOS(false);

    return true;
}

void DrmPlatform::onPreStop()
{
    mExitTask = true;
    mMsgQ.setEOS(true);
}

void DrmPlatform::onPostStop()
{
    if (mPrevBO)
    {
        gbm_surface_release_buffer(mSurface, mPrevBO);
        mPrevBO = NULL;
    }
    mMsgQ.flush();
}

struct drm_fb
{
    struct gbm_bo* bo;
    uint32_t fb_id;
    int fd;
};

void destroy_fb(struct gbm_bo* bo, void *data)
{
    UNUSED(bo);

    struct drm_fb *fb = (struct drm_fb *)data;
    if (fb && fb->fb_id) {
        drmModeRmFB(fb->fd, fb->fb_id);
        free(fb);
    }
}

#if 0
struct drm_fb * drm_fb_get_from_bo(int drm_fd, struct gbm_bo *bo)
{
    struct drm_fb* fb = (struct drm_fb*)gbm_bo_get_user_data(bo);
    uint32_t width, height, format,
         strides[4] = {0}, handles[4] = {0},
         offsets[4] = {0}, flags = 0;
    int ret = -1;

    if (fb)
        return fb;

    fb = (struct drm_fb*)calloc(1, sizeof *fb);
    fb->bo = bo;
    fb->fd = drm_fd;

    width = gbm_bo_get_width(bo);
    height = gbm_bo_get_height(bo);
    format = gbm_bo_get_format(bo);
    const int num_planes = gbm_bo_get_plane_count(bo);

    CHECK("%dx%d format : %d, nplanes: %d", width, height, format, num_planes);

    if (1) {

        uint64_t modifiers[4] = {0};
        modifiers[0] = gbm_bo_get_modifier(bo);
        for (int i = 0; i < num_planes; i++)
        {
            handles[i] = gbm_bo_get_handle_for_plane(bo, i).u32;
            strides[i] = gbm_bo_get_stride_for_plane(bo, i);
            offsets[i] = gbm_bo_get_offset(bo, i);
            modifiers[i] = modifiers[0];
        }

        if (modifiers[0] && modifiers[0] != DRM_FORMAT_MOD_INVALID)
        {
            flags = DRM_MODE_FB_MODIFIERS;
            //printf("Using modifier %" PRIx64 "\n", modifiers[0]);
        }

        ret = drmModeAddFB2WithModifiers(drm_fd, width, height, format, handles, strides, offsets, modifiers, &fb->fb_id, flags);
    }

    if (ret)
    {
        if (flags)
            LOGE("Modifiers failed!");

        memset(handles, 0x00, sizeof(handles));
        memset(strides, 0x00, sizeof(strides));
        memset(offsets, 0x00, sizeof(offsets));
        handles[0] = gbm_bo_get_handle(bo).u32;
        strides[0] = gbm_bo_get_stride(bo);
        ret = drmModeAddFB2(drm_fd, width, height, format, handles, strides, offsets, &fb->fb_id, 0);
    }

    if (ret)
    {
        LOGE("failed to create fb: %s", strerror(errno));
        free(fb);
        return NULL;
    }

    gbm_bo_set_user_data(bo, fb, destroy_fb);

    return fb;
}
#else
struct drm_fb* drm_fb_get_from_bo(int fd, struct gbm_bo* bo)
{
    struct drm_fb* fb = (struct drm_fb*)gbm_bo_get_user_data(bo);
    if (fb) return fb;

    fb = (struct drm_fb*)calloc(1, sizeof(*fb));
    fb->bo = bo;
    fb->fd = fd;

    uint32_t width = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t handle = gbm_bo_get_handle(bo).u32;

    int ret = drmModeAddFB(fd, width, height, 24, 32, stride, handle, &fb->fb_id);
    if (ret)
    {
        LOGE("drmModeAddFB failed");
        return NULL;
    }

    gbm_bo_set_user_data(bo, fb, destroy_fb);
    return fb;
}
#endif

void DrmPlatform::doFlip()
{
    struct gbm_bo* bo = gbm_surface_lock_front_buffer(mSurface);
    struct drm_fb* fb = drm_fb_get_from_bo(mFD, bo);

    if (mPrevBO == NULL)
    {
        drmModeSetCrtc(mFD, mCrtcId, fb->fb_id, 0, 0, &mConnectorId, 1, mDrmMode);
    }
    else
    {
#ifdef USE_SYNC_FLIP
        mWaitingForFlip = true;
        drmModePageFlip(mFD, mCrtcId, fb->fb_id, DRM_MODE_PAGE_FLIP_EVENT, &mWaitingForFlip);
#else
        drmModePageFlip(mFD, mCrtcId, fb->fb_id, 0, NULL);
#endif
    }

#ifdef USE_SYNC_FLIP
    while (mWaitingForFlip)
    {
       fd_set fds;
        FD_ZERO(&fds);
        FD_SET(mFD, &fds);
        select(mFD + 1, &fds, NULL, NULL, NULL);

        drmHandleEvent(mFD, &mEvCtx);
    }
#endif

    if (mPrevBO)
        gbm_surface_release_buffer(mSurface, mPrevBO);

    mPrevBO = bo;
}

void DrmPlatform::flip()
{
#ifdef FLIP_CALL_DIRECTLY
    doFlip();
#else
    mMsgQ.put(Msg(0));
    mWait.wait(mLock, 30);
#endif
}

#ifdef FLIP_CALL_DIRECTLY
void DrmPlatform::run()
{
    while(!mExitTask)
    {
        /* NOP */
        msleep(100);
    }
}
#else
void DrmPlatform::run()
{
    Msg msg;
    while (!mExitTask)
    {
        if (!mMsgQ.get(&msg))
            continue;

        doFlip();
        mWait.signal();
    }
}
#endif

bool DrmPlatform::initDRM(const char* device)
{
    int connector_id = -1;
    const char* mode_str = "";
    unsigned int vrefresh = 0;

    drmModeRes* resources;
    drmModeConnector* connector = NULL;
    drmModeEncoder *encoder = NULL;

    if (device)
        mFD = open_drm_device(device, &resources);
    else
        mFD = find_drm_device(&resources);
    if (mFD < 0)
    {
        LOGE("Cannot open drm device");
        return false;
    }

    connector = find_drm_connector(mFD, resources, connector_id);
    if (!connector)
    {
        LOGE("no connected connector!");
        return false;
    }

    if (mode_str && *mode_str)
    {
        for (int ii = 0; ii < connector->count_modes; ii++)
        {
            drmModeModeInfo *current_mode = &connector->modes[ii];

            if (strcmp(current_mode->name, mode_str) == 0)
            {
                if (vrefresh == 0 || current_mode->vrefresh == vrefresh)
                {
                    mDrmMode = current_mode;
                    break;
                }
            }
        }
        if (!mDrmMode)
            LOGE("requested mode not found, using default mode!");
    }

    if (!mDrmMode)
    {
        int area = 0;
        for (int ii = 0; ii < connector->count_modes; ii++)
        {
            drmModeModeInfo *current_mode = &connector->modes[ii];

            if (current_mode->type & DRM_MODE_TYPE_PREFERRED)
            {
                mDrmMode = current_mode;
                break;
            }

            int current_area = current_mode->hdisplay * current_mode->vdisplay;
            if (current_area > area)
            {
                mDrmMode = current_mode;
                area = current_area;
            }
        }
    }

    if (!mDrmMode)
    {
        LOGE("could not find mode!");
        return false;
    }

    mScreenWidth  = mDrmMode->hdisplay;
    mScreenHeight = mDrmMode->vdisplay;

    CHECK("!!!! %dx%d !!!!", mScreenWidth, mScreenHeight);
    for (int ii = 0; ii < resources->count_encoders; ii++)
    {
        encoder = drmModeGetEncoder(mFD, resources->encoders[ii]);
        if (encoder->encoder_id == connector->encoder_id)
            break;

        drmModeFreeEncoder(encoder);
        encoder = NULL;
    }

    if (encoder)
    {
        mCrtcId = encoder->crtc_id;
    }
    else
    {
        int32_t crtc_id = find_crtc_for_connector(mFD, resources, connector);
        if (crtc_id == -1) {
            printf("no crtc found!\n");
            return -1;
        }

        mCrtcId = crtc_id;
    }

    for (int ii = 0; ii < resources->count_crtcs; ii++)
    {
        if (resources->crtcs[ii] == mCrtcId)
        {
            mCrtcIndex = ii;
            break;
        }
    }

    drmModeFreeResources(resources);

    mConnectorId = connector->connector_id;
    return true;
}

#define WEAK __attribute__((weak))

WEAK struct gbm_surface *
gbm_surface_create_with_modifiers(struct gbm_device *gbm,
                                  uint32_t width, uint32_t height,
                                  uint32_t format,
                                  const uint64_t *modifiers,
                                  const unsigned int count);
WEAK struct gbm_bo *
gbm_bo_create_with_modifiers(struct gbm_device *gbm,
                             uint32_t width, uint32_t height,
                             uint32_t format,
                             const uint64_t *modifiers,
                             const unsigned int count);

bool DrmPlatform::initGBM()
{
    uint32_t format = DRM_FORMAT_XRGB8888;
    uint64_t modifier = DRM_FORMAT_MOD_LINEAR;

    mGbmDev = gbm_create_device(mFD);
    if (!mGbmDev)
        return false;

    if (gbm_surface_create_with_modifiers)
        mSurface = gbm_surface_create_with_modifiers(mGbmDev, mScreenWidth, mScreenHeight, format, &modifier, 1);

    if (!mSurface)
    {
        if (modifier != DRM_FORMAT_MOD_LINEAR)
        {
            LOGE("Modifiers requested but support isn't available");
            return false;
        }

        mSurface = gbm_surface_create(mGbmDev, mScreenWidth, mScreenHeight, format, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    }

    if (!mSurface)
    {
        LOGE("failed to create gbm surface");
        return false;
    }

    return true;
}

NativeDisplayType DrmPlatform::getDisplay()
{
    return (NativeDisplayType)mGbmDev;
}

NativeWindowType DrmPlatform::getWindow()
{
    return (NativeWindowType)mSurface;
}

#include <EGL/eglext.h>
static PFNEGLGETPLATFORMDISPLAYEXTPROC _eglGetPlatformDisplayEXT = NULL;

EGLDisplay DrmPlatform::getEGLDisplay()
{
    if (!_eglGetPlatformDisplayEXT)
        _eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");

    EGLDisplay display = _eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, getDisplay(), NULL);
    LOGD("display %p", display);
    if (!display)
        display = eglGetDisplay(getDisplay());

    return display;
}
