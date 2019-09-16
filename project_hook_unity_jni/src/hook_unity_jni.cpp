// hook_unity_jni.cpp
// Created by sisong on 2019-08-14.

#include <jni.h>
#include "hook_unity.h"
#ifdef __cplusplus
extern "C" {
#endif
    
    JNIEXPORT void
    Java_com_github_sisong_HotUnity_doHot(JNIEnv* jenv,jobject jobj,
                                          jstring baseApkPath,jstring baseSoDir,
                                          jstring hotApkPath,jstring hotSoDir){
        const char* cBaseApkPath= jenv->GetStringUTFChars(baseApkPath, NULL);
        const char* cBaseSoDir  = jenv->GetStringUTFChars(baseSoDir, NULL);
        const char* cHotApkPath = jenv->GetStringUTFChars(hotApkPath, NULL);
        const char* cHotSoDir= jenv->GetStringUTFChars(hotSoDir, NULL);
        hook_unity_doHook(cBaseApkPath,cBaseSoDir,cHotApkPath,cHotSoDir);
        jenv->ReleaseStringUTFChars(hotSoDir,cHotSoDir);
        jenv->ReleaseStringUTFChars(hotApkPath,cHotApkPath);
        jenv->ReleaseStringUTFChars(baseSoDir,cBaseSoDir);
        jenv->ReleaseStringUTFChars(baseApkPath,cBaseApkPath);
    }

#ifdef __cplusplus
}
#endif
