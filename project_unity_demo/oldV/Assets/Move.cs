using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Move : MonoBehaviour {

	// Use this for initialization
	void Start () {
		
	}
    
    float TranslateSpeed = 0.1f;
    float TranslateSpeedTime =0;
    // Update is called once per frame
    void Update(){
        TranslateSpeedTime += 0.1f;
        transform.Translate(Vector3.forward * TranslateSpeed);
        if (TranslateSpeedTime > 20.0f){
            transform.Rotate(0, 180, 0);
            TranslateSpeedTime = 0f;
        }
    }
}
