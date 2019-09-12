// virtual_apk_patch_jni.cpp
// Created by sisong on 2019-08-14.

#include <jni.h>
#include "virtual_apk_patch.h"
#ifdef __cplusplus
extern "C" {
#endif
    
    JNIEXPORT int
    Java_com_github_sisong_VirtualApk_patch(JNIEnv* jenv,jobject jobj,
                                            jstring baseApk,jstring baseSoDir,
                                            jstring hotApk,jstring hotSoDir,
                                            jstring diffData,jbool isCacheHotSo){
        const char* cBaseApk    = jenv->GetStringUTFChars(baseApk, NULL);
        const char* cBaseSoDir  = jenv->GetStringUTFChars(baseSoDir, NULL);
        const char* cHotApk     = jenv->GetStringUTFChars(hotApk, NULL);
        const char* cHotSoDir   = jenv->GetStringUTFChars(hotSoDir, NULL);
        const char* cDiffData   = jenv->GetStringUTFChars(diffData, NULL);
        const char* arch_abi=TARGET_ARCH_ABI; // "armeabi-v7a" "x86" ...
        int result =virtual_apk_patch(cBaseApk,cBaseSoDir,cHotApk,cHotSoDir,
                                      cDiffData,arch_abi,isCacheHotSo);
        jenv->ReleaseStringUTFChars(diffData,cDiffData);
        jenv->ReleaseStringUTFChars(hotSoDir,cHotSoDir);
        jenv->ReleaseStringUTFChars(hotApk,cHotApk);
        jenv->ReleaseStringUTFChars(baseSoDir,cBaseSoDir);
        jenv->ReleaseStringUTFChars(baseApk,cBaseApk);
        return result;
    }

#ifdef __cplusplus
}
#endif
