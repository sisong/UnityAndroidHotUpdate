// apk_patch_jni.cpp
// Created by sisong on 2019-08-22.
#include <jni.h>
#include "../../src/hot_cache_lib.h"
#ifdef __cplusplus
extern "C" {
#endif
    
    JNIEXPORT int
    Java_com_github_sisong_HotCacheLib_cache(JNIEnv* jenv,jobject jobj,
                                             jstring oldApkPath,jstring newApkPath,
                                             jstring cacheLibFilesDir){
        const char* cOldApkPath       = jenv->GetStringUTFChars(oldApkPath, NULL);
        const char* cNewApkPath       = jenv->GetStringUTFChars(newApkPath, NULL);
        const char* cCacheLibFilesDir = jenv->GetStringUTFChars(cacheLibFilesDir, NULL);
        const char* arch_abi=TARGET_ARCH_ABI; // "armeabi-v7a" "x86" ...
        int result=hot_cache_lib(arch_abi,cOldApkPath,cNewApkPath,cCacheLibFilesDir);
        jenv->ReleaseStringUTFChars(cacheLibFilesDir,cCacheLibFilesDir);
        jenv->ReleaseStringUTFChars(newApkPath,cNewApkPath);
        jenv->ReleaseStringUTFChars(oldApkPath,cOldApkPath);
        return result;
    }
    
#ifdef __cplusplus
}
#endif
