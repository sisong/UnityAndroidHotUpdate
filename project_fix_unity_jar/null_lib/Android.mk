LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := null

Src_Files := $(LOCAL_PATH)/null_lib_jni.cpp

LOCAL_SRC_FILES  := $(Src_Files)

LOCAL_LDLIBS     := -landroid
LOCAL_CFLAGS     := -DANDROID_NDK
include $(BUILD_SHARED_LIBRARY)
