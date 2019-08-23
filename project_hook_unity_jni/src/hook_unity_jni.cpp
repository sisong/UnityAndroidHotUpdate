// hook_unity_jni.cpp
// Created by sisong on 2019-08-14.

#include <jni.h>
#include "hook_unity.h"
#ifdef __cplusplus
extern "C" {
#endif
    
    JNIEXPORT void
    Java_com_github_sisong_HotUnity_doHot(JNIEnv* jenv,jobject jobj,
                                          jstring apkPath,jstring libDir,
                                          jstring newApkPath,jstring libCacheDir){
        const char* cApkPath    = jenv->GetStringUTFChars(apkPath, NULL);
        const char* clibDir     = jenv->GetStringUTFChars(libDir, NULL);
        const char* cNewApkPath = jenv->GetStringUTFChars(newApkPath, NULL);
        const char* clibCacheDir= jenv->GetStringUTFChars(libCacheDir, NULL);
        hook_unity_doHook(cApkPath,clibDir,cNewApkPath,clibCacheDir);
        jenv->ReleaseStringUTFChars(libCacheDir,clibCacheDir);
        jenv->ReleaseStringUTFChars(newApkPath,cNewApkPath);
        jenv->ReleaseStringUTFChars(libDir,clibDir);
        jenv->ReleaseStringUTFChars(apkPath,cApkPath);
    }

#ifdef __cplusplus
}
#endif
