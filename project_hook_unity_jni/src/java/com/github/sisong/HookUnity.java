package com.github.sisong;
public class HookUnity{
    public static native void doHook(String apkPath,String newApkPath);
    public static void hookUnity(String apkPath,String newApkPath){
        System.loadLibrary("main");
        System.loadLibrary("unity");
        System.loadLibrary("hookunity");
        doHook(apkPath,newApkPath);
    }
}
