using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class MenuClick : MonoBehaviour
{
    void Start()
    {
        Button btn = GetComponent<Button>();
        if (btn.name=="btnRestartApp")
            btn.onClick.AddListener(restartAppClick);
        else if (btn.name == "btnExitApp")
            btn.onClick.AddListener(exitAppClick);
        else if (btn.name == "btnRevertApp")
            btn.onClick.AddListener(revertAppClick);
    }
    public void restartAppClick()
    {
#if UNITY_ANDROID && !UNITY_EDITOR
        Debug.Log("restartAppClick");
        using (AndroidJavaClass HotUnity = new AndroidJavaClass("com.github.sisong.HotUnity"))
        {
            HotUnity.CallStatic("restartApp");
        }
#endif
    }
    public void exitAppClick()
    {
#if UNITY_ANDROID && !UNITY_EDITOR
        Debug.Log("exitAppClick");
        using (AndroidJavaClass HotUnity = new AndroidJavaClass("com.github.sisong.HotUnity"))
        {
            int exitCode = 0;
            HotUnity.CallStatic("exitApp", exitCode);
        }
#endif
    }
    public void revertAppClick()
    {
#if UNITY_ANDROID && !UNITY_EDITOR
        Debug.Log("revertAppClick");
        using (AndroidJavaClass HotUnity = new AndroidJavaClass("com.github.sisong.HotUnity"))
        {
            HotUnity.CallStatic("revertToBaseApk");
        }
#endif
    }

}
