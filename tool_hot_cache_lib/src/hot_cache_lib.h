// hot_cache_lib.h
// Created by sisong on 2019-08-20.
#ifndef _hot_cache_lib_h_
#define _hot_cache_lib_h_
#ifdef __cplusplus
extern "C" {
#endif
    
    enum hot_cache_lib_result_t{
        kCacheLib_ok =0,
        kCacheLib_inputFileError,
        kCacheLib_abiError,
        kCacheLib_mkDirError,
        kCacheLib_outputFileError,
        kCacheLib_decompressError,
    };
    
#   ifdef __ANDROID__
#       define HOT_CACHE_EXPORT __attribute__((visibility("default")))
#   else
#       define HOT_CACHE_EXPORT
#   endif
    // can used by: after get the newVersionApk
    int hot_cache_lib(const char* arch_abi,const char* apkPath,const char* newApkPath,
                      const char* cacheLibFilesDir) HOT_CACHE_EXPORT;
    
#ifdef __cplusplus
}
#endif
#endif //_hot_cache_lib_h_

