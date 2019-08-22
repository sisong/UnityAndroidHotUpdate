// hot_cache_lib.h
// Created by sisong on 2019-08-20.
#ifndef _hot_cache_lib_h_
#define _hot_cache_lib_h_
#ifdef __cplusplus
extern "C" {
#endif
    
    enum hot_cache_lib_result_t{
        kCacheLib_ok =0,
        kCacheLib_fail_fileError,
        kCacheLib_fail_abiError,
        kCacheLib_fail_unzipError,
        kCacheLib_fail_UnityVersion,
        kCacheLib_fail_SoVersion
    };
    
    // can used by: build apk update setting or after get the newVersionApk
    int hot_cache_lib_test(const char* apkPath,const char* newApkPath,const char* arch_abi);

    // can used by: after get the newVersionApk
    int hot_cache_lib(const char* apkPath,const char* newApkPath,const char* arch_abi,
                      const char* cacheLibFilesDir,int isTestBeforeCache=1);
    
#ifdef __cplusplus
}
#endif
#endif //_hot_cache_lib_h_

