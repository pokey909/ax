LOCAL_PATH := $(call my-dir)

# Edit this line
SHELL := PATH=/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin /bin/bash
CURL_PATH := d:/Android/curl-android-ios

####### CURL ###############
include $(CLEAR_VARS)
LOCAL_MODULE := curl-prebuilt
LOCAL_SRC_FILES := $(CURL_PATH)/prebuilt-with-ssl/android/$(TARGET_ARCH_ABI)/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)
############################




include $(CLEAR_VARS)
LOCAL_MODULE    := player

#### Source files ###########################################################

CURLCPP_SRC := $(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)/curlcpp/*.cpp))
AUDIOX_SRC := $(subst $(LOCAL_PATH)/,,$(wildcard $(LOCAL_PATH)/*.cpp))

LOCAL_SRC_FILES := $(AUDIOX_SRC) $(CURLCPP_SRC)
LOCAL_CPP_EXTENSION := .cpp

LOCAL_C_INCLUDES += \
  $(NDK_PATH)/platforms/$(TARGET_PLATFORM)/arch-$(TARGET_ARCH)/usr/include \
  d:/Android/curl-android-ios/prebuilt-with-ssl/android/include

#### Interfaces #############################################################

SWIG_PACKAGE := com.audiox
SWIG_INTERFACES := player.i
SWIG_LANG := cxx

#### Linker #################################################################

LOCAL_SHARED_LIBRARIES := gstreamer_android
LOCAL_LDLIBS := -lz -llog -landroid -Wl,-s
LOCAL_STATIC_LIBRARIES := curl-prebuilt 

#### Compiler flags #########################################################

ifeq ($(TARGET_ARCH),arm)
  LOCAL_CFLAGS := -mfpu=vfp -mfloat-abi=softfp -fno-short-enums
endif
COMMON_CFLAGS := -Werror -DANDROID -g
LOCAL_CFLAGS += $(COMMON_CFLAGS)
LOCAL_CPPFLAGS += -ID:/Android/projects/Gstreamer-Android-example/app/src/main/jni/curlcpp/include -frtti -fexceptions -g



#### Build Player lib #######################################################
include $(LOCAL_PATH)/swig-generate.mk
include $(BUILD_SHARED_LIBRARY)


#### Build GStreamer lib ####################################################
include $(LOCAL_PATH)/build-gstreamer.mk


