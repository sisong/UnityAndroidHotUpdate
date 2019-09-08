LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := hotunity

XHOOK_PATH := $(LOCAL_PATH)/../xHook/libxhook/jni
xHook_Files := \
        $(XHOOK_PATH)/xh_core.c \
        $(XHOOK_PATH)/xh_elf.c  \
        $(XHOOK_PATH)/xh_log.c  \
        $(XHOOK_PATH)/xh_util.c \
        $(XHOOK_PATH)/xh_version.c

Src_Files := $(LOCAL_PATH)/../src/hook_unity_jni.cpp \
             $(LOCAL_PATH)/../src/hook_unity.cpp

LOCAL_SRC_FILES  := $(Src_Files) $(xHook_Files)

LOCAL_LDLIBS     := -llog -landroid
LOCAL_CFLAGS     := -DANDROID_NDK
include $(BUILD_SHARED_LIBRARY)
