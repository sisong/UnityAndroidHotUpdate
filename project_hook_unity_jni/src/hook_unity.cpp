// hook_unity.cpp
// Created by sisong on 2019-08-15.

#include "hook_unity.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../xHook/libxhook/jni/xhook.h"
#include <android/log.h>
#ifdef __cplusplus
extern "C" {
#endif
    static const char* kLibUnity="libunity.so";
#define _IsDebug 1
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

#define CHECK_FUNC(tag,checkedFunc,rtValue) { \
    if (checkedFunc==NULL) { LOG_ERROR(tag,"old func is NULL"); return rtValue; } }

    //stat
    typedef int (* TFuncStat)(const char* path,struct stat *file_stat);
    static TFuncStat old_stat = NULL;
    static int new_stat(const char* path,struct stat* file_stat){
        LOG_DEBUG("new_stat() %s",path);
        CHECK_FUNC("new_stat() %s",old_stat,-1);
        
        return old_stat(path,file_stat);
    }
    
    //fopen
    typedef FILE* (* TFuncFOpen)(const char* path,const char* mode);
    static TFuncFOpen old_fopen = NULL;
    static FILE* new_fopen(const char* path,const char* mode){
        LOG_DEBUG2("new_fopen() %s %s",path,mode);
        CHECK_FUNC("new_fopen() %s",old_fopen,NULL);
        
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
        LOG_DEBUG("new_open() %s",path);
        CHECK_FUNC("new_open() %s",old_open,-1);
        
        return is_mode? old_open(path,flags,mode) : old_open(path,flags);
    }
    
    //dlopen
    typedef void* (*TFuncDlopen)(const char* path,int flags);
    static TFuncDlopen old_dlopen = NULL;
    static void* new_dlopen(const char* path,int flags){
        LOG_DEBUG("new_dlopen() %s",path);
        CHECK_FUNC("new_dlopen() %s",old_dlopen,NULL);
        
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
    
    void hook_unity_doHook(const char* apkPath,const char* newApkPath){
        LOG_DEBUG2("hook_unity_doHook() %s %s",apkPath,newApkPath);
        int isDoHook=0;
        if (strlen(newApkPath)>0){
            struct stat file_stat;
            memset(&file_stat,0,sizeof(file_stat));
            int ret=stat(newApkPath,&file_stat);
            isDoHook=(ret==0)&&((file_stat.st_mode&S_IFREG)!=0);
        }
        
        if (isDoHook){
            int ret=hook();
            if (ret==0)
                LOG_DEBUG("hook_unity_doHook() %s","success doHook");
        }else{
            LOG_DEBUG("hook_unity_doHook() %s","not doHook");
        }
    }

#ifdef __cplusplus
}
#endif

