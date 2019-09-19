// virtual_apk_patch.h
// Created by sisong on 2019-09-11.
#ifndef _virtual_apk_patch_h_
#define _virtual_apk_patch_h_
#ifdef __cplusplus
extern "C" {
#endif
    
    enum TVirtualApkPatch_result{
        kVApkPatch_ok=0,
        kVApkPatch_memError,
        kVApkPatch_baseApkFileError,
        kVApkPatch_apkFileError,
        kVApkPatch_abiError,
        kVApkPatch_tempFileError,
        kVApkPatch_mkNewSoDirError,
        kVApkPatch_catchedUnknowError,
        kVApkPatch_newApkBadError,
        
        kVApkMerge_ok=0,
        kVApkMerge_ApkPathsError=50,
        kVApkMerge_lostNewApkError,
        kVApkMerge_removeHotApkError,
        kVApkMerge_renameNewApkError,
        kVApkMerge_baseApkFileError,
        kVApkMerge_abiError,
        kVApkMerge_newApkFileError,
        kVApkMerge_removeLibFilesError,
        kVApkMerge_renameLibDirError,
        kVApkMerge_moveLibFilesError,
        kVApkMerge_removeNewSoDirError,
        kVApkMerge_catchedUnknowError,
        kVApkPatch_hotApkBadError,
        
        kVApkPatch_patchError_base=100, // kVApkPatch_patchError_base + TPatchResult error
    };
    
#   ifdef __ANDROID__
#       define VIRTUAL_APK_PATCH_EXPORT __attribute__((visibility("default")))
#   else
#       define VIRTUAL_APK_PATCH_EXPORT
#   endif
    
    // patch hot&base + zipDiff  to new
    // baseApk&baseSoDir read only
    // out_newChangedSoDir can NULL or "", meen no need cache .so files
    // if first patch hotApk/hotSoDir can NULL or "", and not need use merge
    int virtual_apk_patch(const char* baseApk,const char* baseSoDir,
                          const char* hotApk,const char* hotSoDir,
                          const char* out_newApk,const char* out_newChangedSoDir,
                          const char* zipDiffPath,const char* arch_abi,
                          int threadNum) VIRTUAL_APK_PATCH_EXPORT;
    
    //merge new(patch result) to hot
    //if return error then need update new version from base version
    int virtual_apk_merge(const char* baseApk,const char* baseSoDir,
                          const char* hotApk,const char* hotSoDir,
                          const char* newApk,const char* newChangedSoDir,
                          const char* arch_abi) VIRTUAL_APK_PATCH_EXPORT;

#ifdef __cplusplus
}
#endif
#endif //_virtual_apk_patch_h_

