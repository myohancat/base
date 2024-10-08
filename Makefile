.SUFFIXES : .c .o 

include config.mk
#include build/tools.mk.rk3588

Q_		   := @
PKG_CONFIG ?= pkg-config

LOCAL_DIR := $(shell pwd)

CXXFLAGS  += -fPIC -Wno-unused-function -Wno-unused-result
CFLAGS    += -fPIC -Wno-unused-function -Wno-unused-result
LDFLAGS   += -lpthread -ldl
DEFINES   += 

OUT_DIR   := out
TARGET    := test

INCDIRS   :=
SRCDIRS   :=
SRCS      :=

include base/base.mk

INCDIRS   += $(LOCAL_DIR)
SRCDIRS   += $(LOCAL_DIR)
SRCS      += Main.cpp

include build/build_app.mk
