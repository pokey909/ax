LOCAL_PATH := $(call my-dir)

# Edit this line
GSTREAMER_ROOT_ANDROID := /Android/gstreamer-1.0-android-armv7-debug-1.4.5

SHELL := PATH=/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin /bin/bash

include $(CLEAR_VARS)
LOCAL_MODULE := curl-prebuilt
LOCAL_SRC_FILES := \
  d:/Android/curl-android-ios/prebuilt-with-ssl/android/$(TARGET_ARCH_ABI)/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)
################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE    := player
CURLCPP_SRC := $(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)/curlcpp/*.cpp))
LOCAL_SRC_FILES := Downloader.cpp player.cpp Stream.cpp $(CURLCPP_SRC)
LOCAL_SHARED_LIBRARIES := gstreamer_android
LOCAL_CPPFLAGS += -ID:/Android/projects/Gstreamer-Android-example/app/src/main/jni/curlcpp/include -frtti -fexceptions
LOCAL_SRC_FILES += player_jni.cpp
MY_JNI_WRAP := $(LOCAL_PATH)/player_jni.cpp

LOCAL_STATIC_LIBRARIES := curl-prebuilt 
COMMON_CFLAGS := -Werror -DANDROID 

ifeq ($(TARGET_ARCH),arm)
  LOCAL_CFLAGS := -mfpu=vfp -mfloat-abi=softfp -fno-short-enums
endif

LOCAL_CFLAGS += $(COMMON_CFLAGS)
LOCAL_LDLIBS := -lz -llog -landroid -Wl,-s
LOCAL_C_INCLUDES += \
  $(NDK_PATH)/platforms/$(TARGET_PLATFORM)/arch-$(TARGET_ARCH)/usr/include \
  d:/Android/curl-android-ios/prebuilt-with-ssl/android/include


$(MY_JNI_WRAP):
	echo "in myjni target"
	/Android/swigwin-3.0.5/swig -c++ -java -package com.audiox -o $(MY_JNI_WRAP) $(LOCAL_PATH)/player.i
.PHONY: $(MY_JNI_WRAP)

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

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
