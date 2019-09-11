// hook_unity.cpp
// Created by sisong on 2019-08-15.

#include "hook_unity.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h> //exit
#include <sys/stat.h>
#include <fcntl.h>
#include "../xHook/libxhook/jni/xh_core.h"
#include <android/log.h>
#include <dlfcn.h> // dlopen
#ifdef __cplusplus
extern "C" {
#endif
    static const char* kLibMain="libmain.so";
    static const char* kLibUnity="libunity.so";
    static const int   kLibUnityLen=11;//strlen(kLibUnity);
    //for il2cpp
    static const char* kLibIL2cpp="libil2cpp.so";
    
    #define _IsDebug 0
    #define _LogTag "HotUnity"
    #define LOG_INFO(fmt,args...)       __android_log_print(ANDROID_LOG_INFO,_LogTag,fmt, ##args)
    #define LOG_ERROR(fmt,args...)      __android_log_print(ANDROID_LOG_ERROR,_LogTag,fmt, ##args)
    #define LOG_DEBUG(fmt,args...)      do { if (_IsDebug) LOG_INFO(fmt, ##args); } while(0)

    static const int  kMaxPathLen=512-1;
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
    
    static bool pathIsExists(const char* path){
        struct stat path_stat;
        memset(&path_stat,0,sizeof(path_stat));
        int ret=stat(path,&path_stat);
        return (ret==0)&&( (path_stat.st_mode&(S_IFREG|S_IFDIR))!=0 );
    }
    
    static bool appendPath(char* dstPath,int dstPathSize,
                           const char* dir,int dirLen,const char*subPath,int subPathLen){
        if (dirLen+1+subPathLen+1>dstPathSize){
            LOG_ERROR("appendPath() error len  %s + %s",dir,subPath);
            return false;
        }
        memcpy(dstPath,dir,dirLen);
        dstPath+=dirLen;
        int dirTagCount= (((dirLen>0)&&(dir[dirLen-1]==kDirTag))?1:0)
                        +(((subPathLen>0)&&(subPath[0]==kDirTag))?1:0);
        switch (dirTagCount) {
            case 2:{ ++subPath; --subPathLen; } break;
            case 0:{ if ((dirLen>0)&&(subPathLen>0)) { dstPath[0]=kDirTag; ++dstPath; } } break;
        }
        memcpy(dstPath,subPath,subPathLen);
        dstPath[subPathLen]='\0';
        return true;
    }
    
    static const char* map_path(const char* src,int srcLen,
                                const char* dst,int dstLen,
                                const char* path,int pathLen,char* newPath,const int newPathSize,
                                bool needPathIsExists,bool* isCanMap=NULL,int clipLen=-1){
        const char* const errValue=NULL;
        if (isCanMap) *isCanMap=false;
        if (!g_isMapPath) return path;
        if ((pathLen<srcLen)||(0!=memcmp(path,src,srcLen))) return path;
        if ((path[srcLen]!='\0')&&(path[srcLen]!=kDirTag)) return path;
        
        if (isCanMap) *isCanMap=true;
        if (clipLen<0) clipLen=srcLen;
        if (!appendPath(newPath,newPathSize,dst,dstLen,path+clipLen,pathLen-clipLen)) return errValue;
        
        if ((!needPathIsExists)||pathIsExists(newPath)){
            LOG_DEBUG("map_path() to %s",newPath);
            return newPath;
        }else{
            LOG_DEBUG("map_path() not found %s",newPath);
            return path;
        }
    }
    
    static const char* map_path(const char* path,char* newPath,const int newPathSize,bool* isSoDirCanMap){
        const int pathLen=strlen(path);
        path=map_path(g_apkPath,g_apkPathLen,g_newApkPath,g_newApkPathLen,
                      path,pathLen,newPath,newPathSize,false,NULL,-1);
        if ((path==NULL)||(path==newPath)) return path;
        path=map_path(g_soDir,g_soDirLen,g_soCacheDir,g_soCacheDirLen,
                      path,pathLen,newPath,newPathSize,true,isSoDirCanMap,-1);
        if ((path==NULL)||(path==newPath)) return path;
        return map_path(kLibUnity,kLibUnityLen,g_soCacheDir,g_soCacheDirLen,
                        path,pathLen,newPath,newPathSize,true,NULL,0);
    }

    #define MAP_PATH(opath,errValue)    \
        char _newPath[kMaxPathLen+1];   \
        bool isSoDirCanMap=false;       \
        opath=map_path(opath,_newPath,sizeof(_newPath),&isSoDirCanMap); \
        if (opath==NULL) return errValue;
    
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
        LOG_DEBUG("new_fopen() %s %s",mode,path);
        MAP_PATH(path,errValue);
        return ::fopen(path,mode);
    }
    
    //open
    static int new_open(const char *path, int flags, ...){
        const int errValue=-1;
        LOG_DEBUG("new_open() %d %s",flags,path);
        MAP_PATH(path,errValue);
        
        va_list args;
        va_start(args,flags);
        int result=::open(path,flags,args);
        va_end(args);
        return result;
    }
    
