include $(CLEAR_VARS)

GSTREAMER_ROOT_ANDROID := /Android/gstreamer-1.0-android-armv7-debug-1.4.5

ifndef GSTREAMER_ROOT
ifndef GSTREAMER_ROOT_ANDROID
$(error GSTREAMER_ROOT_ANDROID is not defined!)
endif
GSTREAMER_ROOT        := $(GSTREAMER_ROOT_ANDROID)
endif
GSTREAMER_NDK_BUILD_PATH  := $(GSTREAMER_ROOT_ANDROID)/share/gst-android/ndk-build

GSTREAMER_PLUGINS := coreelements audioconvert audioresample typefindfunctions volume autodetect opensles playback fragmented audioparsers androidmedia soup id3tag id3demux
G_IO_MODULES              := gnutls
GSTREAMER_EXTRA_DEPS      := glib-2.0 nettle gstreamer-pbutils-1.0
LOCAL_C_INCLUDES += -I$(LOCAL_PATH)curlpp/include
include $(GSTREAMER_NDK_BUILD_PATH)/gstreamer-1.0.mk
