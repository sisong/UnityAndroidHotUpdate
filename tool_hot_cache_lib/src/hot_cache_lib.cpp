// hot_cache_lib.cpp
// Created by sisong on 2019-08-20.

#include "hot_cache_lib.h"
#include <assert.h>
#include "../../ApkDiffPatch/src/patch/Zipper.h"
#include "../../ApkDiffPatch/HDiffPatch/file_for_patch.h"
#ifdef __cplusplus
extern "C" {
#endif
    // https://developer.android.com/ndk/guides/abis
    
    #define _IsDebug 0

    static const int kMaxPathLen=512-1;
    static const char kDirTag='/';
    
    #define _D_ABI(abi_name,abi_tag) static const char* k_##abi_name = abi_tag
    _D_ABI(armeabi,"armeabi");
    _D_ABI(armeabi_v7a,"armeabi-v7a");
    _D_ABI(arm64_v8a,"arm64-v8a");
    _D_ABI(x86,"x86");
    _D_ABI(x86_64,"x86_64");
    
    static const char* get_abi_backup(const char* arch_abi){
        // backup: armeabi <-- armeabi-v7a <-- arm64-v8a , x86 <-- x86_64
        if (0==strcmp(arch_abi,k_armeabi_v7a))
            return k_armeabi;
        else if (0==strcmp(arch_abi,k_arm64_v8a))
            return k_armeabi_v7a;
        else if (0==strcmp(arch_abi,k_x86_64))
            return k_x86;
        return 0;
    }

    static bool findLibDir(const UnZipper* apk,const char* arch_abi,char* outLibDir,size_t libDirBufLen){
        const int arch_abiLen=(int)strlen(arch_abi);
        if (arch_abiLen<=0) return false;
        const int dirLen=4+arch_abiLen+1;
        if (dirLen+1>libDirBufLen) return false;
        memcpy(outLibDir,"lib/",4);
        memcpy(outLibDir+4,arch_abi,arch_abiLen);
        memcpy(outLibDir+4+arch_abiLen,"/\0",1+1);
        
        int fileCount=UnZipper_fileCount(apk);
        for (int i=0; i<fileCount; ++i) {
            if ((UnZipper_file_nameLen(apk,i)>=dirLen)
                &&(0==memcmp(UnZipper_file_nameBegin(apk,i),outLibDir,dirLen)))
                return true; //ok found
        }
        
        const char* abi_backup=get_abi_backup(arch_abi);
        if (abi_backup!=0)
            return findLibDir(apk,abi_backup,outLibDir,libDirBufLen);
        else
            return false;
    }
    
    #define isEndOfDirTag(str,strLen)  ( (strLen>0) && ((str[strLen-1]=='\\') || (str[strLen-1]=='/')))
    
    
    static bool isLibFile(const UnZipper* apk,int fi,const char* libDir,const int libDirLen){
        const char* fileName=UnZipper_file_nameBegin(apk,fi);
        const int fileNameLen=UnZipper_file_nameLen(apk,fi);
        if (fileNameLen<=libDirLen) return false;
        if (0!=memcmp(fileName,libDir,libDirLen)) return false;
        if (isEndOfDirTag(fileName,fileNameLen))  return false; //is dir
        return true;
    }
    
    static bool isChangedFile(const UnZipper* newApk,int fi,const UnZipper* oldApk){
        const char* fileName=UnZipper_file_nameBegin(newApk,fi);
        const int fileNameLen=UnZipper_file_nameLen(newApk,fi);
        //find file in oldApk
        const int oldFileCount=UnZipper_fileCount(oldApk);
        for (int i=0; i<oldFileCount; ++i) {
            if ((fileNameLen!=UnZipper_file_nameLen(oldApk,i))
                ||(0!=memcmp(fileName,UnZipper_file_nameBegin(oldApk,fi),i))) continue;
            //found
            return ((UnZipper_file_uncompressedSize(newApk,fi)!=UnZipper_file_uncompressedSize(oldApk,i))
                    ||(UnZipper_file_crc32(newApk,fi)!=UnZipper_file_crc32(oldApk,i)));
        }
        return true;//not found is new, new as changed;
    }
    
    static bool cacheAFile(UnZipper* apk,int fi,const char* dstDir){
        //get dstName
        char dstName[kMaxPathLen+1];
        const char* fileName=UnZipper_file_nameBegin(apk,fi);
        const int fileNameLen=UnZipper_file_nameLen(apk,fi);
        int dirLen=(int)strlen(dstDir);
        if (isEndOfDirTag(dstDir,dirLen)) --dirLen;
        if (dirLen+1+fileNameLen+1>=kMaxPathLen) return false;
        memcpy(dstName,dstDir,dirLen);
        dstName[dirLen]=kDirTag;
        memcpy(dstName+dirLen+1,fileName,fileNameLen);
        dstName[dirLen+1+fileNameLen]='\0';
        
        hpatch_TFileStreamOutput file;
        hpatch_TFileStreamOutput_init(&file);
        bool result=UnZipper_fileData_decompressTo(apk,fi,&file.base,0);
        if (result) result=hpatch_TFileStreamOutput_flush(&file);
        assert(file.out_length==UnZipper_file_uncompressedSize(apk,fi));
        hpatch_TFileStreamOutput_close(&file);
        return result;
    }
    
    
    // all .so can't delete or new
    // can skip change .so:
    //      libunity.so libil2cpp.so libmono.so
    //      libmonobdwgc.so libmonobdwgc-2.0.so libMonoPosixHelper.so
    //      libhotunity.so
    
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
            char libDir[kMaxPathLen+1];
            if (!findLibDir(&oldApk,arch_abi,libDir,kMaxPathLen+1))
                _rt_err(kCacheLib_fail_abiError);
            const int libDirLen=(int)strlen(libDir);
            if (isTestBeforeCache){
                //对比允许修改的so文件外的文件是否完全一致： 用文件大小和crc32快速判断
                //todo:
            }
            if (0!=cacheLibFilesDir){
                int newFileCount=UnZipper_fileCount(&newApk);
                for (int i=0; i<newFileCount; ++i) {
                    if (isLibFile(&newApk,i,libDir,libDirLen)&&isChangedFile(&newApk,i,&oldApk)){
                        if (!cacheAFile(&newApk,i,cacheLibFilesDir))
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

