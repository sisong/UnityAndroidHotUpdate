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
                                            jstring out_newApk,jstring out_newSoDir,
                                            jstring zipDiffPath,int threadNum){
        const char* cBaseApk     = jenv->GetStringUTFChars(baseApk, NULL);
        const char* cBaseSoDir   = jenv->GetStringUTFChars(baseSoDir, NULL);
        const char* cHotApk      = hotApk?jenv->GetStringUTFChars(hotApk, NULL):0;
        const char* cHotSoDir    = hotSoDir?jenv->GetStringUTFChars(hotSoDir, NULL):0;
        const char* out_cNewApk  = jenv->GetStringUTFChars(out_newApk, NULL);
        const char* out_cNewSoDir= jenv->GetStringUTFChars(out_newSoDir, NULL);
        const char* cZipDiffPath = jenv->GetStringUTFChars(zipDiffPath, NULL);
        const char* arch_abi=TARGET_ARCH_ABI; // "armeabi-v7a" "x86" ...
        int result =virtual_apk_patch(cBaseApk,cBaseSoDir,cHotApk,cHotSoDir,
                                      out_cNewApk,out_cNewSoDir,cZipDiffPath,arch_abi,threadNum);
        jenv->ReleaseStringUTFChars(zipDiffPath,cZipDiffPath);
        jenv->ReleaseStringUTFChars(out_newSoDir,out_cNewSoDir);
        jenv->ReleaseStringUTFChars(out_newApk,out_cNewApk);
        if (hotSoDir) jenv->ReleaseStringUTFChars(hotSoDir,cHotSoDir);
        if (hotApk) jenv->ReleaseStringUTFChars(hotApk,cHotApk);
        jenv->ReleaseStringUTFChars(baseSoDir,cBaseSoDir);
        jenv->ReleaseStringUTFChars(baseApk,cBaseApk);
        return result;
    }
    
    JNIEXPORT int
    Java_com_github_sisong_VirtualApk_merge(JNIEnv* jenv,jobject jobj,
                                            jstring baseApk,jstring baseSoDir,
                                            jstring hotApk,jstring hotSoDir,
                                            jstring newApk,jstring newSoDir){
        const char* cBaseApk     = jenv->GetStringUTFChars(baseApk, NULL);
        const char* cBaseSoDir   = jenv->GetStringUTFChars(baseSoDir, NULL);
        const char* cHotApk      = jenv->GetStringUTFChars(hotApk, NULL):0;
        const char* cHotSoDir    = jenv->GetStringUTFChars(hotSoDir, NULL):0;
        const char* cNewApk      = jenv->GetStringUTFChars(newApk, NULL);
        const char* cNewSoDir    = jenv->GetStringUTFChars(newSoDir, NULL);
        const char* arch_abi=TARGET_ARCH_ABI; // "armeabi-v7a" "x86" ...
        int result =virtual_apk_merge(cBaseApk,cBaseSoDir,cHotApk,cHotSoDir,
                                      out_cNewApk,out_cNewSoDir,arch_abi);
        jenv->ReleaseStringUTFChars(newSoDir,cNewSoDir);
        jenv->ReleaseStringUTFChars(newApk,cNewApk);
        jenv->ReleaseStringUTFChars(hotSoDir,cHotSoDir);
        jenv->ReleaseStringUTFChars(hotApk,cHotApk);
        jenv->ReleaseStringUTFChars(baseSoDir,cBaseSoDir);
        jenv->ReleaseStringUTFChars(baseApk,cBaseApk);
        return result;
    }

#ifdef __cplusplus
}
#endif
