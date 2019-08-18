// hook_unity.h
// Created by sisong on 2019-08-15.
#ifndef _hook_unity_h_
#define _hook_unity_h_
#ifdef __cplusplus
extern "C" {
#endif
    void hook_unity_doHook(const char* apkPath,const char* soDir,
                           const char* newApkPath,const char* soCacheDir);
#ifdef __cplusplus
}
#endif
#endif //_hook_unity_h_

