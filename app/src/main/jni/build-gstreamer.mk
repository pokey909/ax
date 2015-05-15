# GSTREAMER_ROOT_ANDROID := /Android/gstreamer-1.0-android-armv7-debug-1.4.5
GSTREAMER_ROOT_ANDROID := /Android/gstreamer-1.0-android-$(TARGET_ARCH_ABI)-debug-1.4.5

ifndef GSTREAMER_ROOT
ifndef GSTREAMER_ROOT_ANDROID
$(error GSTREAMER_ROOT_ANDROID is not defined!)
endif
GSTREAMER_ROOT        := $(GSTREAMER_ROOT_ANDROID)
endif
GSTREAMER_NDK_BUILD_PATH  := $(GSTREAMER_ROOT_ANDROID)/share/gst-android/ndk-build


include $(GSTREAMER_NDK_BUILD_PATH)/plugins.mk
GSTREAMER_PLUGINS         := $(GSTREAMER_PLUGINS_CORE) $(GSTREAMER_PLUGINS_PLAYBACK) $(GSTREAMER_PLUGINS_CODECS) $(GSTREAMER_PLUGINS_NET) $(GSTREAMER_PLUGINS_SYS) $(GSTREAMER_PLUGINS_CODECS_RESTRICTED)

GSTREAMER_PLUGINS := coreelements appsrc audioconvert audioresample typefindfunctions volume autodetect opensles playback fragmented audioparsers androidmedia soup id3tag id3demux
#G_IO_MODULES              := gnutls
GSTREAMER_EXTRA_DEPS      := glib-2.0 nettle gstreamer-pbutils-1.0
LOCAL_C_INCLUDES += -I$(LOCAL_PATH)curlpp/include
LOCAL_CFLAGS += -Wno-error,-Wdeprecated-register
include $(GSTREAMER_NDK_BUILD_PATH)/gstreamer-1.0.mk
