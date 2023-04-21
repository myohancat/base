.SUFFIXES : .c .o 

#include tools.mk

Q_		   := @
PKG_CONFIG ?= pkg-config

BASE      := $(shell pwd)
DESTDIR   ?= /tftpboot

LDFLAGS   += -lpthread -ldl
DEFINES   += 

OUT_DIR   := out
TARGET    := test

DEFINES   += -D_GNU_SOURCE
LIBDIRS   +=
LDFLAGS   +=
INCDIRS   := $(BASE)/base
SRCDIRS   := $(BASE)/base
SRCS      := Mutex.cpp
SRCS      += CondVar.cpp
SRCS      += SysTime.cpp
SRCS      += Log.cpp
SRCS      += EventLoop.cpp
SRCS      += Timer.cpp
SRCS      += Task.cpp
SRCS      += MsgQ.cpp
SRCS      += TimerTask.cpp
SRCS      += Util.cpp
SRCS      += NetUtil.cpp
SRCS      += ProcessUtil.cpp

INCDIRS   += $(BASE)
SRCDIRS   += $(BASE)
SRCS      += main.cpp

###############################################################################
# DO NOT MODIFY .......
###############################################################################
APP           := $(OUT_DIR)/$(TARGET)
APP_OBJS      := $(SRCS:%=$(OUT_DIR)/%.o)
APP_CFLAGS    := $(CFLAGS) $(DEFINES)
APP_CXXFLAGS  := $(CXXFLAGS) $(DEFINES)
APP_CXXFLAGS  += $(addprefix -I, $(INCDIRS))
APP_CFLAGS    += $(addprefix -I, $(INCDIRS))
APP_LDFLAGS   := $(addprefix -L, $(LIBDIRS))
APP_LDFLAGS   += $(LDFLAGS)

vpath %.cpp $(SRCDIRS)
vpath %.c $(SRCDIRS)

.PHONY: all clean

all: $(OUT_DIR) app 

app: $(APP_OBJS) 
	@echo "[Linking... $(notdir $(APP))]"
	$(Q_)$(CXX) -o $(APP) $(APP_OBJS) $(APP_CXXFLAGS) $(APP_LDFLAGS)

clean: 
	@echo "[Clean... all objs]"
	$(Q_)rm -rf $(OUT_DIR)

$(OUT_DIR):
	$(Q_)mkdir $(OUT_DIR)

$(OUT_DIR)/%.c.o: %.c
	@echo "[Compile... $(notdir $<)]"
	$(Q_)$(CC) $(APP_CFLAGS) -c $< -o $@

$(OUT_DIR)/%.cpp.o: %.cpp
	@echo "[Compile... $(notdir $<)]"
	$(Q_)$(CXX) $(APP_CXXFLAGS) -c $< -o $@

install:
	@echo "[Install .... $(notdir $(APP))]"
	# $(Q_) install -s --strip-program=$(STRIP) $(APP) $(DESTDIR)$(BIN_DIR)
	scp $(APP) root@192.168.55.1:/home/root/thermal
