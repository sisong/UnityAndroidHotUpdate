// hot_cache_lib.cpp
// Created by sisong on 2019-08-20.

#include "hot_cache_lib.h"
#ifdef __cplusplus
extern "C" {
#endif
    // https://developer.android.com/ndk/guides/abis
    
    #define _IsDebug 0

    static const int kMaxPathLen=512-1;
    static const char kDirTag='/';
    
    
    
    
    int hot_cache_lib_test(const char* apkPath,const char* newApkPath,
                           const char* arch_abi){
        
        return -1;
    }
    
    int hot_cache_lib(const char* apkPath,const char* newApkPath,
                      const char* arch_abi,const char* cacheLibFilesDir){
        
        return -1;
    }
    
#ifdef __cplusplus
}
#endif

