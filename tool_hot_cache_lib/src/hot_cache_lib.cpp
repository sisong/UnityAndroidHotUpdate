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

    static const int  kMaxPathLen=512-1;
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

    static bool findLibDir(const UnZipper* apk,const char* arch_abi,char* outLibDir,size_t libDirBufSize){
        const int arch_abiLen=(int)strlen(arch_abi);
        if (arch_abiLen<=0) return false;
        const int dirLen=4+arch_abiLen+1;
        if (dirLen+1>libDirBufSize) return false;
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
            return findLibDir(apk,abi_backup,outLibDir,libDirBufSize);
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
    
    static int findFileInZip(const UnZipper* zip,const char* fileName,int fileNameLen){
        const int fileCount=UnZipper_fileCount(zip);
        for (int i=0; i<fileCount; ++i) {
            if ((fileNameLen==UnZipper_file_nameLen(zip,i))
                &&(0==memcmp(fileName,UnZipper_file_nameBegin(zip,i),fileNameLen)))
                return i; //found
        }
        return -1;
    }
    
    static bool getPath(const char* dir,const char* name,int nameLen,char* outPath,int outPathBufSize){
        int dirLen=(int)strlen(dir);
        if (isEndOfDirTag(dir,dirLen)) --dirLen;
        if (dirLen+1+nameLen+1>=outPathBufSize) return false;
        memcpy(outPath,dir,dirLen);
        outPath[dirLen]=kDirTag;
        memcpy(outPath+dirLen+1,name,nameLen);
        outPath[dirLen+1+nameLen]='\0';
        return true;
    }
    
    static bool isSameFileData(const UnZipper* apk0,int fi0,const UnZipper* apk1,int fi1){
        return ((UnZipper_file_uncompressedSize(apk0,fi0)==UnZipper_file_uncompressedSize(apk1,fi1))
                &&(UnZipper_file_crc32(apk0,fi0)==UnZipper_file_crc32(apk1,fi1)));
    }
    
    static bool isChangedFile(const UnZipper* newApk,int new_fi,const UnZipper* oldApk){
        const char* fileName=UnZipper_file_nameBegin(newApk,new_fi);
        const int fileNameLen=UnZipper_file_nameLen(newApk,new_fi);
        const int old_fi=findFileInZip(oldApk,fileName,fileNameLen);
        if (old_fi<0) return true; //not found is new, new as changed;
        return !isSameFileData(newApk,new_fi,oldApk,old_fi);
    }
    
    
    #define _rt_err(errValue) { result=errValue; break; }
    
    static int cacheLibFile(UnZipper* apk,int fi,const char* dstDir){
        int result=kCacheLib_ok;
        do{
            if (!hpatch_makeNewDir(dstDir)) _rt_err(kCacheLib_mkDirError);
            
            const char* fileName=UnZipper_file_nameBegin(apk,fi);
            const int fileNameLen=UnZipper_file_nameLen(apk,fi);
            //get namePos no dir
            int namePos=0;
            for (int i=0;i<fileNameLen;++i) { if (fileName[i]==kDirTag) namePos=i+1; }
            char dstPath[kMaxPathLen+1];
            if (!getPath(dstDir,fileName+namePos,fileNameLen-namePos,dstPath,kMaxPathLen+1))
                _rt_err(kCacheLib_outputFileError);
            
            hpatch_TFileStreamOutput file;
            hpatch_TFileStreamOutput_init(&file);
            if (!hpatch_TFileStreamOutput_open(&file,dstPath,-1)) _rt_err(kCacheLib_outputFileError); //can't write?
            if (!UnZipper_fileData_decompressTo(apk,fi,&file.base,0)) _rt_err(kCacheLib_decompressError);
            if (!hpatch_TFileStreamOutput_flush(&file)) _rt_err(kCacheLib_outputFileError);
            if (file.out_length!=UnZipper_file_uncompressedSize(apk,fi)) _rt_err(kCacheLib_outputFileError);
            if (!hpatch_TFileStreamOutput_close(&file)) _rt_err(kCacheLib_outputFileError);
            hpatch_setIsExecuteFile(dstPath);
        }while(0);
        return result;
    }
    
    
    
    int hot_cache_lib(const char* arch_abi,const char* apkPath,const char* newApkPath,
                      const char* cacheLibFilesDir){
        assert(cacheLibFilesDir!=0);
        
        UnZipper oldApk;
        UnZipper newApk;
        UnZipper_init(&oldApk);
        UnZipper_init(&newApk);
        int result=kCacheLib_ok;
        do{
            //open apk
            if (!UnZipper_openFile(&oldApk,apkPath))    _rt_err(kCacheLib_inputFileError);
            if (!UnZipper_openFile(&newApk,newApkPath)) _rt_err(kCacheLib_inputFileError);
            //lib dir
            char libDir[kMaxPathLen+1];
            if (!findLibDir(&oldApk,arch_abi,libDir,kMaxPathLen+1))
                _rt_err(kCacheLib_abiError);
            const int libDirLen=(int)strlen(libDir);
            
            int newFileCount=UnZipper_fileCount(&newApk);
            for (int i=0; i<newFileCount; ++i) {
                if (isLibFile(&newApk,i,libDir,libDirLen)&&isChangedFile(&newApk,i,&oldApk)){
                    int rt=cacheLibFile(&newApk,i,cacheLibFilesDir);
                    if (rt!=kCacheLib_ok) _rt_err(rt);
                }
            }
            if (result!=kCacheLib_ok) _rt_err(result);
            
        }while(0);
        UnZipper_close(&newApk);
        UnZipper_close(&oldApk);
        return result;
    }
    
#ifdef __cplusplus
}
#endif

