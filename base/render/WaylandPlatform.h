/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __WAYLAND_PLATFROM_H_
#define __WAYLAND_PLATFROM_H_

#include "Platform.h"
#include "Task.h"

#include <memory>

#include <EGL/egl.h>
#include <EGL/eglplatform.h>

#include <wayland-client.h>
#include <wayland-server.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h> // Wayland EGL MUST be included before EGL headers

#ifdef USE_XDG_SHELL
#include "xdg-shell-client-protocol.h"
#endif

class WaylandPlatform : public IPlatform, Task
{
public:
    WaylandPlatform();
    ~WaylandPlatform();

    NativeDisplayType getDisplay();
    NativeWindowType  getWindow();

    int getScreenWidth();
    int getScreenHeight();

private:
    void run();

private:
    struct wl_display*       mDisplay = NULL;
    struct wl_compositor*    mCompositor = NULL;
    struct wl_surface*       mSurface = NULL;
    struct wl_egl_window*    mEglWindow = NULL;
    struct wl_region*        mRegion = NULL;
#ifndef USE_XDG_SHELL
    struct wl_shell*         mShell = NULL;
    struct wl_shell_surface* mShellSurface = NULL;
#else
    struct xdg_wm_base*      mWmBase = NULL;
    struct xdg_surface*      mXdgSurface = NULL;
    struct xdg_toplevel*     mXdgToplevel = NULL;
    bool                     mWaitForConfigure = false;
#endif

    struct wl_shm*           mShm = NULL;

    int    mScreenWidth;
    int    mScreenHeight;

private:
    void createWindow(int width, int height);
    void destroyWindow();

private:
    static struct wl_registry_listener      registry_listener;
    static struct wl_output_listener        output_listener;
    static struct wl_seat_listener          seat_listener;

#ifndef USE_XDG_SHELL
    static struct wl_shell_surface_listener shell_surface_listener;
#else
    static struct xdg_wm_base_listener   wm_base_listener;
    static struct xdg_surface_listener   xdg_surface_listener;
#endif

private:
    static void global_registry_handler(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version);
    static void global_registry_remover(void* data, struct wl_registry* registry, uint32_t id);

    static void output_handle_geometry(void* data, struct wl_output* wl_output, int x, int y, int physical_width, int physical_height,
                                       int subpixel, const char* make, const char* model, int32_t transform);
    static void output_handle_done(void* data, struct wl_output* wl_output);
    static void output_handle_scale(void* data, struct wl_output* wl_output, int32_t scale);
    static void output_handle_mode(void* data, struct wl_output* wl_output, uint32_t flags, int width, int height, int refresh);

#ifndef USE_XDG_SHELL
    static void handle_ping(void *_data, struct wl_shell_surface *shell_surface, uint32_t serial);
    static void handle_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height);
    static void handle_popup_done(void *data, struct wl_shell_surface *shell_surface);
#else
    static void xdg_wm_base_ping(void* data, struct xdg_wm_base* shell, uint32_t serial);
    static void handle_surface_configure(void* data, struct xdg_surface* surface, uint32_t serial);
#endif
};

inline int WaylandPlatform::getScreenWidth()
{
    return mScreenWidth;
}

inline int WaylandPlatform::getScreenHeight()
{
    return mScreenHeight;
}

#endif /* __WAYLAND_PLATFROM_H_ */
