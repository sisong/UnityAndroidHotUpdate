package com.github.sisong;
public class HookUnity{
    public static native void doHook(String newApkPath);//in hook_unity_jni.cpp
    public static void hookUnity(String newApkPath){
        System.loadLibrary("main");
        System.loadLibrary("unity");
        System.loadLibrary("hookunity");
        doHook(newApkPath);
    }
}

