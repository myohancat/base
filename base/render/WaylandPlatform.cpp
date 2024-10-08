/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "WaylandPlatform.h"
#include "Log.h"

#include <string.h>
#include <stdlib.h>

#include <GLES2/gl2.h>
#include <algorithm>

struct wl_registry_listener WaylandPlatform::registry_listener =
{
    WaylandPlatform::global_registry_handler,
    WaylandPlatform::global_registry_remover
};

struct wl_output_listener WaylandPlatform::output_listener =
{
    WaylandPlatform::output_handle_geometry,
    WaylandPlatform::output_handle_mode,
    WaylandPlatform::output_handle_done,
    WaylandPlatform::output_handle_scale,
    NULL, //void (*name)(void *data, struct wl_output *wl_output, const char *name);
    NULL, //void (*description)(void *data, struct wl_output *wl_output, const char *description);
};

#define OUTPUT_VERSION 3
void WaylandPlatform::global_registry_handler(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
{
    UNUSED(version);

    LOGD("registry event for %s id %d", interface, id);
    WaylandPlatform* pThis = (WaylandPlatform*)data;

    if (strcmp(interface, "wl_compositor") == 0)
        pThis->mCompositor = (wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
#ifndef USE_XDG_SHELL
    else if (strcmp(interface, "wl_shell") == 0)
        pThis->mShell = (wl_shell*)wl_registry_bind(registry, id, &wl_shell_interface, 1);
#else
    else if (strcmp(interface, "xdg_wm_base") == 0)
    {
        struct display* d = (struct display*)data;
        pThis->mWmBase = (xdg_wm_base*)wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(pThis->mWmBase, &wm_base_listener, d);
    }
#endif
    else if (strcmp(interface, "wl_shm") == 0)
    {
        pThis->mShm = (wl_shm *)wl_registry_bind(registry, id, &wl_shm_interface, 1);
    }
    else if (strcmp(interface, "wl_output") == 0)
    {
        struct wl_output* output = (struct wl_output*)wl_registry_bind(registry, id, &wl_output_interface, MIN(version, OUTPUT_VERSION));
        wl_output_add_listener (output, &output_listener, pThis);
    }
}

void WaylandPlatform::global_registry_remover(void* data, struct wl_registry* registry, uint32_t id)
{
    LOGD("registry losing event for %d", id);
    UNUSED(data);
    UNUSED(registry);
}

void WaylandPlatform::output_handle_geometry(void* data, struct wl_output* wl_output, int x, int y, int physical_width, int physical_height,
                                             int subpixel, const char* make, const char* model, int32_t transform)
{
    WaylandPlatform* pThis = (WaylandPlatform*)data;
    UNUSED(wl_output);
    UNUSED(pThis);
    UNUSED(x);
    UNUSED(y);
    UNUSED(physical_width);
    UNUSED(physical_height);
    UNUSED(subpixel);
    UNUSED(make);
    UNUSED(model);
    UNUSED(transform);

    //LOGD("output_handle_geometry() x : %d, y : %d, physical_width : %d, physical_height : %d", x, y, physical_width, physical_height);
    //LOGD("output_handle_geometry() usbpixel : %d, make : %s, model : %s, transform : %d", subpixel, make, model, transform);
}

void WaylandPlatform::output_handle_done(void* data, struct wl_output* wl_output)
{
    WaylandPlatform* pThis = (WaylandPlatform*)data;
    UNUSED(wl_output);
    UNUSED(pThis);

}

void WaylandPlatform::output_handle_scale(void* data, struct wl_output* wl_output, int32_t scale)
{
    WaylandPlatform* pThis = (WaylandPlatform*)data;
    UNUSED(wl_output);
    UNUSED(pThis);
    UNUSED(scale);

    //LOGD("output_handle_scale() scale : %d", scale)
}

void WaylandPlatform::output_handle_mode(void* data, struct wl_output* wl_output, uint32_t flags, int width, int height, int refresh)
{
    WaylandPlatform* pThis = (WaylandPlatform*)data;
    UNUSED(wl_output);
    UNUSED(pThis);
    UNUSED(flags);
    UNUSED(width);
    UNUSED(height);
    UNUSED(refresh);

    //LOGD("output_handle_mode() flags : %d, width : %d, height : %d, refresh : %d", flags, width, height, refresh);
    if (flags & WL_OUTPUT_MODE_CURRENT)
    {
        LOGD("Select Display Size : %dx%d", width, height);
        pThis->mScreenWidth = width;
        pThis->mScreenHeight =height;
    }
}

#ifndef USE_XDG_SHELL
void WaylandPlatform::handle_ping(void* data, struct wl_shell_surface* shell_surface, uint32_t serial)
{
    WaylandPlatform* pThis = (WaylandPlatform*)data;
    UNUSED(pThis);

    wl_shell_surface_pong(shell_surface, serial);
}

void WaylandPlatform::handle_configure(void* data, struct wl_shell_surface* shell_surface, uint32_t edges, int32_t width, int32_t height)
{
    WaylandPlatform* pThis = (WaylandPlatform*)data;
    UNUSED(pThis);
    UNUSED(shell_surface);
    UNUSED(edges);
    UNUSED(width);
    UNUSED(height);
}

void WaylandPlatform::handle_popup_done(void* data, struct wl_shell_surface* shell_surface)
{
    WaylandPlatform* pThis = (WaylandPlatform*)data;
    UNUSED(pThis);
    UNUSED(shell_surface);
}

struct wl_shell_surface_listener WaylandPlatform::shell_surface_listener =
{
    handle_ping,
    handle_configure,
    handle_popup_done
};
#else
void WaylandPlatform::xdg_wm_base_ping(void* data, struct xdg_wm_base* shell, uint32_t serial)
{
    UNUSED(data);

    xdg_wm_base_pong(shell, serial);
}

struct xdg_wm_base_listener WaylandPlatform::wm_base_listener =
{
    xdg_wm_base_ping,
};

void  WaylandPlatform::handle_surface_configure(void* data, struct xdg_surface* surface, uint32_t serial)
{
    WaylandPlatform* pThis = (WaylandPlatform*)data;

    xdg_surface_ack_configure(surface, serial);

    pThis->mWaitForConfigure = false;
}

struct xdg_surface_listener WaylandPlatform::xdg_surface_listener = 
{
    handle_surface_configure
};
#endif

WaylandPlatform::WaylandPlatform()
               : Task("WaylandPlatform")
{
__TRACE__
    mDisplay = wl_display_connect(NULL);
    if (mDisplay == NULL)
    {
        LOGE("failed to connect to wayland display.");
        exit(1);
    }

    struct wl_registry *wl_registry = wl_display_get_registry(mDisplay);
    wl_registry_add_listener(wl_registry, &registry_listener, this);

    wl_display_dispatch(mDisplay);
    wl_display_roundtrip(mDisplay);

    if (mCompositor == NULL)
    {
        LOGE("Compositor(%p), Cannot init.", mCompositor);
        exit(1);
    }

    mSurface = wl_compositor_create_surface(mCompositor);
    if (mSurface == NULL)
    {
        LOGE("cannot create surface ...");
        exit(1);
    }

#ifndef USE_XDG_SHELL
    mShellSurface = wl_shell_get_shell_surface(mShell, mSurface);

    //wl_shell_surface_set_toplevel(mShellSurface);
    wl_shell_surface_set_maximized(mShellSurface, NULL);
    wl_shell_surface_add_listener(mShellSurface, &shell_surface_listener, this);
#else
    mXdgSurface = xdg_wm_base_get_xdg_surface(mWmBase, mSurface);
    xdg_surface_add_listener(mXdgSurface, &xdg_surface_listener, this);

    mXdgToplevel = xdg_surface_get_toplevel(mXdgSurface);
    xdg_toplevel_set_fullscreen(mXdgToplevel, NULL);
    //xdg_toplevel_set_maximized(mXdgToplevel);

    wl_surface_commit(mSurface);

    mWaitForConfigure = true;
    while (mWaitForConfigure)
    {
        wl_display_dispatch(mDisplay);
    }
#endif

    createWindow(mScreenWidth, mScreenHeight);

    start();
}

void WaylandPlatform::run()
{
    while (wl_display_dispatch(mDisplay) != -1)
    {
        /* This space deliberately left blank */
    }
}

void WaylandPlatform::createWindow(int width, int height)
{
__TRACE__
    mRegion = wl_compositor_create_region(mCompositor);

    wl_region_add(mRegion, 0, 0, width, height);
    wl_surface_set_opaque_region(mSurface, mRegion);
    //wl_surface_set_opaque_region(mSurface, NULL);

    mEglWindow = wl_egl_window_create(mSurface, width, height);
    if (mEglWindow == EGL_NO_SURFACE)
    {
        LOGE("cannot create egl window");
        return;
    }
}

void WaylandPlatform::destroyWindow()
{
__TRACE__
    if (mEglWindow)
    {
        wl_egl_window_destroy(mEglWindow);
        mEglWindow = NULL;
    }

    if (mRegion)
    {
        wl_region_destroy(mRegion);
        mRegion = NULL;
    }

#ifndef USE_XDG_SHELL
    if (mShellSurface)
    {
        wl_shell_surface_destroy(mShellSurface);
        mShellSurface = NULL;
    }
#else
    if (mXdgToplevel)
    {
    // TODO
        mXdgToplevel = NULL;
    }

    if (mXdgSurface)
    {
        xdg_surface_destroy(mXdgSurface);
        mXdgSurface = NULL;
    }
#endif

#if defined(CONFIG_SOC_RK3588)
    // TODO Check it. W/A Code . It cause crash.
#else
    if (mSurface)
    {
        wl_surface_destroy(mSurface);
        mSurface = NULL;
    }
#endif
}

WaylandPlatform::~WaylandPlatform()
{
    destroyWindow();

#if defined(CONFIG_SOC_RK3588)
    // TODO Check it. W/A Code . It cause crash.
    if (mSurface)
    {
        wl_surface_destroy(mSurface);
        mSurface = NULL;
    }
#endif

#ifndef USE_XDG_SHELL
    if (mShell)
    {
        wl_shell_destroy(mShell);
        mShell = NULL;
    }
#else
    if (mWmBase)
    {
        xdg_wm_base_destroy(mWmBase);
        mWmBase = NULL;
    }
#endif

    if (mCompositor)
    {
        wl_compositor_destroy(mCompositor);
        mCompositor = NULL;
    }

    if (mDisplay)
    {
        wl_display_disconnect (mDisplay);
        mDisplay = NULL;
    }
}

NativeDisplayType WaylandPlatform::getDisplay()
{
    return (NativeDisplayType)mDisplay;
}

NativeWindowType WaylandPlatform::getWindow()
{
    return (NativeWindowType)mEglWindow;
}