#if (_IsDebug>=2)
    static void _DEBUG_log_libmaps(){
        const char* path="/proc/self/maps";
        FILE* fp=fopen(path,"r");
        char line[(kMaxPathLen+1)*2]={0};
        while(fgets(line,sizeof(line)-1, fp)){
            LOG_DEBUG(" lib maps : %s\n",line);
        }
        fclose(fp);
    }
#else
    #define _DEBUG_log_libmaps()
#endif
    
    //dlopen
    static bool hook_lib(const char* libPath);
    static void* my_new_dlopen(const char* path,int flags,bool isMustLoad,bool isMustHookOk){
        void* const errValue=NULL;
        LOG_DEBUG("new_dlopen() %d %s",flags,path);
        MAP_PATH(path,errValue);
        if ((!isMustLoad)&&(!pathIsExists(path)))
            return errValue;
        
        void* result=::dlopen(path,flags);
        LOG_INFO("dlopen() result 0x%08x %s",(unsigned int)(size_t)result,path);
        _DEBUG_log_libmaps();
        
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
        if (0!=xh_core_register(lib,#symbol,(void*)new_##symbol,NULL)){ \
            LOG_ERROR("hook_lib() failed to find function:%s in %s",#symbol,lib); return errValue; } }

    static bool hook_lib(const char* libPath){
        LOG_INFO("hook_lib() to hook %s",libPath);
        libPath=getFileName(libPath);
#if (_IsDebug>=2)
        xh_core_enable_debug(1);
#endif
        const bool errValue=false;
        HOOK(libPath,errValue,stat);
        HOOK(libPath,errValue,fopen);
        HOOK(libPath,errValue,open);
        HOOK(libPath,errValue,dlopen);
        if(0!= xh_core_refresh(0)){
            LOG_ERROR("hook_lib() failed to hook %s",libPath);
            return errValue;
        }
        xh_core_clear();
        return true;
    }
    
    
    static bool loadUnityLib(const char* libName,bool isMustLoad,bool isMustHookOk){
        char libPath[kMaxPathLen+1];
        if (!appendPath(libPath,sizeof(libPath),
                        g_soDir,g_soDirLen,libName,(int)strlen(libName))) return false;
        
        const int flags= RTLD_NOW;
        void* result=my_new_dlopen(libPath,flags,isMustLoad,isMustHookOk);
        return (result!=NULL)||(!isMustLoad);
    }
    
    static bool loadUnityLibs(){
        if (!hook_lib(kLibMain)) return false; // loaded in java, only hook
        if (!hook_lib(kLibUnity)) return false; // loaded in java, only hook
        if (!loadUnityLib(kLibIL2cpp,false,false)) return false; // pre-load for il2cpp
        //test found : not need pre-load libs for mono
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
        LOG_INFO("hook_unity_doHook() from: %s  %s",apkPath,soDir);
        LOG_INFO("hook_unity_doHook() to: %s  %s",newApkPath,soCacheDir);
        
        _COPY_PATH(g_apkPath,apkPath);
        _COPY_PATH(g_soDir,soDir);
        _COPY_PATH(g_newApkPath,newApkPath);
        _COPY_PATH(g_soCacheDir,soCacheDir);
        
        bool isDoHook=(g_newApkPathLen>0)&&pathIsExists(g_newApkPath)
                    &&(g_soCacheDirLen>0)&&pathIsExists(g_soCacheDir);
        if (!isDoHook) { LOG_INFO("hook_unity_doHook() %s","not do hook"); return; }
        g_isMapPath=true;
        if (!loadUnityLibs()) _ERR_RETURN();
    }

#ifdef __cplusplus
}
#endif

