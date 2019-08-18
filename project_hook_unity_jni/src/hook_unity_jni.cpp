// hook_unity_jni.cpp
// Created by sisong on 2019-08-14.

#include <jni.h>
#include "hook_unity.h"
#ifdef __cplusplus
extern "C" {
#endif
    
    JNIEXPORT void
    Java_com_github_sisong_HookUnity_doHook(JNIEnv* jenv,jobject jobj,
                                            jstring newApkPath){
        const char* cstrPath = jenv->GetStringUTFChars(newApkPath, NULL);
        hook_unity_doHook(cstrPath);
        jenv->ReleaseStringUTFChars(newApkPath,cstrPath);
    }

#ifdef __cplusplus
}
#endif
