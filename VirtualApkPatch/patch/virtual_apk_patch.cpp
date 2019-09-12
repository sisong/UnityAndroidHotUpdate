// virtual_apk_patch.cpp
// Created by sisong on 2019-09-11.

#include "virtual_apk_patch.h"
#include <assert.h>
#include "../../ApkDiffPatch/src/patch/Zipper.h"
#include "../../ApkDiffPatch/HDiffPatch/file_for_patch.h"


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

#define isEndOfDirTag(str,strLen)  ( (strLen>0) && ((str[strLen-1]=='\\') || (str[strLen-1]=='/')))

static bool isLibFile(const UnZipper* apk,int fi,const char* libDir,const int libDirLen){
    const char* fileName=UnZipper_file_nameBegin(apk,fi);
    const int fileNameLen=UnZipper_file_nameLen(apk,fi);
    if (fileNameLen<=libDirLen) return false;
    if (0!=memcmp(fileName,libDir,libDirLen)) return false;
    if (isEndOfDirTag(fileName,fileNameLen))  return false; //is dir
    return true;
}

static bool findLibDir(const UnZipper* apk,const char* arch_abi,
                       char* outLibDir,size_t libDirBufSize,int* out_maxLibCount){
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
            &&(0==memcmp(UnZipper_file_nameBegin(apk,i),outLibDir,dirLen))){
            //ok found arch_abi, to get maxLibCount
            *out_maxLibCount=0;
            for (int j=i;j<fileCount;++j){
                if (isLibFile(apk,j,outLibDir,dirLen))
                    ++(*out_maxLibCount);
            }
            return true;
        }
    }
    
    const char* abi_backup=get_abi_backup(arch_abi);
    if (abi_backup!=0)
        return findLibDir(apk,abi_backup,outLibDir,libDirBufSize,out_maxLibCount);
    else
        return false;
}

static bool fileIsExists(const char* filePath){
    hpatch_TPathType    type;
    if (hpatch_getPathStat(filePath,&type,0)){
        if (type==kPathType_file)
            return true;
    }
    return false;
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

static bool libIsInDir(const UnZipper* apk,int fi,const char* cacheSoDir){
    char path[kMaxPathLen+1];
    if (!getPath(cacheSoDir,UnZipper_file_nameBegin(apk,fi),UnZipper_file_nameLen(apk,fi),
                 path,sizeof(path))) return false;
    return fileIsExists(path);
}


enum TLibCacheType {
    kTLibCache_inApk =0,
    kTLibCache_inBaseSoDir,
    kTLibCache_inHotSoDir
};

struct TLibCacheInfo {
    int             indexInApk;
    TLibCacheType   type;
};

#define _rt_err(errValue) { result=errValue; break; }

int getSoMapList(const char* apkPath,const char* baseSoDir,const char* hotSoDir,const char* arch_abi,
                 TLibCacheInfo** out_soMapList,int* out_soMapListCount){
    //find all so files in hotSoDir or baseSoDir and cached from apk;
    int result=kVApkPatch_ok;
    TLibCacheInfo* soMapList=0;
    int            soMapListCount=0;
    UnZipper apk;
    UnZipper_init(&apk);
    do{
        if (!UnZipper_openFile(&apk,apkPath)) _rt_err(kVApkPatch_apkFileError);
        //lib dir
        char libDir[kMaxPathLen+1];
        if (!findLibDir(&apk,arch_abi,libDir,kMaxPathLen+1,&soMapListCount))
            _rt_err(kVApkPatch_abiError);
        const int libDirLen=(int)strlen(libDir);
        
        if (soMapListCount==0) break;
        soMapList=(TLibCacheInfo*)malloc(soMapListCount);
        int insert=0;
        int fileCount=UnZipper_fileCount(&apk);
        for (int i=0; i<fileCount; ++i) {
            if (isLibFile(&apk,i,libDir,libDirLen)){
                TLibCacheType type=kTLibCache_inApk;
                if (libIsInDir(&apk,i,hotSoDir))
                    type=kTLibCache_inHotSoDir;
                else if (libIsInDir(&apk,i,baseSoDir))
                    type=kTLibCache_inBaseSoDir;
                if (type!=kTLibCache_inApk){
                    soMapList[insert].indexInApk=i;
                    soMapList[insert].type=type;
                    ++insert;
                }
            }
        }
        assert(insert<=soMapListCount);
        soMapListCount=insert;
    }while(0);
    UnZipper_close(&apk);
    if (result!=kVApkPatch_ok){
        if (soMapList) free(soMapList);
    }else{
        *out_soMapList=soMapList;
        *out_soMapListCount=soMapListCount;
    }
    return result;
}

int virtual_apk_patch(const char* baseApk,const char* baseSoDir,
                      const char* hotApk,const char* hotSoDir,
                      const char* diffData,const char* arch_abi,
                      bool  isCacheHotSo){
    //curApk
    const char* curApk=baseApk;
    if (fileIsExists(hotApk)) curApk=hotApk;
    
    TLibCacheInfo* in_soMapList=0;
    int            in_soMapListCount=0;
    int result=getSoMapList(curApk,baseSoDir,hotSoDir,arch_abi,
                            &in_soMapList,&in_soMapListCount);
    if (result!=kVApkPatch_ok) return result;
    
    do{
        //tod:
        
        
    }while(0);
    if (in_soMapList) free(in_soMapList);
    return result;
}

