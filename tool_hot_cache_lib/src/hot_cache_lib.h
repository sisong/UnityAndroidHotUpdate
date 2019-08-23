// hot_cache_lib.h
// Created by sisong on 2019-08-20.
#ifndef _hot_cache_lib_h_
#define _hot_cache_lib_h_
#ifdef __cplusplus
extern "C" {
#endif
    
    enum hot_cache_lib_result_t{
        kCacheLib_ok =0,
        kCacheLib_fileError,
        kCacheLib_abiError,
        kCacheLib_unzipError,
        kCacheLib_monoIl2cppCrossError,
        kCacheLib_libChangedError, // now it's libmain.so changed
    };
    
    // can used by: build apk update setting or after get the newVersionApk
    //   now only check libmain.so
    int hot_cache_lib_check(const char* arch_abi,const char* apkPath,const char* newApkPath);

    // can used by: after get the newVersionApk
    int hot_cache_lib(const char* arch_abi,const char* apkPath,const char* newApkPath,
                      const char* cacheLibFilesDir);
    
#ifdef __cplusplus
}
#endif
#endif //_hot_cache_lib_h_

