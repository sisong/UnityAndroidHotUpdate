LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := hotunity

ADP_PATH   := $(LOCAL_PATH)/../../ApkDiffPatch
 
Lzma_Files := $(ADP_PATH)/lzma/C/LzmaDec.c 

ZLIB_PATH  := $(ADP_PATH)/zlib1.2.11
Zlib_Files := $(ZLIB_PATH)/crc32.c    \
              $(ZLIB_PATH)/deflate.c  \
              $(ZLIB_PATH)/inflate.c  \
              $(ZLIB_PATH)/zutil.c    \
              $(ZLIB_PATH)/adler32.c  \
              $(ZLIB_PATH)/trees.c    \
              $(ZLIB_PATH)/inftrees.c \
              $(ZLIB_PATH)/inffast.c

HDP_PATH  := $(ADP_PATH)/HDiffPatch
Hdp_Files := $(HDP_PATH)/file_for_patch.c \
             $(HDP_PATH)/libHDiffPatch/HPatch/patch.c \
             $(HDP_PATH)/libParallel/parallel_import.cpp \
             $(HDP_PATH)/libParallel/parallel_channel.cpp

ADP_PATCH_PATH := $(ADP_PATH)/src/patch
Adp_Files := $(ADP_PATCH_PATH)/NewStream.cpp \
             $(ADP_PATCH_PATH)/OldStream.cpp \
             $(ADP_PATCH_PATH)/Patcher.cpp \
             $(ADP_PATCH_PATH)/ZipDiffData.cpp \
             $(ADP_PATCH_PATH)/Zipper.cpp

XHOOK_PATH := $(LOCAL_PATH)/../xHook/libxhook/jni
xHook_Files := \
        $(XHOOK_PATH)/xh_core.c \
        $(XHOOK_PATH)/xh_elf.c  \
        $(XHOOK_PATH)/xh_log.c  \
        $(XHOOK_PATH)/xh_util.c \
        $(XHOOK_PATH)/xh_version.c


Src_Files := $(LOCAL_PATH)/../src/hook_unity.cpp \
             $(LOCAL_PATH)/../src/hook_unity_jni.cpp \
             $(LOCAL_PATH)/../src/virtual_apk_patch_jni.cpp \
             $(LOCAL_PATH)/../../VirtualApkPatch/patch/virtual_apk_patch.cpp


LOCAL_SRC_FILES  := $(Src_Files) $(xHook_Files) $(Lzma_Files) $(Zlib_Files) $(Hdp_Files) $(Adp_Files)

LOCAL_LDLIBS     := -llog -landroid
LOCAL_CFLAGS     := -Os -DANDROID_NDK  -DTARGET_ARCH_ABI=\"$(TARGET_ARCH_ABI)\"  \
                    -D_7ZIP_ST -D_IS_USED_MULTITHREAD=1 -D_IS_USED_PTHREAD=1 \
                    -D_IS_NEED_FIXED_ZLIB_VERSION=1 -D_IS_NEED_VIRTUAL_ZIP=1

include $(BUILD_SHARED_LIBRARY)
