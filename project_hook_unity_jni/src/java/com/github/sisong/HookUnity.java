package com.github.sisong;
/* customize edit for Unity export android project:
    add this file HookUnity.java to project;
    add libhookunity.so to project libs;
    edit file UnityPlayerActivity.java in project;
        add code: import com.github.sisong.HookUnity;
        change code: mUnityPlayer = new UnityPlayer(this);
        to: mUnityPlayer = new UnityPlayer(this);
            String apkPath=getApplicationContext().getPackageResourcePath();
            String libDir=getApplicationContext().getApplicationInfo().nativeLibraryDir;
            String newApkPath=getApplicationContext().getFilesDir().getPath() + "/HotUpdate/hot.apk";
            String libCacheDir=newApkPath+"_lib";
            HookUnity.hookUnity(apkPath,libDir,newApkPath,libCacheDir);
*/

public class HookUnity{
    public static native void doHook(String apkPath,String libDir,String newApkPath,String libCacheDir);
    public static void hookUnity(String apkPath,String libDir,String newApkPath,String libCacheDir){
        System.loadLibrary("main");
        System.loadLibrary("unity");
        System.loadLibrary("hookunity");
        doHook(apkPath,libDir,newApkPath,libCacheDir);
    }
}

