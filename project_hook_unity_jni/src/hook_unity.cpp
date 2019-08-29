// hook_unity.cpp
// Created by sisong on 2019-08-15.

#include "hook_unity.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h> //exit
#include <sys/stat.h>
#include <fcntl.h>
#include "../xHook/libxhook/jni/xhook.h"
#include <android/log.h>
#include <dlfcn.h> // dlopen
#ifdef __cplusplus
extern "C" {
#endif
    static const char* kLibMain="libmain.so";
    static const char* kLibUnity="libunity.so";
    static int         kLibUnityLen=11;//strlen(kLibUnity);
    //for il2cpp
    static const char* kLibIL2cpp="libil2cpp.so";
    
#define _IsDebug 1
#define _LogTag "HotUnity"
#define LOG_INFO(tag,info)         { __android_log_print(ANDROID_LOG_INFO,_LogTag,tag "\n",info); }
#define LOG_INFO2(tag,info0,info1) { __android_log_print(ANDROID_LOG_INFO,_LogTag,tag "\n",info0,info1); }
#define LOG_ERROR(tag,err)         { __android_log_print(ANDROID_LOG_ERROR,_LogTag,tag "\n",err); }
#define LOG_ERROR2(tag,err0,err1)  { __android_log_print(ANDROID_LOG_ERROR,_LogTag,tag "\n",err0,err1); }
#define LOG_DEBUG(tag,info)        { if (_IsDebug) LOG_INFO(tag,info); }
#define LOG_DEBUG2(tag,info0,info1){ if (_IsDebug) LOG_INFO2(tag,info0,info1); }

    static const int kMaxPathLen=512-1;
    static const char kDirTag='/';
    
    static bool g_isMapPath =false;
    static char g_apkPath[kMaxPathLen+1]={0};
    static  int g_apkPathLen=0;
    static char g_soDir[kMaxPathLen+1]={0};
    static  int g_soDirLen=0;
    static char g_newApkPath[kMaxPathLen+1]={0};
    static  int g_newApkPathLen=0;
    static char g_soCacheDir[kMaxPathLen+1]={0};
    static  int g_soCacheDirLen=0;
    
    
    static const char*  getFileName(const char* filePath){
        int pathLen=(int)strlen(filePath);
        int i=pathLen-1;
        for (;i>=0;--i) {
            if (filePath[i]==kDirTag) break;
        }
        return filePath+(i+1);
    }
    
    static bool isFindFile(const char* filePath){
        struct stat file_stat;
        memset(&file_stat,0,sizeof(file_stat));
        int ret=stat(filePath,&file_stat);
        return (ret==0)&&((file_stat.st_mode&S_IFREG)!=0);
    }
    
    
    #define __MAP_PATH_TO(src,clipLen,isAddDirTag,dst,opath,errValue,need_isFindFile,isCanMap) \
    if (g_isMapPath){              \
        int olen=strlen(opath);     \
        if ((olen>=src##Len)&&(0==memcmp(opath,src,src##Len))   \
            &&((opath[src##Len]=='\0')||(opath[src##Len]==kDirTag))){ \
            if (dst##Len+(olen-clipLen)+isAddDirTag>kMaxPathLen)\
                { LOG_ERROR2("MAP_PATH() len %d %s",olen,opath);  return errValue; } \
            isCanMap=true;          \
            memcpy(_newPath,dst,dst##Len);              \
            if (isAddDirTag) _newPath[dst##Len]=kDirTag;\
            memcpy(_newPath+dst##Len+isAddDirTag,opath+clipLen,olen-clipLen+1); \
            if ((!need_isFindFile)||isFindFile(_newPath)){      \
                opath=&_newPath[0]; \
                LOG_DEBUG("MAP_PATH() to %s",opath);    \
            }else{                  \
                LOG_DEBUG("MAP_PATH() not found %s",_newPath);  \
            } } }

    #define _MAP_PATH_TO(src,dst,opath,errValue,need_isFindFile,isCanMap)   \
        __MAP_PATH_TO(src,src##Len,0,dst,opath,errValue,need_isFindFile,isCanMap)

    #define IS_MAPED_PATH(opath) (opath==&_newPath[0])
    
    #define MAP_PATH(opath,errValue)    \
        char _newPath[kMaxPathLen+1];   \
        bool _null_no_use=false;        \
        _MAP_PATH_TO(g_apkPath,g_newApkPath,opath,errValue,0,_null_no_use); \
        if (!IS_MAPED_PATH(opath))      \
            _MAP_PATH_TO(g_soDir,g_soCacheDir,opath,errValue,1,_null_no_use);
    
    #define MAP_SO_PATH(opath,errValue) \
        char _newPath[kMaxPathLen+1];   \
        bool isSoDirCanMap=false;       \
        int  _null_no_use=false;        \
        _MAP_PATH_TO(g_soDir,g_soCacheDir,opath,errValue,1,isSoDirCanMap); \
        if (!IS_MAPED_PATH(opath)) {    \
            __MAP_PATH_TO(kLibUnity,0,1,g_soCacheDir,opath,errValue,1,_null_no_use); } \
        if (!IS_MAPED_PATH(opath)) {    \
            _MAP_PATH_TO(g_apkPath,g_newApkPath,opath,errValue,0,_null_no_use); }
    
    //stat
    static int new_stat(const char* path,struct stat* file_stat){
        const int errValue=-1;
        LOG_DEBUG("new_stat() %s",path);
        MAP_PATH(path,errValue);
        return ::stat(path,file_stat);
    }
    
    //fopen
    static FILE* new_fopen(const char* path,const char* mode){
        FILE* const errValue=NULL;
        LOG_DEBUG2("new_fopen() %s %s",mode,path);
        MAP_PATH(path,errValue);
        return ::fopen(path,mode);
    }
    
    //open
    static int new_open(const char *path, int flags, ...){
        const int errValue=-1;
        LOG_DEBUG2("new_open() %d %s",flags,path);
        MAP_PATH(path,errValue);
        
        va_list args;
        va_start(args,flags);
        int result=::open(path,flags,args);
        va_end(args);
        return result;
    }
    
    
    //dlopen
    static bool hook_lib(const char* libPath);
    static void* my_new_dlopen(const char* path,int flags,bool isMustLoad,bool isMustHookOk){
        void* const errValue=NULL;
        LOG_INFO2("new_dlopen() %d %s",flags,path);
        MAP_SO_PATH(path,errValue);
        if ((!isMustLoad)&&(!isFindFile(path)))
            return errValue;
        
        void* result=::dlopen(path,flags);
        LOG_DEBUG2("dlopen() result 0x%08x %s",(unsigned int)(size_t)result,path);
        if ((result!=errValue)&&isSoDirCanMap){
            bool ret=hook_lib(path);
            if ((!ret)&&isMustHookOk)
                return errValue;
        }
        return result;
    }
    
    static void* new_dlopen(const char* path,int flags){
        return my_new_dlopen(path,flags,true,false);
    }
    

    #define HOOK(lib,errValue,symbol){ \
        if (0!=xhook_register(lib,#symbol,(void*)new_##symbol,NULL)){ \
            LOG_ERROR2("hook_lib() failed to find function:%s in %s",#symbol,lib); return errValue; } }

    static bool hook_lib(const char* libPath){
        libPath=getFileName(libPath);
        
        //xhook_enable_debug(1);
        const bool errValue=false;
        HOOK(libPath,errValue,stat);
        HOOK(libPath,errValue,fopen);
        HOOK(libPath,errValue,open);
        HOOK(libPath,errValue,dlopen);
        if(0 != xhook_refresh(0)){
            LOG_ERROR("hook_lib() failed to hook %s",libPath);
            return errValue; }
        xhook_clear();
        LOG_INFO("hook_lib() success to hook %s",libPath);
        return true;
    }
    
    
    static bool loadUnityLib(const char* libName,bool isMustLoad,bool isMustHookOk){
        const bool errValue=false;
        const int flags= RTLD_GLOBAL|RTLD_NOW;
        char libPath[kMaxPathLen+1];
        const int nameLen=(int)strlen(libName);
        if (g_soDirLen+1+nameLen>kMaxPathLen)
            { LOG_ERROR2("loadUnityLib() len %d %s",nameLen,libName);  return errValue; }
        memcpy(libPath,g_soDir,g_soDirLen);
        libPath[g_soDirLen]=kDirTag;
        memcpy(libPath+g_soDirLen+1,libName,nameLen+1);
        
        void* result=my_new_dlopen(libPath,flags,isMustLoad,isMustHookOk);
        return (result!=NULL)||(!isMustLoad);
    }
    
    #define  _LOAD_LIB(libName,isMustLoad,isMustHookOk) { \
        if (!loadUnityLib(libName,isMustLoad,isMustHookOk)) return false; }
    
    static bool loadUnityLibs(){
        _LOAD_LIB(kLibMain,true,true);   //if new version libmain.so, maybe fail on some device?
        _LOAD_LIB(kLibUnity,true,true);
        _LOAD_LIB(kLibIL2cpp,false,false); // pre-load for il2cpp
        //test found : not need pre-load lib for mono libs
        return true;
    }
    
    
    #define _ERR_RETURN() { g_isMapPath=false; exit(-1); return; }
    
    #define _COPY_PATH(dst,src) {   \
        dst##Len=strlen(src);       \
        if (dst##Len>kMaxPathLen)   \
            { LOG_ERROR("hook_unity_doHook() strlen("#src") %d",dst##Len); _ERR_RETURN(); } \
        memcpy(dst,src,dst##Len+1); }
    
    void hook_unity_doHook(const char* apkPath,const char* soDir,
                           const char* newApkPath,const char* soCacheDir){
        LOG_INFO2("hook_unity_doHook() %s %s",apkPath,soDir);
        LOG_INFO2("hook_unity_doHook() %s %s",newApkPath,soCacheDir);
        
        _COPY_PATH(g_apkPath,apkPath);
        _COPY_PATH(g_soDir,soDir);
        _COPY_PATH(g_newApkPath,newApkPath);
        _COPY_PATH(g_soCacheDir,soCacheDir);
        
        bool isDoHook=(g_newApkPathLen>0)&&isFindFile(newApkPath);
        if (!isDoHook) { LOG_INFO("hook_unity_doHook() %s","not do hook"); return; }
        g_isMapPath=true;
        if (!loadUnityLibs()) _ERR_RETURN();
    }

#ifdef __cplusplus
}
#endif

