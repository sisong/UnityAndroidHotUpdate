# [UnityAndroidHotUpdate]
[![release tag](https://img.shields.io/github/v/tag/sisong/UnityAndroidHotUpdate?label=release%20tag)](https://github.com/sisong/UnityAndroidHotUpdate/releases) 
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE) 
[![+issue Welcome](https://img.shields.io/github/issues-raw/sisong/UnityAndroidHotUpdate?color=green&label=%2Bissue%20welcome)](https://github.com/sisong/UnityAndroidHotUpdate/issues)   
   
[[README English](README.en.md)]  
提供了一个在Android上热更新Unity开发的app的方案，支持代码和资源；不依赖其他语言(lua,js等)不干涉项目开发过程；它通过直接加载新版本apk文件来实现的。   
( 依赖的库 [ApkDiffPatch], [xHook]. )   

[UnityAndroidHotUpdate]: https://github.com/sisong/UnityAndroidHotUpdate
[ApkDiffPatch]: https://github.com/sisong/ApkDiffPatch
[xHook]: https://github.com/iqiyi/xHook
[UnityAndroidIl2cppPatchDemo]: https://github.com/noodle1983/UnityAndroidIl2cppPatchDemo
[HDiffPatch]: https://github.com/sisong/HDiffPatch
[bsdiff]: http://www.daemonology.net/bsdiff/
[archive-patcher]: https://github.com/andrewhayden/archive-patcher
[v2版及以上签名]: https://source.android.com/security/apksigning/v2


## 该方案的效果
  Android用户在使用app过程中，更新逻辑可以在后台完成下载新版本的任务，当app重新启动后用户看到的是新版本的app(不需要系统安装); 这里的“热更”就是指不需要经过系统的重新安装就拥有了新版本!   


## 热更原理
  Android程序在安装后,apk文件存放在```getApplicationContext().getPackageResourcePath()```;并将其中相应abi的库文件解压存放到了```getApplicationContext().getApplicationInfo().nativeLibraryDir```,其中包括libmain.so,libunity.so,libil2cpp.so or libmono*.so,等;   
  本方案会在app运行中，将更新到的新版本apk存放到```getApplicationContext().getFilesDir().getPath()```的更新路径,并将apk中本机abi并发生了改变的库文件解压到更新路径; 当app重启后用hook和java层代码配合,将unity相关库访问apk文件和lib路径的c文件访问函数映射到访问我们准备好的新的apk文件和lib路径。   
  ( Hook Unity库的C文件API来实现热更的思路来源于[UnityAndroidIl2cppPatchDemo]，感谢，这里做了一些简化。 )   


## 特性
* **简单快速**   
原理和实现简单，并且支持代码和资源的热更新;同时支持il2cpp与mono这2种代码打包方式，支持libunity.so、libmono.so等库文件变动；(当前热更不支持libmain.so改动)   
接入项目简单，项目代码量小，也容易修改定制;（本文后面有接入项目说明）   
热更新后app执行速度不受影响，不像使用插件化方案后遇到的速度慢和Activity兼容等问题；   
这是一个几乎不改变Unity开发流程的方案，不需要引入额外的语言(如lua或js等)，也不要一个额外的中间层解释器(如ILRuntime)、动态加载反射、手工绑定等工作。   
* **下载流量小**   
不需要下载完整的apk文件，而只需要下载已有版本和最新版本之间的差异就可以了；然后在用户本地合成新的apk文件；   
这里选择用的[ApkDiffPatch]方案，可以生成非常小的补丁；比如只是一些简单代码修改，补丁一般在百k级别。     
* **运行环境和兼容性**   
在Unity5.6、Unity2017、Unity2018、Unity2019的多个版本上测试过；   
测试过分别使用mono代码后端和il2cpp代码后端；   
测试过armeabi-v7a、arm64-v8a和x86的设备；   
一般支持Andorid4.1及以上的系统,但使用Unity5.6或Unity2017和il2cpp代码后端时只支持Andorid5及以上；   
在发布使用的Unity版本不变(libmain.so也不会变)和没有新增.so库文件的情况下，一般都可以兼容；   
项目的常规.so库可以添加到允许更新的库列表中提前加载从而兼容热更(在HotUnity.java文件中添加,参见接入项目说明)；   
测试过用相同的unity版本新建了一个最简单的app，也可以更新到一个已有的复杂的游戏app；   
当然某些涉及权限、特殊第三方业务等是否兼容还需要更多测试。   


## 如何接入你的项目测试
* 导出项目: 将你的Unity项目选择Gradle导出项目,以便修改后交给Android Studio进行打包;   
* 修改导出的项目:    
 将libhotunity.so文件(重新build项目在```project_hook_unity_jni```目录里，注意abi路径对应)复制到项目的jniLibs下的相应子目录中；   
 将```com/github/sisong/HotUnity.java```文件复制到项目源代码的java相应路径中； (可以在这个文件中添加你需要支持热更的.so，这会立即加载可能存在的新版本库)   
 在项目的UnityPlayerActivity.java文件中```import com.github.sisong.HotUnity;```，并且在```mUnityPlayer = new UnityPlayer(this);```代码之前添加代码```HotUnity.hotUnity(this);```   
* 用Android Studio打包测试项目（你可以把这个导出、修改、打包的过程在Unity中利用编辑器扩展自动化下来；后续本仓库会更新到Demo中），app在设备上应该能够正常运行; 现在你需要测试热更新到一个新版本；   
* 手工热更新测试过程：假设有了修改过的新版本apk命名成update.apk,放置到```getApplicationContext().getFilesDir().getPath()```目录的HotUpdate子目录下（一般设备上路径```/data/data/<appid>/files/HotUpdate/```）； 将update.apk包中```lib/<本测试设备abi>/```中的*.so文件解压后直接放置到```HotUpdate/update.apk_lib/```目录下（可以只放置有修改过的.so文件）； 重新运行设备上安装的app，你应该可以看到，已经运行的是新版本！   


## 如何在正式app里自动化热更
  app热更应该是一个程序自动化的过程，它将代替我们上面的手工测试过程；（本方案提供了一种可选实现路径，你可以自己想办法在客户端拿到新版本apk就行）   
* 假设开发了新版本，我可以用diff工具生成新旧版本之间的补丁；将版本升级信息和补丁文件放到服务器上；   
* 客户运行app时检查到升级信息并下载到相应的补丁文件，app用patch算法将本机最新apk和补丁生成新版本apk(即HotUpdate/update.apk), 然后调用hot_cache_lib函数(项目在```tool_hot_cache_lib```目录里，生成libhotcachelib.so)来缓存apk中有修改过的.so文件；   
* 判断2个apk是否可以热更新的函数hot_cache_lib_check也在```tool_hot_cache_lib```里，当然以这个函数为准并不完全可靠，这只是检查了热更的必要条件；   
* 生成apk间的补丁和在设备上合并，可以使用[ApkDiffPatch]项目; diff程序如果不想自己编译，可以在[releases](https://github.com/sisong/ApkDiffPatch/releases)下载到；patch过程需要在用户设备上执行，生成libapkpatch.so的项目在```ApkDiffPatch/builds/android_ndk_jni_mk```目录里；   
* 注意：ApkDiffPatch针对zip进行了特别的优化，一般比[bsdiff]和[HDiffPatch]生成更小的补丁，其ZipDiff工具对输入的apk文件有特别的要求，如果apk有[v2版及以上签名]，那需要用库提供的ApkNormalized工具对apk进行标准化，然后再用AndroidSDK#apksigner对apk重新进行签名；所有对用户发布的apk都需要经过这个处理，这是为了patch时能够原样还原apk；（经过这个处理过的apk,也兼容谷歌Play商店的补丁大小优化方案[archive-patcher]）   
* 实践中建议可以将libhotcachelib.so、libapkpatch.so的代码都合并编译到libhotunity.so一个文件中；   


## 推荐一个多版本更新流程（较复杂的示例,考虑了支持需要重新安装的情况）
```
v0 -> new install(v0)

v1 -> diff(v0,v1) -> pat01 ; v0: patch(v0,download(pat01)) -> v1 + cache_lib(v1)
      ( 如果patch时发生错误: download(v1) -> v1 + cache_lib(v1)
        如果cache_lib时发生错误: install(v1)  )

v2 -> diff(v0,v2) -> pat02 ; v0: patch(v0,download(pat02)) -> v2 + cache_lib(v2)
      diff(v1,v2) -> pat12 ; v1: patch(v1,download(pat12)) -> v2 + cache_lib(v2)

v3 -> (假设放弃了对v0的补丁) ; v0: download(v3) -> v3 + cache_lib(v3)
      diff(v1,v3) -> pat13 ; v1: patch(v1,download(pat13)) -> v3 + cache_lib(v3)
      diff(v2,v3) -> pat23 ; v2: patch(v2,download(pat23)) -> v3 + cache_lib(v3)

if is_need_install(v3,v4):
v4 -> (假设放弃了对v0的补丁) ; v0: download(v4) -> v4 + install(v4)
      diff(v1,v4) -> pat14 ; v1: patch(v1,download(pat14)) -> v4 + install(v4)
      diff(v2,v4) -> pat24 ; v2: patch(v2,download(pat24)) -> v4 + install(v4)
      diff(v3,v4) -> pat34 ; v3: patch(v3,download(pat34)) -> v4 + install(v4)

v5 -> (假设放弃了对v0的补丁) ; v0: download(v5) -> v5 + install(v5)
      (假设放弃了对v1的补丁) ; v1: download(v5) -> v5 + install(v5)
      diff(v2,v5) -> pat25 ; v2: patch(v2,download(pat25)) -> v5 + install(v5)
      diff(v3,v5) -> pat35 ; v3: patch(v3,download(pat35)) -> v5 + install(v5)
      diff(v4,v5) -> pat45 ; v4: patch(v4,download(pat45)) -> v5 + cache_lib(v5)
```
这个新版本和补丁的发布过程，需要自动化；并且自动测试补丁的正确性并检查配置是否能够热更新等。   


## 已知缺点   
* 切换升级Unity版本后无法热更，新apk需要安装，相同Unity版本才能继续热更；    
* 不能随意切换il2cpp与mono代码打包方式，否则无法热更新，apk需要重新安装；   
* 使用Unity5.6或Unity2017+il2cpp代码后端时，现在不支持在Android4系统上热更，apk需要重新安装；   
* 方案只能支持android，无法应用到iOS上；（PC上Unity开发的app需要支持差异更新可以考虑使用[HDiffPatch]之类支持目录间diff和patch的方案就可以了）   
* diff&patch方案选择了[ApkDiffPatch]方案，该方案可能不能支持这种情况：apk必须要支持[v2版及以上签名]发布，但签名权又不在自己手中，而在渠道手中，并且造成了无法进行版本控制和diff的；   
* 得到的新apk文件和库缓存会长期占用磁盘空间；一个实践方案是：固定Unity版本，初始版本用一个最小化的apk(或者obb分离模式)，后续更新后才是完整版本；(另一个可能的改进方案是使用虚拟apk的概念：将没有改变的entry文件利用hook将访问映射到原apk文件里，补丁逻辑也需要另外实现；类似的实现参见[UnityAndroidIl2cppPatchDemo])。   
   
   
by housisong@gmail.com   
