BASE_DIR  ?= ${LOCAL_DIR}/base

DEFINES   += -DCONFIG_SOC_$(CHIP_NAME)
DEFINES   += -D_GNU_SOURCE
LIBDIRS   +=
LDFLAGS   +=
INCDIRS   += $(BASE_DIR)/common
SRCDIRS   += $(BASE_DIR)/common
SRCS      += MainLoop.cpp
SRCS      += EventQueue.cpp
SRCS      += Timer.cpp
SRCS      += Task.cpp
SRCS      += Mutex.cpp
SRCS      += CondVar.cpp
SRCS      += Log.cpp
SRCS      += TimerTask.cpp
SRCS      += SysTime.cpp
SRCS      += Pipe.cpp
SRCS      += MsgQ.cpp

INCDIRS   += $(BASE_DIR)/utils
SRCDIRS   += $(BASE_DIR)/utils
SRCS      += NetUtil.cpp
SRCS      += ProcessUtil.cpp
SRCS      += FileUtil.cpp
SRCS      += Util.cpp

INCDIRS   += $(BASE_DIR)/net
SRCDIRS   += $(BASE_DIR)/net
SRCS      += HttpServer.cpp
SRCS      += HttpClient.cpp
SRCS      += TcpServer.cpp

INCDIRS   += $(BASE_DIR)/external/mongoose
SRCDIRS   += $(BASE_DIR)/external/mongoose
SRCS      += mongoose.c

INCDIRS   += $(BASE_DIR)/input
SRCDIRS   += $(BASE_DIR)/input
SRCS      += InputManager.cpp
SRCS      += PointerTracker.cpp
SRCS      += HAL_Input.cpp

INCDIRS   += $(BASE_DIR)/system
SRCDIRS   += $(BASE_DIR)/system
SRCS      += NetlinkManager.cpp
SRCS      += UsbHotplugManager.cpp
SRCS      += UsbStorageManager.cpp

INCDIRS   += $(BASE_DIR)/hw
SRCDIRS   += $(BASE_DIR)/hw
SRCS      += GPIO.cpp
SRCS      += IRQ.cpp
SRCS      += I2C.cpp
SRCS      += UART.cpp
SRCS      += BUTTON.cpp
SRCS      += LED.cpp
SRCS      += RGB_LED.cpp
SRCS      += EEPROM.cpp

ifeq ($(WITH_RENDERABLE),yes)
ifeq ($(WITH_DRM),yes)
DEFINES   += -DUSE_DRM_PLATFORM
CFLAGS    += $(shell $(PKG_CONFIG) --cflags libdrm)
CXXFLAGS  += $(shell $(PKG_CONFIG) --cflags libdrm)
LDFLAGS   += $(shell $(PKG_CONFIG) --libs libdrm)
DEFINES   += -DGL_VERSION_1_5 -DEGL_EGLEXT_PROTOTYPES
LDFLAGS   += -lEGL
LDFLAGS   += -lGLESv2
LDFLAGS   += -lmali
SRCS      += DrmPlatform.cpp
else
DEFINES   += -DEGL_API_FB -DLINUX -DWL_EGL_PLATFORM
LDFLAGS   += $(shell $(PKG_CONFIG) --libs wayland-client)
LDFLAGS   += $(shell $(PKG_CONFIG) --libs wayland-egl)
LDFLAGS   += $(shell $(PKG_CONFIG) --libs wayland-cursor)
LDFLAGS   += $(shell $(PKG_CONFIG) --libs libdrm)
DEFINES   += -DEGL_EGLEXT_PROTOTYPES
LDFLAGS   += -lEGL
DEFINES   += -DGL_VERSION_1_5
LDFLAGS   += -lGLESv2
LDFLAGS   += -lmali
DEFINES   += -DUSE_XDG_SHELL
INCDIRS   += $(BASE_DIR)/render/xdg_proto
SRCDIRS   += $(BASE_DIR)/render/xdg_proto
SRCS      += xdg-shell-protocol.c
SRCS      += WaylandPlatform.cpp
endif

INCDIRS   += $(BASE_DIR)/render
SRCDIRS   += $(BASE_DIR)/render
SRCS      += ShaderUtil.cpp
SRCS      += EGLHelper.cpp
SRCS      += FrameBuffer.cpp
SRCS      += RenderService.cpp
SRCS      += DisplayHotplugManager.cpp
SRCS      += OnDisplayRenderer.cpp

