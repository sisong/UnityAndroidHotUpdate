package com.github.sisong;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.Context;
import android.util.Log;
import java.io.File;

/*
 customize edit for Unity export android project:
 1. add this file HotUnity.java to project;
 2. add libhotunity.so to project jniLibs;
 3. edit file UnityPlayerActivity.java in project;
 add code: `import com.github.sisong.HotUnity;`
 add code: `HotUnity.hotUnity(this);` before `mUnityPlayer = new UnityPlayer(this);`
 */

public class HotUnity{
    private static native void doHot(String baseApk,String baseSoDir,String hotApk,String hotSoDir);
    private static native int  virtualApkPatch(String baseApk,String baseSoDir,
                                               String hotApk,String hotSoDir, //first patch hotApk,hotSoDir can not exist or ""
                                               String out_newApk,String out_newSoDir,//for install(apk) out_newSoDir set "", not cache libs
                                               String zipDiffPath,int threadNum);
    private static native int  virtualApkMerge(String baseApk,String baseSoDir,
                                               String hotApk,String hotSoDir,
                                               String newApk,String newSoDir);
    
    private static final String kHotUnityLib ="hotunity";
    private static final String kLogTag ="HotUnity";
    public static void hotUnity(Context app){
        String baseApk=app.getPackageResourcePath();
        String baseSoDir=app.getApplicationInfo().nativeLibraryDir;
        String hotApk=app.getFilesDir().getPath() + "/HotUpdate/update.apk";
        String hotSoDir=hotApk+"_lib";
        String newApk=app.getFilesDir().getPath() + "/HotUpdate/new_update.apk";//ApkPatch temp out
        String newSoDir=newApk+"_lib";
        //for DEBUG test
        testHotUpdate(app, baseApk,baseSoDir,hotApk,hotSoDir,newApk,newSoDir);
        //merge new to hot for patch result
        if (!mergeNewUpdate(baseApk,baseSoDir,hotApk,hotSoDir,newApk,newSoDir)){
            removeFile(newApk);
            removeFile(hotApk);
            restartApp(app);
        }
        
        if (pathIsExists(hotApk)&&pathIsExists(hotSoDir)){ //run app by hot update
            mapPathLoadLib(hotSoDir,"main");
            mapPathLoadLib(hotSoDir,"unity");
            mapPathLoadLib(hotSoDir,kHotUnityLib);
            //note: You can load other your lib(not unity's) by mapPathLoadLib, can use newVersion lib;
            
            doHot(baseApk,baseSoDir,hotApk,hotSoDir);
        }else{ //run base apk
            System.loadLibrary("main");
            System.loadLibrary(kHotUnityLib);
        }
    }
    
    private static void restartApp(Context app) {
        Intent intent = app.getPackageManager().getLaunchIntentForPackage(app.getPackageName());
        PendingIntent restartIntent = PendingIntent.getActivity(app.getApplicationContext(),
                                                                0, intent, PendingIntent.FLAG_ONE_SHOT);
        AlarmManager mgr = (AlarmManager)app.getSystemService(Context.ALARM_SERVICE);
        mgr.set(AlarmManager.RTC,System.currentTimeMillis() + 200, restartIntent);
        System.exit(0);
    }
    private static void exitApp(Context app,int errCode) {
        throw new RuntimeException("exit code: "+String.valueOf(errCode));
    }
    
