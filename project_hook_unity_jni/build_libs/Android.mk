LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := hookunity

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
					../xHook/libxhook/jni 

xHook_Files := xhook.c xh_core.c xh_elf.c xh_log.c xh_util.c xh_version.c
Lib_Files := $(foreach f, $(xHook_Files), $(shell find $(LOCAL_PATH)/../xHook/libxhook/jni -name $f))

Src_Files := ../src/hook_unity_jni.cpp \
				 ../src/hook_unity.cpp

Patch_Source_Files := $(Src_Files) $(Lib_Files)
Patch_Source_Files := $(Patch_Source_Files:$(LOCAL_PATH)/%=%)

LOCAL_SRC_FILES  :=  $(Patch_Source_Files)

$(info compile files-------------------->)
$(info $(LOCAL_SRC_FILES))

LOCAL_LDLIBS     := -llog -landroid
LOCAL_CFLAGS     := -DANDROID_NDK
include $(BUILD_SHARED_LIBRARY)
