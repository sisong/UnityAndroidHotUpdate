// virtual_apk_patch.cpp
// Created by sisong on 2019-09-11.

#include "virtual_apk_patch.h"
#include <assert.h>
#include "../../ApkDiffPatch/src/patch/Zipper.h"
#include "../../ApkDiffPatch/HDiffPatch/file_for_patch.h"
#include "../../ApkDiffPatch/src/patch/VirtualZipIO.h"
#include "../../ApkDiffPatch/src/patch/Patcher.h"
#include "../../ApkDiffPatch/HDiffPatch/dirDiffPatch/dir_diff/file_for_dirDiff.h"

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
                       char* outLibDir,size_t libDirBufSize){
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
            return true; //ok found arch_abi
        }
    }
    
    const char* abi_backup=get_abi_backup(arch_abi);
    if (abi_backup!=0)
        return findLibDir(apk,abi_backup,outLibDir,libDirBufSize);
    else
        return false;
}

static int findCountInLibDir(const UnZipper* apk,const char* libDir){
    int result=0;
    int dirLen=(int)strlen(libDir);
    int fileCount=UnZipper_fileCount(apk);
    for (int i=0; i<fileCount; ++i) {
        if (isLibFile(apk,i,libDir,dirLen))
            ++result;
    }
    return result;
}

static inline bool fileIsExists(const char* filePath){
    hpatch_TPathType    type;
    if (hpatch_getPathStat(filePath,&type,0)){
        if (type==kPathType_file)
            return true;
    }
    return false;
}

