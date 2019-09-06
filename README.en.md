# [UnityAndroidHotUpdate]
[![release tag](https://img.shields.io/github/v/tag/sisong/UnityAndroidHotUpdate?label=release%20tag)](https://github.com/sisong/UnityAndroidHotUpdate/releases) 
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE) 
[![+issue Welcome](https://img.shields.io/github/issues-raw/sisong/UnityAndroidHotUpdate?color=green&label=%2Bissue%20welcome)](https://github.com/sisong/UnityAndroidHotUpdate/issues)   
   
[[README 中文版](README.md)]   
provide a way to hot update Unity app on Android, support code & resources, not need lua js or IL runtime etc..., will not disturb your project development; just loading the new version apk file to achieve.   
( dependent libraries [ApkDiffPatch], [xHook]. )   

[UnityAndroidHotUpdate]: https://github.com/sisong/UnityAndroidHotUpdate
[ApkDiffPatch]: https://github.com/sisong/ApkDiffPatch
[xHook]: https://github.com/iqiyi/xHook
[UnityAndroidIl2cppPatchDemo]: https://github.com/noodle1983/UnityAndroidIl2cppPatchDemo
[HDiffPatch]: https://github.com/sisong/HDiffPatch
[bsdiff]: http://www.daemonology.net/bsdiff/
[archive-patcher]: https://github.com/andrewhayden/archive-patcher
[v2+ sign]: https://source.android.com/security/apksigning/v2


## The effect of this solution
  When the android user running the app,  update logic can complete the task of downloaded the new version app in the background; When the app is restarted, user run with new version app (not need reinstall through system, the "hot update" means this)!   

## Theory of hot update
  When installing the Android app, the apk file is stored in path ```getApplicationContext().getPackageResourcePath()```, and the native abi library files are decompressed and stored in path ```getApplicationContext().getApplicationInfo().nativeLibraryDir``` , including libmain.so, libunity.so, libil2cpp.so or libmono*.so, etc...   
  This solution will store the updated version apk file to the update path in ```getApplicationContext().getFilesDir().getPath()```, and decompress the changed local abi library files from apk to the update path; When the app restarts, use the hook and java layer code to intercept the c file API in Unity's library files, map access path from the apk file and library files to the new apk file and new library files.   


## Feature
* **Implement simple and run fast**   
The theory and implement are simple, and support hot update of code and resources; support il2cpp and mono two code backend, support libunity.so, libmono.so and other library files changes; (currently does not support libmain.so changes)    
Solution import into project is simple, and it's source code is less, easy to modify by yourself; (the description of import project is at after of this document.)   
After the hot update, the execution performance of the app is not affected, unlike the slow performance and Activity compatibility problems after using the plugin (or virtual) apk solution;   
This solution will not disturb your Unity project development, not need other program language or constraints, such as lua js or IL runtime etc...   
* **The patch package by downloaded is small**   
Not need to download the complete apk file, you just need to download the difference between the existing version and the latest version; then you can create the latest version apk file on device;   
You can select [ApkDiffPatch] for apk file diff&patch, it can generate very small patch package; for example, just some simple code changes, the patch package size about hundreds of KB.     
* **Development environment and compatibility**   
Test used Unity5.6, Unity2017, Unity2018, Unity2019;   
Test used mono and il2cpp backend;   
Test on armeabi-v7a arm64-v8a and x86 device;   
Test supported Andorid4.1+, but only supported Andorid5+ when using Unity5.6 or Unity2017 + il2cpp backend;   
It is generally compatible, if the version of Unity unchanged (libmain.so will not change) and no new .so library files been added;   
The project's other .so libraries can be added to the list of libraries that are allowed to be advance load, for compatible with the hot update (added in the HotUnity.java file, see the description of improt project);   
Tested to create a simple app with the same Unity version, and successfully hot updated to an existing complex game app;   
Of course, those involving permissions or third-party services, etc., their compatibility need more testing.   


## How to import into your Unity project for testing
* Export project:  select Gradle and export project in your Unity project, and then modify it and do packaging by Android Studio;   
* Modify the exported project:   
add libhotunity.so(rebuild in path ```project_hook_unity_jni```) to project jniLibs;   
add ```com/github/sisong/HotUnity.java``` to project; (You can add the .so in this file that need hot update, which will be loaded if exist new version;)   
edit file UnityPlayerActivity.java in project; add code: ```import com.github.sisong.HotUnity;```, and add code: ```HotUnity.hotUnity(this);``` before ```mUnityPlayer = new UnityPlayer(this);```   
* Package the test project by Android Studio (you can automate the process of exporting modifying and packaging in Unity with the editor extension), the app should be able to running normally
 on the device; now you need test the app "hot" update to a new version;   
* Manual hot update test process: you have a new version app named update.apk, placed it in the HotUpdate subdirectory of the ```getApplicationContext().getFilesDir().getPath()``` (device path ```/data/data/<appid>/files/HotUpdate/```); Decompress the *.so file in ```lib/<this test device abi>/``` from update.apk, and place it in the directory ```HotUpdate/update.apk_lib/``` (can also dispose only changed .so file); restart the app on the device, you should be able to see that the new version is already running!   


## How to automate hot update by app
  (this info provides an way, but you can get the new version apk on device by your way)   
* We have a new version apk, I can use the diff tool to generate patch between the old and new versions; put the version upgrade info and patch files on the server;   
* When the app running, it checks the upgrade info and download the patch file; the app call patch algorithm to generate a new version apk(ie HotUpdate/update.apk) using the latest apk and patch file; then call hot_cache_lib function (project in ```tool_hot_cache_lib``` directory, can generate libhotcachelib.so) to cache the changed .so files in the new version apk; 
* The function hot_cache_lib_check that determines whether 2 apks can be hot updated, it is also in ```tool_hot_cache_lib```; of course, this function is not completely reliable, this is only check necessary;   
* Generate patch file between apks and merge it on device, you can use the [ApkDiffPatch]; if you want test diff application, you can download it at [releases](https://github.com/sisong/ApkDiffPatch/releases); The patch process needs to be executed on the user device. The project that generates libapkpatch.so is in the ```ApkDiffPatch/builds/android_ndk_jni_mk``` directory;   
* Note: ApkDiffPatch is specially optimized for zip file. Generally, it generates smaller patch file size than [bsdiff] or [HDiffPatch]; The ZipDiff tool has special requirements for the input apk file, if the apk has [v2+ sign], then you need to normalized the apk files by the ApkNormalized tool; Then use AndroidSDK#apksigner to re-sign the apk; All the apks released for user need to done this process, this is to byte by byte restore the apk when patching;  (apk after this processed, it is also compatible with the patch size optimization scheme [archive-patcher] of the Google Play Store)   
* In practice, it is recommended to compile the code of libhotcachelib.so and libapkpatch.so into a file of libhotunity.so;   


## Recommend a process for multi version update
```
v0 -> new install(v0)

v1 -> diff(v0,v1) -> pat01 ; v0: patch(v0,download(pat01)) -> v1 + cache_lib(v1)
      ( if error on patch then: download(v1) -> v1 + cache_lib(v1)
        if error on cache_lib then: install(v1)  )

v2 -> diff(v0,v2) -> pat02 ; v0: patch(v0,download(pat02)) -> v2 + cache_lib(v2)
      diff(v1,v2) -> pat12 ; v1: patch(v1,download(pat12)) -> v2 + cache_lib(v2)

v3 -> (if no patch for v0) ; v0: download(v3) -> v3 + cache_lib(v3)
      diff(v1,v3) -> pat13 ; v1: patch(v1,download(pat13)) -> v3 + cache_lib(v3)
      diff(v2,v3) -> pat23 ; v2: patch(v2,download(pat23)) -> v3 + cache_lib(v3)

if is_need_install(v3,v4):
v4 -> (if no patch for v0) ; v0: download(v4) -> v4 + install(v4)
      diff(v1,v4) -> pat14 ; v1: patch(v1,download(pat14)) -> v4 + install(v4)
      diff(v2,v4) -> pat24 ; v2: patch(v2,download(pat24)) -> v4 + install(v4)
      diff(v3,v4) -> pat34 ; v3: patch(v3,download(pat34)) -> v4 + install(v4)

v5 -> (if no patch for v0) ; v0: download(v5) -> v5 + install(v5)
      (if no patch for v1) ; v1: download(v5) -> v5 + install(v5)
      diff(v2,v5) -> pat25 ; v2: patch(v2,download(pat25)) -> v5 + install(v5)
      diff(v3,v5) -> pat35 ; v3: patch(v3,download(pat35)) -> v5 + install(v5)
      diff(v4,v5) -> pat45 ; v4: patch(v4,download(pat45)) -> v5 + cache_lib(v5)
```
This new version and patch release process needs to be automated, and test whether the patch files is correct, generate configuration by check can be hot updated, etc...   


## Defect   
* After changed the Unity version, apk can't be hot update, the new apk needs to be installed, and the same Unity version can continue to hot update;    
* Can not switch between il2cpp and mono backend apks by hot update, the new apk needs to be installed;   
* When apk using Unity5.6 or Unity2017 + il2cpp backend, it is not currently supported on Android4, the new apk needs to be installed;   
* The solution can only support Android and can't be applied on iOS; (The app developed by Unity run on PC, if needs difference update, you can using [HDiffPatch] to diff&patch between directory.)   
* The diff&patch algorithm selected [ApkDiffPatch], which may not support this situation: apk must support [v2+ sign], but released apk cannot be signed by self, then it is impossible to version control and diff;   
* The new apk file and cached .so files take up disk space;  A recommended solution: fixed Unity version, initial version with a minimized apk (or obb split mode), subsequent updates are the full version; (Another possible improvement is to use the concept of virtual apk: Use hook to map the access to the original apk file when file API access unchanged entry files  in virtual apk; the patch process also requires a new implementation; a similar implementation see [UnityAndroidIl2cppPatchDemo].)   
   
   
by housisong@gmail.com
