// hook_unity_jni.cpp
// Created by sisong on 2019-08-14.

#include <jni.h>
#include "hook_unity.h"
#ifdef __cplusplus
extern "C" {
#endif
    
    JNIEXPORT void
    Java_com_github_sisong_HookUnity_doHook(JNIEnv* jenv,jobject jobj,
                                             jstring apkPath,jstring newApkPath){
        const char* cPath = jenv->GetStringUTFChars(apkPath, NULL);
        const char* cNewPath = jenv->GetStringUTFChars(newApkPath, NULL);
        hook_unity_doHook(cPath,cNewPath);
        jenv->ReleaseStringUTFChars(newApkPath,cNewPath);
        jenv->ReleaseStringUTFChars(apkPath,cPath);
    }

#ifdef __cplusplus
}
#endif
