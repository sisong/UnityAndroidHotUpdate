using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEngine.UI;

public class MenuClick : MonoBehaviour
{
    Text txtError=null;
    Text txtDownloadURL = null;
    Text txtDownloadProgress = null;
    string zipDiffPath = null;
    string zipDiffURL = "https://github.com/sisong/UnityAndroidHotUpdate/releases/download/v1.0.0-beta/updateDemo_oldV_newV.pat";
    void Start()
    {
        txtError = (Text)GameObject.Find("Canvas/txtError").GetComponent<Text>();
        txtDownloadProgress = (Text)GameObject.Find("Canvas/txtDownloadProgress").GetComponent<Text>();
        txtDownloadURL = (Text)GameObject.Find("Canvas/txtDownloadURL").GetComponent<Text>();
        zipDiffPath = Application.temporaryCachePath + "/temp_new.pat";

        Button btn = GetComponent<Button>();
        if (btn.name == "btnDownloadPat")
            btn.onClick.AddListener(downloadPatClick);
        else if (btn.name == "btnHotUpdate")
            btn.onClick.AddListener(hotUpdateClick);
        else if (btn.name == "btnInstallUpdate")
            btn.onClick.AddListener(installUpdateClick);
    }

    private WWW downloadOperation=null;
    void Update()
    {
        if (downloadOperation != null) {
            if (!downloadOperation.isDone)
                txtDownloadProgress.text = ((int)(downloadOperation.progress * 100.0)).ToString() + "%";
            else
                txtDownloadProgress.text = "100%";
        }
    }
    private IEnumerator doDownload(string url, string localPath)
    {
        downloadOperation = new WWW(url);
        yield return downloadOperation;

        Byte[] b = downloadOperation.bytes;
        File.WriteAllBytes(localPath, b);
    }

    public void downloadPatClick()
    {
        Debug.Log("downloadPatClick");
        if (File.Exists(zipDiffPath)) File.Delete(zipDiffPath);
        txtDownloadURL.text = zipDiffURL;
        Debug.Log("download URL: " + zipDiffURL);
        StartCoroutine(doDownload(zipDiffURL,zipDiffPath));
    }

    public void hotUpdateClick()
    {
#if UNITY_ANDROID && !UNITY_EDITOR
        Debug.Log("hotUpdateClick");
        if (!File.Exists(zipDiffPath)) {
            txtError.text = "hot update error, zipDiffPath not exists ";
            return;
        }
        using (AndroidJavaClass HotUnity = new AndroidJavaClass("com.github.sisong.HotUnity"))
        {
            int patchResult=HotUnity.CallStatic<int>("apkPatch",zipDiffPath,1,"");
            if (patchResult==0) { //ok
                File.Delete(zipDiffPath);
                HotUnity.CallStatic("restartApp");
            }else{ //error
                txtError.text="hot update error, HotUnity.apkPatch() result "+patchResult.ToString();
            }
        }
#endif
    }
    public void installUpdateClick()
    {
#if UNITY_ANDROID && !UNITY_EDITOR
        Debug.Log("installUpdateClick");
        if (!File.Exists(zipDiffPath)) {
            txtError.text = "install update error, zipDiffPath not exists ";
            return;
        }
        using (AndroidJavaClass HotUnity = new AndroidJavaClass("com.github.sisong.HotUnity"))
        {
            string newApkPath= Application.temporaryCachePath + "/temp_new.apk";
            if (File.Exists(newApkPath)) File.Delete(newApkPath);
            int patchResult = HotUnity.CallStatic<int>("apkPatch", zipDiffPath, 3, newApkPath);
            if (patchResult == 0) { //ok
                File.Delete(zipDiffPath);
                HotUnity.CallStatic("installApk", newApkPath);
            }else{ //error
                txtError.text = "install update error, HotUnity.apkPatch() result " + patchResult.ToString();
            }
        }
#endif
    }
}