INCDIRS   += $(BASE_DIR)/render/renderer
SRCDIRS   += $(BASE_DIR)/render/renderer
SRCS      += EGLImageRenderer.cpp
SRCS      += RawNV12Renderer.cpp
SRCS      += RawRGBARenderer.cpp
endif

ifeq ($(WITH_UI), yes)
INCDIRS   += $(BASE_DIR)/ui
SRCDIRS   += $(BASE_DIR)/ui
SRCS      += Point.cpp
SRCS      += Size.cpp
SRCS      += Rectangle.cpp
SRCS      += Color.cpp
SRCS      += Font.cpp
SRCS      += Image.cpp
SRCS      += Animation.cpp
SRCS      += Canvas.cpp
SRCS      += Window.cpp
SRCS      += Page.cpp
SRCS      += WindowRenderer.cpp
LDFLAGS   += -lrga

INCDIRS   += $(BASE_DIR)/ui/lib/skia
INCDIRS   += $(BASE_DIR)/ui/lib/skia/include
INCDIRS   += $(BASE_DIR)/ui/lib/skia/include/core
INCDIRS   += $(BASE_DIR)/ui/lib/skia/include/gpu
INCDIRS   += $(BASE_DIR)/ui/lib/skia/include/config
CXXFLAGS  += -std=c++1z
LIBDIRS   += $(BASE_DIR)/ui/lib/skia/lib/$(ARCH)
LDFLAGS   += -lskia
endif

ifeq ($(WITH_CLI),yes)
INCDIRS   += $(BASE_DIR)/cli
SRCDIRS   += $(BASE_DIR)/cli
SRCS      += CLI.cpp
SRCS      += vkey.cpp
endif

ifeq ($(WITH_WIFI),yes)
INCDIRS   += $(BASE_DIR)/wifi
SRCDIRS   += $(BASE_DIR)/wifi
SRCS      += WifiManager.cpp

DEFINES   += -DCONFIG_CTRL_IFACE_UNIX -DCONFIG_CTRL_IFACE
INCDIRS   += $(BASE_DIR)/wifi/wpa_ctrl
SRCDIRS   += $(BASE_DIR)/wifi/wpa_ctrl
#SRCS      += os_unix.c
SRCS      += wpa_ctrl.c
endif

ifeq ($(WITH_BLUETOOTH),yes)
INCDIRS   += $(BASE_DIR)/bluetooth
SRCDIRS   += $(BASE_DIR)/bluetooth
SRCS      += BluetoothManager.cpp
SRCS      += agent.c

INCDIRS   += $(PKG_CONFIG_SYSROOT_DIR)/usr/include/dbus-1.0
INCDIRS   += $(PKG_CONFIG_SYSROOT_DIR)/usr/lib/dbus-1.0/include
INCDIRS   += $(PKG_CONFIG_SYSROOT_DIR)/usr/include/glib-2.0
INCDIRS   += $(PKG_CONFIG_SYSROOT_DIR)/usr/lib/glib-2.0/include
INCDIRS   += $(BASE_DIR)/bluetooth/gdbus
SRCDIRS   += $(BASE_DIR)/bluetooth/gdbus
SRCS      += object.c
SRCS      += polkit.c
SRCS      += client.c
SRCS      += mainloop.c
SRCS      += watch.c
LDFLAGS   += -ldbus-1 -lglib-2.0
endif

ifeq ($(WITH_MULTIMEDIA),yes)
INCDIRS   += $(BASE_DIR)/multimedia
SRCDIRS   += $(BASE_DIR)/multimedia
CXXFLAGS  += $(shell $(PKG_CONFIG) --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-allocators-1.0 gstreamer-video-1.0 gstreamer-gl-1.0)
LDFLAGS   += $(shell $(PKG_CONFIG) --libs gstreamer-1.0 gstreamer-app-1.0 gstreamer-allocators-1.0 gstreamer-video-1.0 gstreamer-gl-1.0)
SRCS      += GstHelper.cpp
SRCS      += GstAppsinkRenderable.cpp
SRCS      += FilePlayer.cpp
endif

ifeq ($(WITH_XML),yes)
INCDIRS   += $(BASE_DIR)/external/ezxml
SRCDIRS   += $(BASE_DIR)/external/ezxml
SRCS      += ezxml.c
endif

ifeq ($(WITH_JSON),yes)
INCDIRS   += $(BASE_DIR)/external/jsoncpp
SRCDIRS   += $(BASE_DIR)/external/jsoncpp
SRCS      += jsoncpp.cpp
endif
