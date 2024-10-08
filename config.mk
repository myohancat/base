# base library
WITH_HW:=yes
WITH_WIFI:=no
WITH_BLUETOOTH:=no
WITH_RENDERABLE:=no
WITH_UI:=no
WITH_MULTIMEDIA:=no
WITH_INPUT:=no
WITH_CLI:=yes

# extern library
WITH_WEBSERVER:=no
WITH_XML:=no
WITH_JSON:=no

ifeq ($(WITH_WIFI),yes)
DEFINES += -DCONFIG_SUPPORT_WIFI
endif

ifeq ($(WITH_BLUETOOTH),yes)
DEFINES += -DCONFIG_SUPPORT_BLUETOOTH
endif

ifeq ($(WITH_MULTIMEDIA),yes)
DEFINES += -DCONFIG_SUPPORT_MULTIMEDIA
endif

ifeq ($(WITH_UI),yes)
DEFINES += -DCONFIG_SUPPORT_UI
WITH_INPUT:=yes
endif

ifeq ($(WITH_CLI),yes)
DEFINES += -DCONFIG_SUPPORT_CLI
endif

ifneq ($(or $(findstring $(WITH_UI),yes), $(findstring $(WITH_MULTIMEDIA),yes)),)
WITH_RENDERABLE:=yes
endif
