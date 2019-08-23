LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := hotcachelib

ZLIB_PATH  := $(LOCAL_PATH)/../../../ApkDiffPatch/zlib1.2.11
Zlib_Files := $(ZLIB_PATH)/crc32.c    \
              $(ZLIB_PATH)/deflate.c  \
              $(ZLIB_PATH)/inflate.c  \
              $(ZLIB_PATH)/zutil.c    \
              $(ZLIB_PATH)/adler32.c  \
              $(ZLIB_PATH)/trees.c    \
              $(ZLIB_PATH)/inftrees.c \
              $(ZLIB_PATH)/inffast.c

HDP_PATH  := $(LOCAL_PATH)/../../../ApkDiffPatch/HDiffPatch
Hdp_Files := $(HDP_PATH)/file_for_patch.c \
             $(HDP_PATH)/libHDiffPatch/HPatch/patch.c

ADP_PATH  := $(LOCAL_PATH)/../../../ApkDiffPatch/src/patch
Adp_Files := $(ADP_PATH)/Zipper.cpp

Src_Files := $(LOCAL_PATH)/hot_cache_lib_jni.cpp \
             $(LOCAL_PATH)/../../src/hot_cache_lib.cpp

LOCAL_SRC_FILES  := $(Src_Files) $(Zlib_Files) $(Hdp_Files) $(Adp_Files)

LOCAL_LDLIBS     := -llog -landroid
LOCAL_CFLAGS     := -DANDROID_NDK -DTARGET_ARCH_ABI=\"$(TARGET_ARCH_ABI)\" -D_IS_USED_MULTITHREAD=0
include $(BUILD_SHARED_LIBRARY)

