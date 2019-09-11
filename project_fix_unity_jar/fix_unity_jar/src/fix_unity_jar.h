// fix_unity_jar.h
// Created by sisong on 2019-09-10.
#ifndef _fix_unity_jar_h_
#define _fix_unity_jar_h_
#ifdef __cplusplus
extern "C" {
#endif
    
    enum hot_cache_lib_result_t{
        kFixUnityJar_ok =0,
        kFixUnityJar_inputJarFileError,
        kFixUnityJar_outputJarFileError,
        kFixUnityJar_classFileError,
        kFixUnityJar_decompressError,
        kFixUnityJar_fixClassError
    };
    
    //fix unity-classes.jar file : change load libmain.so to load libnull.so
    int fix_unity_jar(const char* unity_classes_jar,const char* out_new_jar);
    
#ifdef __cplusplus
}
#endif
#endif //_fix_unity_jar_h_

