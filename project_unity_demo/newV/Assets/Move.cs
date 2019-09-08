using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Move : MonoBehaviour {

	// Use this for initialization
	void Start () {
		
	}
    
    float TranslateSpeedTime =1f;
    // Update is called once per frame
    void Update(){
        TranslateSpeedTime += Time.deltaTime;
        transform.Translate(Vector3.right * Time.deltaTime*7);
        if (TranslateSpeedTime > 2.0f){
            transform.Rotate(0, 180, 0);
            TranslateSpeedTime = 0f;
        }
    }
}
