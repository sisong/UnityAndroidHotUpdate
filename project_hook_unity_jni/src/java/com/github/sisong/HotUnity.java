package com.github.sisong;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.Context;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.util.Log;
import java.io.File;

import android.support.v4.content.FileProvider;
//import androidx.core.content.FileProvider;

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
                                               String hotApk,String hotSoDir,
                                               String out_newApk,String out_newSoDir,//if need install(out_newApk), out_newSoDir set ""
                                               String zipDiffPath,int threadNum);
    private static native int  virtualApkMerge(String baseApk,String baseSoDir,
                                               String hotApk,String hotSoDir,
                                               String newApk,String newSoDir);

    private static final String kHotUnityLib ="hotunity";
    private static final String kLogTag ="HotUnity";
    public static Context app=null;
    public static String baseApk=null;
    public static String baseSoDir=null;
    public static String updateDirPath=null;
    public static String hotApk=null;
    public static String hotSoDir=null;
    public static String newApk=null;
    public static String newSoDir=null;
    public static void hotUnity(Context _app){
        app=_app;
        baseApk=app.getPackageResourcePath();
        baseSoDir=app.getApplicationInfo().nativeLibraryDir;
        updateDirPath=app.getFilesDir().getPath() + "/HotUpdate";
        if (!makeDir(updateDirPath)) { runByBaseApk(); return; }
        hotApk=updateDirPath+"/update.apk";
        hotSoDir=hotApk+"_lib";
        newApk=updateDirPath+"/new_update.apk";//ApkPatch temp out
        newSoDir=newApk+"_lib";
        
        //for DEBUG test
        testHotUpdate(app, baseApk,baseSoDir,hotApk,hotSoDir,newApk,newSoDir);
        
        //merge new to hot for patch result
        if (!mergeNewUpdate(baseApk,baseSoDir,hotApk,hotSoDir,newApk,newSoDir))
            revertToBaseApk();

        boolean isRunHot=pathIsExists(hotApk)&&pathIsExists(hotSoDir);
        if (isRunHot){
            if (!isVersionRise())
                isRunHot=false;
        }
        if (isRunHot) {
            runByHotApk();
        }else{
            removeHotToBaseApk();
            runByBaseApk();
        }
    }

    //version check: hotApk's versionCode > baseApk's versionCode
    private static boolean isVersionRise() {
        boolean isRise=false;
        try {
            int hotApkVersion=getVersionCodeFromApk(app, hotApk);
            int baseApkVersion=getVersionCode(app, app.getPackageName());
            isRise = hotApkVersion>baseApkVersion;
        }catch (Exception e){
        }
        return isRise;
    }

    private  static void  runByBaseApk(){
        System.loadLibrary("main");
        System.loadLibrary(kHotUnityLib);
    }
    private  static void  runByHotApk(){
        mapPathLoadLib(hotSoDir,"main");
        mapPathLoadLib(hotSoDir,"unity");
        mapPathLoadLib(hotSoDir,kHotUnityLib);
        //note: You can load other your lib(not unity's) by mapPathLoadLib, can use newVersion lib;

        doHot(baseApk,baseSoDir,hotApk,hotSoDir);
    }

    //public funcs for call by C#
    public static int apkPatch(String zipDiffPath,int threadNum,String installApkPath){
        removeFile(newApk);
        removeLibDirWithLibs(newSoDir);
        boolean isHotUpdate=(installApkPath==null)||(installApkPath.isEmpty());
        //kHotUnityLib is loaded, not need: mapPathLoadLib(hotSoDir,kHotUnityLib);
        String tempApkPath=isHotUpdate?(updateDirPath+"/new_apk.tmp"):(installApkPath+".tmp");
        removeFile(tempApkPath);
        if ((!isHotUpdate)&&pathIsExists(installApkPath)) removeFile(installApkPath);
        int  ret=virtualApkPatch(baseApk,baseSoDir,hotApk,hotSoDir,
                tempApkPath,isHotUpdate?newSoDir:"",zipDiffPath,threadNum);
        Log.w(kLogTag, "virtualApkPatch() result " +String.valueOf(ret));
        if (ret!=0) {
            removeFile(tempApkPath);
            if (isHotUpdate) removeLibDirWithLibs(newSoDir);
        }else {
            String targetApkFile=isHotUpdate?newApk:installApkPath;
            if (!moveFileTo(tempApkPath,targetApkFile)){
                Log.w(kLogTag, tempApkPath + " moveFileTo " + targetApkFile + " failed.");
                return -1;
            }
            if (isHotUpdate) makeDir(hotSoDir);
        }
        return ret;
    }
    public static void installApk(String installApkPath) {
        File apkFile=new File(installApkPath);
        Intent intent=new Intent("android.intent.action.INSTALL_PACKAGE");
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setAction(Intent.ACTION_VIEW);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            Uri uri = FileProvider.getUriForFile(app, app.getApplicationContext().getPackageName() + ".fileprovider", apkFile);
            intent.setDataAndType(uri, "application/vnd.android.package-archive");
        } else {
            intent.setDataAndType(Uri.fromFile(apkFile), "application/vnd.android.package-archive");
        }
        app.startActivity(intent);
    }
    public static void restartApp() {
        Intent intent = app.getPackageManager().getLaunchIntentForPackage(app.getPackageName());
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_CLEAR_TASK|Intent.FLAG_ACTIVITY_NEW_TASK);
        PendingIntent pendingIntent = PendingIntent.getActivity(app.getApplicationContext(),
                0, intent, PendingIntent.FLAG_ONE_SHOT);
        AlarmManager mgr = (AlarmManager)app.getSystemService(Context.ALARM_SERVICE);
        long alarmTimeMillis = System.currentTimeMillis() + 1000;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
            mgr.setExactAndAllowWhileIdle(AlarmManager.RTC, alarmTimeMillis, pendingIntent);
        else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
            mgr.setExact(AlarmManager.RTC, alarmTimeMillis, pendingIntent);
        else
            mgr.set(AlarmManager.RTC, alarmTimeMillis, pendingIntent);
        System.exit(0);
    }
    public static void exitApp(int errCode) {
        Intent mainIntent = new Intent(Intent.ACTION_MAIN);
        mainIntent.addCategory(Intent.CATEGORY_HOME);
        mainIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        app.startActivity(mainIntent);
        System.exit(errCode);
    }
    private static void removeHotToBaseApk(){
        removeFile(newApk);
        removeLibDirWithLibs(newSoDir);
        removeFile(hotApk);
        removeLibDirWithLibs(hotSoDir);
        clearCache();
    }
    public static void revertToBaseApk() {
        removeHotToBaseApk();
        restartApp();
    }

    private static void testHotUpdate(Context app,
                                      String baseApk,String baseSoDir,
                                      String hotApk,String hotSoDir,
                                      String newApk,String newSoDir){
        if (pathIsExists(newApk)||(pathIsExists(newSoDir))) return;
        String testDir=app.getExternalFilesDir("").getAbsolutePath()+"/testHotUpdate";
        //default: testDir=="/sdcard/Android/data/<your-app-id>/files/testHotUpdate";
        //NOTE: put the files you need test into the testDir directory
        String testPatFile=testDir+"/new.pat"; //test pat file
        if (!pathIsExists(testPatFile)) return;
        Log.w(kLogTag, "testHotUpdate() with "+testPatFile);
        mapPathLoadLib(hotSoDir,kHotUnityLib); //for native function: virtualApkPatch()

        String testBase=testDir+"/base.apk";
        if (pathIsExists(testBase)) {
            baseApk  = testBase;
            baseSoDir= testBase + "_lib";
        }
        String testHot=testDir+"/update.apk";
        if (pathIsExists(testBase)||pathIsExists(testHot)) {
            hotApk  = testHot;
            hotSoDir= testHot + "_lib";
        }
        int  ret=virtualApkPatch(baseApk,baseSoDir,hotApk,hotSoDir,
                 newApk,newSoDir,testPatFile,3);
        Log.w(kLogTag, "virtualApkPatch() result " +String.valueOf(ret));
        if ((ret==0) && removeFile(testPatFile)){ //update ok
            Log.w(kLogTag, "testHotUpdate() ok");
        }else{
            Log.w(kLogTag, "testHotUpdate() ERROR, exitApp");
            exitApp(ret);
        }
    }

    // merge new to hot
    private static  boolean mergeNewUpdate(String baseApk,String baseSoDir,
                                           String hotApk,String hotSoDir,
                                           String newApk,String newSoDir){
        if (!pathIsExists(hotApk))
            removeLibDirWithLibs(hotSoDir); //continue
        if (!pathIsExists(newApk)) {
            removeLibDirWithLibs(newSoDir);
            return true; //not need merge, continue run app
        }

        clearCache();
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

    private static boolean mergeHotUnityLib(String newSoDir,String hotSoDir){
        String newLibHotUnity=getLibPath(newSoDir,kHotUnityLib);
        if  (!pathIsExists(newLibHotUnity)) return true;
        if (!makeDir(hotSoDir)) return false;
        String hotLibHotUnity=getLibPath(hotSoDir,kHotUnityLib);
        if (!removeFile(hotLibHotUnity)) return false;
        return moveFileTo(newLibHotUnity,hotLibHotUnity);
    }

    private static void clearCache() {
        Log.i(kLogTag, "clearCache");
        File cacheDir = app.getExternalFilesDir("il2cpp");
        if (!removeDir(cacheDir))
            Log.w(kLogTag, "clearCache() error, can't clear dir \"" + cacheDir.getPath() + "\"! ");
    }

    private static void removeLibDirWithLibs(String libDir) {
        File soDir=new File(libDir);
        if (!removeDir(soDir))
            Log.w(kLogTag,"removeLibDirWithLibs() error, can't clear dir \""+libDir+"\"! ");
    }

    private static boolean moveFileTo(String oldFilePath,String newFilePath) {
        File df=new File(oldFilePath);
        File newdf=new File(newFilePath);
        return df.renameTo(newdf);
    }
    private static boolean removeFile(String fileName) {
        File f=new File(fileName);
        return removeFile(f);
    }
    private static boolean removeFile(File f) {
        if ((f==null)||(!f.exists()))
            return true;
        return f.delete();
    }
    private static boolean removeDir(File dir) {
        if ((null == dir) || (!dir.exists()))
            return true;
        if (!dir.isDirectory())
            throw new RuntimeException("\"" + dir.getAbsolutePath() + "\" should be a directory!");
        boolean result=true;
        File[] files = dir.listFiles();
        if ((null != files)&&(files.length>0)) {
            for (File f : files) {
                if (f.isDirectory())
                    result &=removeDir(f);
                else
                    result &=removeFile(f);
            }
        }
        return result & dir.delete();
    }

    private static boolean makeDir(String dirPath) {
        File df=new File(dirPath);
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
        return (file!=null)&&file.exists();
    }

    private static int getVersionCode(Context context,String packageName) throws PackageManager.NameNotFoundException {
        return context.getPackageManager().getPackageInfo(packageName, 0).versionCode;
    }
    private static int getVersionCodeFromApk(Context context,String apkFilePath) {
        return context.getPackageManager().
                getPackageArchiveInfo(apkFilePath,PackageManager.GET_ACTIVITIES).versionCode;
    }
}
