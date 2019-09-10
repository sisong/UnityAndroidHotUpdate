LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := hotcachelib

HDP_PATH  := $(LOCAL_PATH)/../../../ApkDiffPatch/HDiffPatch
Hdp_Files := $(HDP_PATH)/file_for_patch.c \
             $(HDP_PATH)/libHDiffPatch/HPatch/patch.c

ADP_PATH  := $(LOCAL_PATH)/../../../ApkDiffPatch/src/patch
Adp_Files := $(ADP_PATH)/Zipper.cpp

Src_Files := $(LOCAL_PATH)/hot_cache_lib_jni.cpp \
             $(LOCAL_PATH)/../../src/hot_cache_lib.cpp

LOCAL_SRC_FILES  := $(Src_Files) $(Hdp_Files) $(Adp_Files)

LOCAL_LDLIBS     := -llog -landroid -lz
LOCAL_CFLAGS     := -DANDROID_NDK -DTARGET_ARCH_ABI=\"$(TARGET_ARCH_ABI)\" -D_IS_USED_MULTITHREAD=0
include $(BUILD_SHARED_LIBRARY)

