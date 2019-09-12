// virtual_apk_patch.h
// Created by sisong on 2019-09-11.
#ifndef _virtual_apk_patch_h_
#define _virtual_apk_patch_h_
#ifdef __cplusplus
extern "C" {
#endif
    
    enum TVirtualApkPatch_result{
        kVApkPatch_ok=0,
        kVApkPatch_apkFileError,
        kVApkPatch_abiError,
        
    };
    
#   ifdef __ANDROID__
#       define VIRTUAL_APK_PATCH_EXPORT __attribute__((visibility("default")))
#   else
#       define VIRTUAL_APK_PATCH_EXPORT
#   endif
    //baseApk&baseSoDir read only
    int virtual_apk_patch(const char* baseApk,const char* baseSoDir,
                          const char* hotApk,const char* hotSoDir,
                          const char* diffData,const char* arch_abi,
                          bool  isCacheHotSo) VIRTUAL_APK_PATCH_EXPORT;
#ifdef __cplusplus
}
#endif
#endif //_virtual_apk_patch_h_

