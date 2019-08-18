// hook_unity_jni.cpp
// Created by sisong on 2019-08-14.

#include <jni.h>
#include "hook_unity.h"
#ifdef __cplusplus
extern "C" {
#endif
    
    JNIEXPORT void
    Java_com_github_sisong_HookUnity_doHook(JNIEnv* jenv,jobject jobj,
                                            jstring apkPath,jstring soDir,
                                            jstring newApkPath,jstring soCacheDir){
        const char* cApkPath    = jenv->GetStringUTFChars(apkPath, NULL);
        const char* cSoDir      = jenv->GetStringUTFChars(soDir, NULL);
        const char* cNewApkPath = jenv->GetStringUTFChars(newApkPath, NULL);
        const char* cSoCacheDir = jenv->GetStringUTFChars(soCacheDir, NULL);
        hook_unity_doHook(cApkPath,cSoDir,cNewApkPath,cSoCacheDir);
        jenv->ReleaseStringUTFChars(soCacheDir,cSoCacheDir);
        jenv->ReleaseStringUTFChars(newApkPath,cNewApkPath);
        jenv->ReleaseStringUTFChars(soDir,cSoDir);
        jenv->ReleaseStringUTFChars(apkPath,cApkPath);
    }

#ifdef __cplusplus
}
#endif