    private static void testHotUpdate(Context app,
                                      String baseApk,String baseSoDir,
                                      String hotApk,String hotSoDir,
                                      String newApk,String newSoDir){
        if (pathIsExists(newApk)||(pathIsExists(newSoDir))) return;
        String testDir=app.getExternalFilesDir("testHotUpdate").getAbsolutePath();
        //String testDir="/sdcard/Android/data/<your-app-id>/files/testHotUpdate";
        //NOTE: put the files you need test into the testDir directory
        String testPatFile=testDir+"/new.pat"; //test pat file
        if (!pathIsExists(testPatFile)) return;
        Log.w("PATCH", "testHotUpdate() with "+testPatFile);
        mapPathLoadLib(hotSoDir,kHotUnityLib); //for native function: virtualApkPatch()
        
        baseApk  =findTestPath(baseApk,  testDir+"/base.apk");
        baseSoDir=findTestPath(baseSoDir,testDir+"/base.apk_lib");
        hotApk   =findTestPath(hotApk,   testDir+"/update.apk");
        hotSoDir =findTestPath(hotSoDir, testDir+"/update.apk_lib");
        
        int  ret=virtualApkPatch(baseApk,baseSoDir,hotApk,hotSoDir,
                                 newApk,newSoDir,testPatFile,3);
        Log.w("PATCH", "virtualApkPatch() result " +String.valueOf(ret));
        if ((ret==0) && removeFile(testPatFile)){ //update ok
            restartApp(app);
        }else{
            Log.w("PATCH", "testHotUpdate() ERROR, exitApp");
            exitApp(app,ret);
        }
    }
    private static String findTestPath(String oldPath,String testPath){
        return pathIsExists(testPath)?testPath:oldPath;
    }
    
    // merge new to hot
    private static  boolean mergeNewUpdate(String baseApk,String baseSoDir,
                                           String hotApk,String hotSoDir,
                                           String newApk,String newSoDir){
        if (!pathIsExists(hotApk)) {
            if (pathIsExists(hotSoDir)) removeLibDirWithLibs(hotSoDir);
        }
        if (!pathIsExists(newApk)) {
            if (pathIsExists(newSoDir)) removeLibDirWithLibs(newSoDir);
            return true;
        }
        
        if (!mergeHotUnityLib(newSoDir,hotSoDir)){
            Log.w(kLogTag,"mergeHotUnityLib() error! "+newSoDir);
            return false;
        }
        mapPathLoadLib(hotSoDir,kHotUnityLib); //for native function: virtualApkMerge()
        int rt=virtualApkMerge(baseApk,baseSoDir,hotApk,hotSoDir,newApk,newSoDir);
        if (rt!=0){
            Log.w(kLogTag,"virtualApkMerge() error code "+String.valueOf(rt)+"! "+newApk);
            return false;
        }
        return true;
    }
    
    private static void removeLibDirWithLibs(String libDir) {
        File dir=new File(libDir);
        String[] files=dir.list();
        for (int i=0;i<files.length;++i) {
            String fileName=files[i];
            if ((fileName=="."||(fileName==".."))) continue;
            removeFile(fileName);
        }
    }
    
    private static boolean mergeHotUnityLib(String newSoDir,String hotSoDir){
        mapPathLoadLib(hotSoDir,kHotUnityLib);
        String newLibHotUnity=getLibPath(newSoDir,kHotUnityLib);
        if  (!pathIsExists(newLibHotUnity)) return true;
        if (!makeLibDir(hotSoDir)) return false;
        String hotLibHotUnity=getLibPath(hotSoDir,kHotUnityLib);
        if (!removeFile(hotLibHotUnity)) return false;
        return moveFileTo(newLibHotUnity,hotLibHotUnity);
    }
    
    private static boolean moveFileTo(String oldFilePath,String newFilePath) {
        File df=new File(oldFilePath);
        File newdf=new File(newFilePath);
        return df.renameTo(newdf);
    }
    private static boolean removeFile(String fileName) {
        File df=new File(fileName);
        if (!df.exists()) return true;
        return df.delete();
    }
    
    private static boolean makeLibDir(String libDir) {
        File df=new File(libDir);
        if (df.exists()) return true;
        if (!df.mkdir()) return false;
        return true;
    }
    
    private static void mapPathLoadLib(String hotSoDir, String libName){
        String cachedLibPath=getLibPath(hotSoDir,libName);
        if (pathIsExists(cachedLibPath)) {
            Log.w(kLogTag,"java map_path() to "+cachedLibPath);
            System.load(cachedLibPath);
        } else {
            Log.w(kLogTag,"java map_path() not found "+cachedLibPath);
            System.loadLibrary(libName);
        }
    }
    private static String getLibPath(String dir,String libName){
        return dir+"/"+System.mapLibraryName(libName);
    }
    private static boolean pathIsExists(String path) {
        File file = new File(path);
        return file.exists();
    }
}