static inline bool dirIsExists(const char* dirPath){
    hpatch_TPathType    type;
    if (hpatch_getPathStat(dirPath,&type,0)){
        if (type==kPathType_dir)
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

static const char* getFileName(const char* filePath,int pathLen){
    int i=pathLen-1;
    for (;i>=0;--i) {
        if (filePath[i]==kDirTag) break;
    }
    return filePath+(i+1);
}

static bool libIsInDir(const UnZipper* apk,int fi,const char* cacheSoDir){
    const char* _fileName=UnZipper_file_nameBegin(apk,fi);
    int nameLen=UnZipper_file_nameLen(apk,fi);
    const char* fileName=getFileName(_fileName,nameLen);
    nameLen-=(fileName-_fileName);
    char path[kMaxPathLen+1];
    if (!getPath(cacheSoDir,fileName,nameLen,path,sizeof(path))) return false;
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

static int getSoMapList(UnZipper* apk,const char* libDir,const char* baseSoDir,const char* hotSoDir,
                        TLibCacheInfo** out_soMapList,int* _soMapListCount){
    //find all so files in hotSoDir or baseSoDir and cached from apk;
    int result=kVApkPatch_ok;
    TLibCacheInfo* soMapList=0;
    int soMapListCount=*_soMapListCount;
    do{
        const int libDirLen=(int)strlen(libDir);
        
        if (soMapListCount==0) break;
        soMapList=(TLibCacheInfo*)malloc(soMapListCount);
        if (soMapList==0) _rt_err(kVApkPatch_memError);
        int insert=0;
        int fileCount=UnZipper_fileCount(apk);
        for (int i=0; i<fileCount; ++i) {
            if (isLibFile(apk,i,libDir,libDirLen)){
                TLibCacheType type=kTLibCache_inApk;
                if ((hotSoDir!=0)&&libIsInDir(apk,i,hotSoDir))
                    type=kTLibCache_inHotSoDir;
                else if (libIsInDir(apk,i,baseSoDir))
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
    if ((result!=kVApkPatch_ok)||(soMapListCount==0)){
        if (soMapList) free(soMapList);
        *out_soMapList=0;
        *_soMapListCount=0;
    }else{
        *out_soMapList=soMapList;
        *_soMapListCount=soMapListCount;
    }
    return result;
}

struct t_IVirtualZip_in{
    IVirtualZip_in  base;
    int             soMapListCount;
    TLibCacheInfo*  soMapList;
    const char*     baseSoDir;
    const char*     hotSoDir;
    TZipEntryData*           _entryList;
    hpatch_TFileStreamInput* _fileStreamList;
};
static uint32_t t_IVirtualZip_getCrc32(const struct IVirtualZip_in* virtual_in,const UnZipper* apk,
                                       int fileIndex, const TZipEntryData* entryData){
    return UnZipper_file_crc32(apk,fileIndex);
}

static bool t_IVirtualZip_in_beginVirtual(struct IVirtualZip_in* _self,const UnZipper* apk,
                                          TZipEntryData* out_entryDatas[]){
    t_IVirtualZip_in* self=(t_IVirtualZip_in*)_self->virtualImport;
    assert(self->_entryList==0);
    const int apkFileCount=UnZipper_fileCount(apk);
    size_t _memSize=(sizeof(TZipEntryData)+sizeof(hpatch_TFileStreamInput))*self->soMapListCount;
    self->_entryList=(TZipEntryData*)malloc(_memSize);
    memset(self->_entryList,0,_memSize);
    self->_fileStreamList=(hpatch_TFileStreamInput*)&self->_entryList[self->soMapListCount];
    //open files (note: no ctrl max file's handle)
    for (int i=0; i<self->soMapListCount; ++i) {
        int fIndex=self->soMapList[i].indexInApk;
        if (fIndex>=apkFileCount) return false; //error
        const char* fileDir=self->soMapList[i].type==kTLibCache_inHotSoDir?self->hotSoDir:self->baseSoDir;
        const char* _filePathName=UnZipper_file_nameBegin(apk,fIndex);
        int         fileNameLen=UnZipper_file_nameLen(apk,fIndex);
        const char* fileName=getFileName(_filePathName,fileNameLen);
        fileNameLen-=(fileName-_filePathName);
        char filePath[kMaxPathLen+1];
        if (!getPath(fileDir,fileName,fileNameLen,filePath,sizeof(filePath))) return false;
        if (!hpatch_TFileStreamInput_open(&self->_fileStreamList[i],filePath)) return false;
    }
    
    for (int i=0; i<self->soMapListCount; ++i) {
        self->_entryList[i].isCompressed=false;
        self->_entryList[i].dataStream=&self->_fileStreamList[i].base;
        self->_entryList[i].uncompressedSize=(ZipFilePos_t)self->_entryList[i].dataStream->streamSize;
        int fIndex=self->soMapList[i].indexInApk;
        out_entryDatas[fIndex]=&self->_entryList[i];
    }
    return true;
}
static bool t_IVirtualZip_in_endVirtual(IVirtualZip_in* _self){
    if (_self==0) return true;
    t_IVirtualZip_in* self=(t_IVirtualZip_in*)_self->virtualImport;
    bool result=true;
    if (self->_fileStreamList){
        for (int i=0; i<self->soMapListCount; ++i) {
            if (!hpatch_TFileStreamInput_close(&self->_fileStreamList[i]))
                result=false;
        }
    }
    if (self->_entryList){
        free(self->_entryList);
        self->_entryList=0;
    }
    return result;
}

static void t_IVirtualZip_in_set(t_IVirtualZip_in* self,const char* baseSoDir,const char* hotSoDir){
    self->baseSoDir=baseSoDir;
    self->hotSoDir=hotSoDir;
    self->base.virtualImport=self;
    self->base.getCrc32=t_IVirtualZip_getCrc32;
    self->base.beginVirtual=t_IVirtualZip_in_beginVirtual;
    self->base.endVirtual=t_IVirtualZip_in_endVirtual;
}


struct t_IVirtualZip_out{
    IVirtualZip_out base;
    int             libDirLen;
    const char*     libDir;
    const char*     baseSoDir;
    const char*     hotSoDir;
    const char*     temp_newSoDir;
    hpatch_TFileStreamOutput _fileStream;
};


static TVirtualZip_out_type t_IVirtualZip_out_beginVirtual(IVirtualZip_out* _self,
                                                           const UnZipper* newInfoZip,int newFileIndex){
    t_IVirtualZip_out* self=(t_IVirtualZip_out*)_self;
    assert(self->base.virtualStream==0);
    if (!isLibFile(newInfoZip,newFileIndex,self->libDir,self->libDirLen))
        return kVirtualZip_out_void; //not .so file
    //maybe same .so file in baseApk (not in HotApk)? then can return kVirtualZip_out_emptyFile_cast;
    
    char newPath[kMaxPathLen+1];
    const char* newName=UnZipper_file_nameBegin(newInfoZip,newFileIndex);
    int newNameLen=UnZipper_file_nameLen(newInfoZip,newFileIndex);
    if (!getPath(self->temp_newSoDir,newName+self->libDirLen,newNameLen-self->libDirLen,
                 newPath,sizeof(newPath))) return kVirtualZip_out_error;
    size_t newFileSize=UnZipper_file_uncompressedSize(newInfoZip,newFileIndex);
    if (!hpatch_TFileStreamOutput_open(&self->_fileStream,newPath,newFileSize))
         return kVirtualZip_out_error;
    
    self->base.virtualStream=&self->_fileStream.base;
    return kVirtualZip_out_emptyFile_uncompressed;
}

static TVirtualZip_out_type t_IVirtualZip_out_beginVirtualCopy(IVirtualZip_out* _self,
                                                               const UnZipper* newInfoZip,int newFileIndex,
                                                               const UnZipper* oldZip,int oldFileIndex){
    t_IVirtualZip_out* self=(t_IVirtualZip_out*)_self;
    assert(self->base.virtualStream==0);
    if (!isLibFile(newInfoZip,newFileIndex,self->libDir,self->libDirLen))
        return kVirtualZip_out_void; //not .so file
    
    //must same name
    const char* newName=UnZipper_file_nameBegin(newInfoZip,newFileIndex);
    int newNameLen=UnZipper_file_nameLen(newInfoZip,newFileIndex);
    if (newNameLen!=UnZipper_file_nameLen(oldZip,oldFileIndex)
        ||(0!=memcmp(newName,UnZipper_file_nameBegin(oldZip,oldFileIndex),newNameLen)))
        return t_IVirtualZip_out_beginVirtual(_self,newInfoZip,newFileIndex);

    return kVirtualZip_out_emptyFile_cast;
}

static bool t_IVirtualZip_out_endVirtual(IVirtualZip_out* _self){
    if (_self==0) return true;
    t_IVirtualZip_out* self=(t_IVirtualZip_out*)_self;
    assert(self->base.virtualStream!=0);
    hpatch_BOOL rt=hpatch_TFileStreamOutput_close(&self->_fileStream);
    if (rt) hpatch_TFileStreamOutput_init(&self->_fileStream);
    self->base.virtualStream=0;
    return (bool)rt;
}


static void t_IVirtualZip_out_set(t_IVirtualZip_out* self,const char* libDir,
                                  const char* baseSoDir,const char* hotSoDir,const char* temp_newSoDir){
    self->libDir=libDir;
    self->libDirLen=(int)strlen(libDir);
    assert(self->libDirLen>0);
    self->baseSoDir=baseSoDir;
    self->hotSoDir=hotSoDir;
    self->temp_newSoDir=temp_newSoDir;
    self->base.virtualImport=self;
    assert(self->base.virtualStream==0);
    self->base.beginVirtual=t_IVirtualZip_out_beginVirtual;
    self->base.beginVirtualCopy=t_IVirtualZip_out_beginVirtualCopy;
    self->base.endVirtual=t_IVirtualZip_out_endVirtual;
}

static int _virtual_apk_patch(const char* baseApk,const char* baseSoDir,
                              const char* hotApk,const char* hotSoDir,
                              const char* out_newApk,const char* out_newChangedSoDir,
                              const char* zipDiffPath,const char* arch_abi,
                              int threadNum){
    if ((out_newChangedSoDir==0)||(strlen(out_newChangedSoDir)==0)) out_newChangedSoDir=0;
    if ((hotApk==0)||(strlen(hotApk)==0)) hotApk=0;
    if ((hotSoDir==0)||(strlen(hotSoDir)==0)) hotSoDir=0;
    assert((out_newApk!=0)&&(strlen(out_newApk)>0));
    char temp_uncompressFileName[kMaxPathLen+1]; temp_uncompressFileName[0]=0;
    UnZipper apk;
    UnZipper_init(&apk);
    
    t_IVirtualZip_in virtual_in;
    memset(&virtual_in,0,sizeof(virtual_in));
    t_IVirtualZip_out virtual_out;
    memset(&virtual_out,0,sizeof(virtual_out));
    int result=kVApkPatch_ok;
    do{
        //lib dir
        if (!UnZipper_openFile(&apk,baseApk)) _rt_err(kVApkPatch_baseApkFileError);
        char libDir[kMaxPathLen+1];
        if (!findLibDir(&apk,arch_abi,libDir,kMaxPathLen+1)) //must found in baseApk
            _rt_err(kVApkPatch_abiError);
        //curApk
        const char* curApk=baseApk;
        if ((hotApk!=0)&&fileIsExists(hotApk)){
            curApk=hotApk;
            if (!UnZipper_close(&apk)) _rt_err(kVApkPatch_baseApkFileError);
            UnZipper_init(&apk);
            if (!UnZipper_openFile(&apk,curApk)) _rt_err(kVApkPatch_apkFileError);
        }
        //cur .so file count
        virtual_in.soMapListCount=findCountInLibDir(&apk,libDir);
        
        //virtual_in
        if (virtual_in.soMapListCount>0){
            int rt=getSoMapList(&apk,libDir,baseSoDir,hotSoDir,
                                &virtual_in.soMapList,&virtual_in.soMapListCount);
            if (rt!=kVApkPatch_ok) _rt_err(rt);
            if (!UnZipper_close(&apk)) _rt_err(kVApkPatch_apkFileError);
            t_IVirtualZip_in_set(&virtual_in,baseSoDir,hotSoDir);
        }
        //virtual_out
        if (out_newChangedSoDir){
            if (!hpatch_makeNewDir(out_newChangedSoDir)) return kVApkPatch_mkNewSoDirError;
            t_IVirtualZip_out_set(&virtual_out,libDir,baseSoDir,hotSoDir,out_newChangedSoDir);
        }
        
        if (!hpatch_getTempPathName(out_newApk,temp_uncompressFileName,
                                    temp_uncompressFileName+sizeof(temp_uncompressFileName)))
                                        _rt_err(kVApkPatch_tempFileError);
        TPatchResult rt=VirtualZipPatch(curApk,zipDiffPath,out_newApk,0,temp_uncompressFileName,threadNum,
                                        (virtual_in.soMapListCount>0)?&virtual_in.base:0,
                                        out_newChangedSoDir?&virtual_out.base:0);
        if (rt!=PATCH_SUCCESS)
            _rt_err(kVApkPatch_patchError_base+rt);
    }while(0);
    if (virtual_in.soMapList) free(virtual_in.soMapList);
    assert(virtual_in._entryList==0);
    UnZipper_close(&apk);
    if (temp_uncompressFileName[0]) hpatch_removeFile(temp_uncompressFileName);
    return result;
}

static bool removeDirLibAndFiles(const char* soDir){
    hdiff_TDirHandle hdir=hdiff_dirOpenForRead(soDir);
    if (hdir==0) return false;
    const char* subPath=0;
    hpatch_TPathType subType;
    bool result=true;
    while (true){
        if (!hdiff_dirNext(hdir,&subType,&subPath)) _rt_err(false);
        if (subPath==0) break; //finish
        if (subType!=kPathType_file) continue;
        char fileName[kMaxPathLen+1];
        if (!getPath(soDir,subPath,(int)strlen(subPath),fileName,sizeof(fileName))) _rt_err(false);
        if (!hpatch_removeFile(fileName)) _rt_err(false);
    }
    hdiff_dirClose(hdir);
    if (!hpatch_removeDir(soDir)) result=false;
    return result;
}

int virtual_apk_patch(const char* baseApk,const char* baseSoDir,
                      const char* hotApk,const char* hotSoDir,
                      const char* out_newApk,const char* out_newChangedSoDir,
                      const char* zipDiffPath,const char* arch_abi,
                      int threadNum){
    int result=kVApkPatch_ok;
    try {
        result=_virtual_apk_patch(baseApk,baseSoDir,hotApk,hotSoDir, out_newApk,out_newChangedSoDir,
                                  zipDiffPath,arch_abi,threadNum);
    } catch (...) {
        result=kVApkPatch_catchedUnknowError;
    }
    if (result!=kVApkPatch_ok){
         //for app run by cur apk
        if ((fileIsExists(out_newApk))&&(!hpatch_removeFile(out_newApk))) //must remove
            result=kVApkPatch_newApkBadError;
        if ((dirIsExists(out_newChangedSoDir)))
            removeDirLibAndFiles(out_newChangedSoDir); //try remove
    }
    return result;
}


static bool _fileIsInApk(const char* fileName,const UnZipper* apk){
    int fCount=UnZipper_fileCount(apk);
    int fileNameLen=(int)strlen(fileName);
    for (int i=0; i<fCount; ++i) {
        if (fileNameLen!=UnZipper_file_nameLen(apk,i)) continue;
        const char* apkFName=UnZipper_file_nameBegin(apk,i);
        if (0!=memcmp(apkFName,fileName,fileNameLen)) continue;
        return true; //ok found
    }
    return false;
}

static bool removeLibFilesNotInApk(const char* hotSoDir,const UnZipper* apk,const char* libDir){
    hdiff_TDirHandle hdir=hdiff_dirOpenForRead(hotSoDir);
    if (hdir==0) return false;
    const char* subPath=0;
    hpatch_TPathType subType;
    bool result=true;
    while (true){
        if (!hdiff_dirNext(hdir,&subType,&subPath)) _rt_err(false);
        if (subPath==0) break; //finish
        if (subType!=kPathType_file) continue;
        char fileName[kMaxPathLen+1];
        if (!getPath(libDir,subPath,(int)strlen(subPath),fileName,sizeof(fileName))) _rt_err(false);
        if (!_fileIsInApk(fileName,apk)){
            if (!hpatch_removeFile(fileName)) _rt_err(false);
        }
    }
    hdiff_dirClose(hdir);
    return result;
}

static bool moveAllFilesTo(const char* srcDir,const char* dstDir){
    hdiff_TDirHandle hdir=hdiff_dirOpenForRead(srcDir);
    if (hdir==0) return false;
    const char* subPath=0;
    hpatch_TPathType subType;
    bool result=true;
    while (true){
        if (!hdiff_dirNext(hdir,&subType,&subPath)) _rt_err(false);
        if (subPath==0) break; //finish
        if (subType!=kPathType_file) continue;
        int subPathLen=(int)strlen(subPath);
        char srcName[kMaxPathLen+1];
        char dstName[kMaxPathLen+1];
        if (!getPath(srcDir,subPath,subPathLen,srcName,sizeof(srcName))) _rt_err(false);
        if (!getPath(dstDir,subPath,subPathLen,dstName,sizeof(dstName))) _rt_err(false);
        if (fileIsExists(dstName)){
            if (!hpatch_removeFile(dstName)) _rt_err(false);
        }
        if (!hpatch_moveFile(srcName,dstName)) _rt_err(false);
    }
    hdiff_dirClose(hdir);
    return result;
}


static int _virtual_apk_merge(const char* baseApk,const char* baseSoDir,
                              const char* hotApk,const char* hotSoDir,
                              const char* newApk,const char* newChangedSoDir,
                              const char* arch_abi){
    assert((hotApk!=0)&&(strlen(hotApk)>0));
    assert((newApk!=0)&&(strlen(newApk)>0));
    if (0==strcmp(hotApk,newApk))
        return kVApkMerge_ApkPathsError;
    if (!fileIsExists(newApk)){
        if (dirIsExists(newChangedSoDir))
            return kVApkMerge_lostNewApkError;
        else
            return kVApkMerge_ok;  //not need merge
    }
    
    UnZipper apk;
    UnZipper_init(&apk);
    int result=kVApkPatch_ok;
    do{
        //move newApk to hotApk
        if (fileIsExists(hotApk)){
            if (!hpatch_removeFile(hotApk)) return kVApkMerge_removeHotApkError;
        }
        if (!hpatch_moveFile(newApk,hotApk)) return kVApkMerge_renameNewApkError;
        newApk=hotApk;
        //lib dir
        if (!UnZipper_openFile(&apk,baseApk)) _rt_err(kVApkMerge_baseApkFileError);
        char libDir[kMaxPathLen+1];
        if (!findLibDir(&apk,arch_abi,libDir,kMaxPathLen+1)) //must found in baseApk
            _rt_err(kVApkMerge_abiError);
        if (!UnZipper_close(&apk)) _rt_err(kVApkMerge_baseApkFileError);
        UnZipper_init(&apk);
        //delete .so files in hotSoDir and not in newApk
        if (dirIsExists(hotSoDir)){
            if (!UnZipper_openFile(&apk,newApk)) _rt_err(kVApkMerge_newApkFileError);
            if (!removeLibFilesNotInApk(hotSoDir,&apk,libDir)) _rt_err(kVApkMerge_removeLibFilesError);
            if (!UnZipper_close(&apk)) _rt_err(kVApkMerge_newApkFileError);
        }
        //merge .so files
        if (!dirIsExists(newChangedSoDir)) break; //ok //WARNING: no .so file changed?
        if (!dirIsExists(hotSoDir)){//rename newChangedSoDir to hotSoDir
            if (!hpatch_renamePath(newChangedSoDir,hotSoDir)) _rt_err(kVApkMerge_renameLibDirError);
            break; //ok
        }
        if (!moveAllFilesTo(newChangedSoDir,hotSoDir)) _rt_err(kVApkMerge_moveLibFilesError);
        if (!hpatch_removeDir(newChangedSoDir))  _rt_err(kVApkMerge_removeNewSoDirError);
    }while(0);
    UnZipper_close(&apk);
    return result;
}

int virtual_apk_merge(const char* baseApk,const char* baseSoDir,
                      const char* hotApk,const char* hotSoDir,
                      const char* newApk,const char* newChangedSoDir,
                      const char* arch_abi){
    int result=kVApkMerge_ok;
    try {
        result=_virtual_apk_merge(baseApk,baseSoDir,hotApk,hotSoDir,
                                  newApk,newChangedSoDir,arch_abi);
    } catch (...) {
        result=kVApkMerge_catchedUnknowError;
    }
    if (result!=kVApkMerge_ok){
        //for app run by base apk
        if ((fileIsExists(hotApk))&&(!hpatch_removeFile(hotApk))) //must remove
            result=kVApkPatch_hotApkBadError;
        if ((fileIsExists(newApk))&&(!hpatch_removeFile(newApk))) //must remove
            result=kVApkPatch_hotApkBadError;
        if ((dirIsExists(hotSoDir)))
            removeDirLibAndFiles(hotSoDir); //try remove
        if ((dirIsExists(newChangedSoDir)))
            removeDirLibAndFiles(newChangedSoDir); //try remove
    }
    return result;
}
