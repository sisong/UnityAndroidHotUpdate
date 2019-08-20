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
#ifdef __cplusplus
extern "C" {
#endif
    static const char* kLibUnity="libunity.so";
    #define _IsDebug 0
#define LOG_INFO(tag,info)         { __android_log_print(ANDROID_LOG_INFO,\
                                        "HookUnity_LOG",tag "\n",info); }
#define LOG_INFO2(tag,info0,info1) { __android_log_print(ANDROID_LOG_INFO,\
                                        "HookUnity_LOG",tag "\n",info0,info1); }
#define LOG_ERROR(tag,err)         { __android_log_print(ANDROID_LOG_ERROR,\
                                        "HookUnity_ERROR",tag "\n",err); }
#define LOG_ERROR2(tag,err0,err1)  { __android_log_print(ANDROID_LOG_ERROR,\
                                        "HookUnity_ERROR",tag "\n",err0,err1); }
#define LOG_DEBUG(tag,info)        { if (_IsDebug) LOG_INFO(tag,info); }
#define LOG_DEBUG2(tag,info0,info1){ if (_IsDebug) LOG_INFO2(tag,info0,info1); }

    static const int kMaxPathLen=1023;
    static const char kDirTag='/';
    
    static int  g_isHookSuccess =0;
    static char g_apkPath[kMaxPathLen+1]={0};
    static  int g_apkPathLen=0;
    static char g_soDir[kMaxPathLen+1]={0};
    static  int g_soDirLen=0;
    static char g_newApkPath[kMaxPathLen+1]={0};
    static  int g_newApkPathLen=0;
    static char g_soCacheDir[kMaxPathLen+1]={0};
    static  int g_soCacheDirLen=0;
    
    
    static int isFindFile(const char* filePath){
        struct stat file_stat;
        memset(&file_stat,0,sizeof(file_stat));
        int ret=stat(filePath,&file_stat);
        return (ret==0)&&((file_stat.st_mode&S_IFREG)!=0);
    }
    
    #define _MAP_PATH_TO(src,dst,opath,errValue,need_isFindFile)    \
        char _newPath[kMaxPathLen+1];   \
        if (g_isHookSuccess){           \
            int olen=strlen(opath);     \
            if ((olen>=src##Len)&&(0==memcmp(opath,src,src##Len))   \
                &&((opath[src##Len]=='\0')||(opath[src##Len]=='/'))){       \
                if (dst##Len+(olen-src##Len)>kMaxPathLen)   \
                    { LOG_ERROR2("MAP_PATH() len %d %s",olen,opath);  return errValue; } \
                memcpy(_newPath,dst,dst##Len);              \
                memcpy(_newPath+dst##Len,opath+src##Len,olen-src##Len+1);   \
                if ((!need_isFindFile)||isFindFile(_newPath)){              \
                    opath=_newPath;     \
                    LOG_DEBUG("MAP_PATH() to %s",opath);    \
                }else{                  \
                    LOG_DEBUG("MAP_PATH() failed to %s",_newPath);          \
                } } }

    #define MAP_PATH(opath,errValue) _MAP_PATH_TO(g_apkPath,g_newApkPath,opath,errValue,0)

    #define CHECK_FUNC(tag,checkedFunc,errValue) { \
        if (checkedFunc==NULL) { LOG_ERROR(tag,"old func is NULL"); return errValue; } }

    
    //stat
    typedef int (* TFuncStat)(const char* path,struct stat *file_stat);
    static TFuncStat old_stat = NULL;
    static int new_stat(const char* path,struct stat* file_stat){
        LOG_DEBUG("new_stat() %s",path);
        CHECK_FUNC("new_stat() %s",old_stat,-1);
        MAP_PATH(path,-1);
        return old_stat(path,file_stat);
    }
    
    //fopen
    typedef FILE* (* TFuncFOpen)(const char* path,const char* mode);
    static TFuncFOpen old_fopen = NULL;
    static FILE* new_fopen(const char* path,const char* mode){
        LOG_DEBUG2("new_fopen() %s %s",mode,path);
        CHECK_FUNC("new_fopen() %s",old_fopen,NULL);
        MAP_PATH(path,NULL);
        return old_fopen(path,mode);
    }
    
    //open
    typedef int (*TFuncOpen)(const char* path,int flags,...);
    static TFuncOpen old_open = NULL;
    static int new_open(const char *path, int flags, ...){
        mode_t mode = 0;
        int is_mode = ((flags & O_CREAT) == O_CREAT) || ((flags & 020000000) == 020000000);
        if (is_mode){
            va_list args;
            va_start(args, flags);
            mode = (mode_t)(va_arg(args, int));
            va_end(args);
        }
        LOG_DEBUG2("new_open() %d %s",flags,path);
        CHECK_FUNC("new_open() %s",old_open,-1);
        MAP_PATH(path,-1);
        return is_mode? old_open(path,flags,mode) : old_open(path,flags);
    }
    
    #define MAP_SO_PATH(opath,errValue) _MAP_PATH_TO(g_soDir,g_soCacheDir,opath,errValue,1)
    
    //dlopen
    typedef void* (*TFuncDlopen)(const char* path,int flags);
    static TFuncDlopen old_dlopen = NULL;
    static void* new_dlopen(const char* path,int flags){
        LOG_INFO2("new_dlopen() %d %s",flags,path);
        CHECK_FUNC("new_dlopen() %s",old_dlopen,NULL);
        MAP_SO_PATH(path,NULL);
        LOG_INFO2("old_dlopen() %d %s",flags,path);
        //chmod(path,0755);
        return old_dlopen(path,flags);
    }

    #define HOOK(lib,symbol){ \
        if (0!=xhook_register(lib,#symbol,(void*)new_##symbol,(void **)&old_##symbol)){ \
            LOG_ERROR2("hook() failed to find function:%s in %s",#symbol,lib); return -1; }}
    static int hook(){
        //xhook_enable_debug(1);
        
        HOOK(kLibUnity,stat);
        HOOK(kLibUnity,fopen);
        HOOK(kLibUnity,open);
        HOOK(kLibUnity,dlopen);
        
        if(0 != xhook_refresh(1)){
            LOG_ERROR("hook() %s","failed to replace function");
            return -1;
        }
        return 0;
    }
    
    #define _COPY_PATH(dst,src) {\
        dst##Len=strlen(src);   \
        if (dst##Len>kMaxPathLen)   \
            { LOG_ERROR("hook_unity_doHook() strlen("#src") %d",dst##Len); return; } \
        memcpy(dst,src,dst##Len+1); }
    
    void hook_unity_doHook(const char* apkPath,const char* soDir,
                           const char* newApkPath,const char* soCacheDir){
        LOG_INFO2("hook_unity_doHook() %s %s",apkPath,soDir);
        LOG_INFO2("hook_unity_doHook() %s %s",newApkPath,soCacheDir);
        int isDoHook=0;
        
        _COPY_PATH(g_apkPath,apkPath);
        _COPY_PATH(g_soDir,soDir);
        _COPY_PATH(g_newApkPath,newApkPath);
        _COPY_PATH(g_soCacheDir,soCacheDir);
        
        isDoHook=(g_newApkPathLen>0)&&isFindFile(newApkPath);
        if (!isDoHook){ LOG_INFO("hook_unity_doHook() %s","not doHook"); return; }
        int ret=hook();
        if (ret!=0) { exit(-1); return; }
        LOG_INFO("hook_unity_doHook() %s","success to doHook");
        g_isHookSuccess=1;
    }

#ifdef __cplusplus
}
#endif

