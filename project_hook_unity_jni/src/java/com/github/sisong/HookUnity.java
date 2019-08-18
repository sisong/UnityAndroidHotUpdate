package com.github.sisong;
public class HookUnity{
    public static native void doHook(String apkPath,String soDir,String newApkPath,String soCacheDir);
    public static void hookUnity(String apkPath,String soDir,String newApkPath,String soCacheDir){
        System.loadLibrary("main");
        System.loadLibrary("unity");
        System.loadLibrary("hookunity");
        doHook(apkPath,soDir,newApkPath,soCacheDir);
    }
}

