// hot_cache_lib.cpp
// Created by sisong on 2019-08-20.

#include "hot_cache_lib.h"
#include "../../ApkDiffPatch/src/patch/Zipper.h"
#ifdef __cplusplus
extern "C" {
#endif
    // https://developer.android.com/ndk/guides/abis
    
    #define _IsDebug 0

    static const int kMaxPathLen=512-1;
    static const char kDirTag='/';
    
    #define _D_ABI(abi_name,abi_tag)    \
        static const char* k_##abi_name     = abi_tag;    \
        static const char* k_##abi_name##_dir = "lib/" abi_tag "/"
    _D_ABI(armeabi,"armeabi");
    _D_ABI(armeabi_v7a,"armeabi-v7a");
    _D_ABI(arm64_v8a,"arm64-v8a");
    _D_ABI(x86,"x86");
    _D_ABI(x86_64,"x86_64");

    static const char* findLibDir(const UnZipper* apk,const char* arch_abi){
        const char* libDir=0;
        // backup: armeabi <-- armeabi-v7a <-- arm64-v8a , x86 <-- x86_64
        const char* abi_backup=0;
        if (0==strcmp(arch_abi,k_armeabi)){
            libDir=k_armeabi_dir;
        }else if (0==strcmp(arch_abi,k_armeabi_v7a)){
            libDir=k_armeabi_v7a_dir;
            abi_backup=k_armeabi;
        }else if (0==strcmp(arch_abi,k_arm64_v8a)){
            libDir=k_arm64_v8a_dir;
            abi_backup=k_armeabi_v7a;
        }else if (0==strcmp(arch_abi,k_x86)){
            libDir=k_x86_dir;
        }else if (0==strcmp(arch_abi,k_x86_64)){
            libDir=k_x86_64_dir;
            abi_backup=k_x86;
        }else{
            return 0; // unknown abi
        }
        
        int fileCount=UnZipper_fileCount(apk);
        int dirLen=(int)strlen(libDir);
        for (int i=0; i<fileCount; ++i) {
            if ((UnZipper_file_nameLen(apk,i)>=dirLen)
                &&(0==memcmp(UnZipper_file_nameBegin(apk,i),libDir,dirLen)))
                return libDir; //ok found
        }
        if (abi_backup!=0)
            return findLibDir(apk,abi_backup);
        else
            return 0;
    }
    
    
    static bool isLibFile(const UnZipper* apk,int fi){
        
    }
    
    static bool isChangedFile(const UnZipper* newApk,int fi,const UnZipper* oldApk){
        
    }
    
    static bool cacheFile(const UnZipper* apk,int fi,const char* cacheLibFilesDir){
        
    }
    
    
    // skip change .so: libil2cpp.so libmono.so libmono.sym.so
    //      libmonobdwgc.so libmonobdwgc-2.0.so libmonobdwgc-2.0.sym.so libMonoPosixHelper.so
    
    
    #define _rt_err(errValue) { result=errValue; break; }
    static int do_hot_cache_lib(const char* apkPath,const char* newApkPath,
                                const char* arch_abi,const char* cacheLibFilesDir,
                                int isTestBeforeCache){
        UnZipper oldApk;
        UnZipper newApk;
        UnZipper_init(&oldApk);
        UnZipper_init(&newApk);
        int result=kCacheLib_ok;
        do{
            //open apk
            if (!UnZipper_openFile(&oldApk,apkPath))    _rt_err(kCacheLib_fail_fileError);
            if (!UnZipper_openFile(&newApk,newApkPath)) _rt_err(kCacheLib_fail_fileError);
            //lib dir
            const char* libDir=findLibDir(&oldApk,arch_abi);
            if (libDir==0)  _rt_err(kCacheLib_fail_abiError);
            if (isTestBeforeCache){
                //对比允许修改的so文件外的文件是否完全一致： 用文件大小和crc32快速判断
                //todo:
            }
            if (0!=cacheLibFilesDir){
                int newFileCount=UnZipper_fileCount(&newApk);
                for (int i=0; i<newFileCount; ++i) {
                    if (isLibFile(&newApk,i)&&isChangedFile(&newApk,i,&oldApk)){
                        if (!cacheFile(&newApk,i,cacheLibFilesDir))
                            _rt_err(kCacheLib_fail_unzipError);
                    }
                }
                if (result!=kCacheLib_ok) break;
            }
            
        }while(0);
        UnZipper_close(&newApk);
        UnZipper_close(&oldApk);
        return -1;
    }
    
    
    
    int hot_cache_lib_test(const char* apkPath,const char* newApkPath,const char* arch_abi){
        return do_hot_cache_lib(apkPath,newApkPath,arch_abi,0,1);
    }
    
    int hot_cache_lib(const char* apkPath,const char* newApkPath,const char* arch_abi,
                      const char* cacheLibFilesDir,int isTestBeforeCache){
        if (cacheLibFilesDir==0) return kCacheLib_fail_fileError;
        return do_hot_cache_lib(apkPath,newApkPath,arch_abi,cacheLibFilesDir,isTestBeforeCache);
    }
    
#ifdef __cplusplus
}
#endif

